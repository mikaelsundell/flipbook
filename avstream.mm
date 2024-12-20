// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avstream.h"

#include <AVFoundation/AVFoundation.h>
#include <CoreMedia/CoreMedia.h>

#include <QPainter>
#include <QPointer>
#include <QThread>
#include <QtGlobal>

#include <QDebug>

class AVStreamPrivate
{
    public:
        AVStreamPrivate();
        ~AVStreamPrivate();
        void ui();
        void init();
        void open();
        void close();
        void read();
        void seek(CMTime time);
        void play();
    
    public:
        void debug_time(QImage& image, CMTime time) { // todo: temporary
            QPainter painter(&image);
            QFont font("Arial", 50, QFont::Bold);
            painter.setFont(font);
            painter.setPen(QColor(Qt::white));
            QString text = QString("AVTime: %1 / %2").arg(time.value).arg(time.timescale);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(text);
            int textHeight = fm.height();
            QPoint center((image.width() - textWidth) / 2, (image.height() + textHeight) / 2);
            painter.drawText(center, text);
            painter.end();
        }
        // https://developer.apple.com/library/archive/technotes/tn2310/_index.html
        int64_t to_frame(CVSMPTETime timecode, uint32_t framequanta, uint32_t flags);
        CVSMPTETime to_timecode(int64_t frame, uint32_t framequanta, uint32_t flags);
        AVTime to_time(CMTime other);
        CMTime to_time(AVTime other);
        AVTimeRange to_timerange(CMTimeRange other);
        AVSmpteTime to_timecode(CVSMPTETime other);
        bool compare_time(CMTime time, AVTime other);
        bool compare_timecode(CVSMPTETime timecode, AVSmpteTime other);
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* videooutput = nil;
        CVSMPTETime timecode;
        QString filename;
        CMTimeRange timerange;
        CMTime time;
        float rate = 0.0;
        std::atomic<bool> abort;
        AVMetadata metadata;
        AVSidecar sidecar;
        QString errormessage;
        AVStream::Error error;
        QPointer<AVStream> object;
};

AVStreamPrivate::AVStreamPrivate()
: abort(false)
{
}

AVStreamPrivate::~AVStreamPrivate()
{
}

void
AVStreamPrivate::init()
{
}

void
AVStreamPrivate::open()
{
    close();
    NSURL* url = [NSURL fileURLWithPath:filename.toNSString()];
    asset = [AVAsset assetWithURL:url];
    if (!asset) {
        error = AVStream::FILE_ERROR;
        errormessage = QString("unable to load asset from file: %1").arg(filename);
        qWarning() << "warning: " << errormessage;
        return false;
    }
    NSError* averror = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
    if (!reader) {
        error = AVStream::API_ERROR;
        errormessage = QString("unable to create AVAssetReader for video: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << errormessage;
        return false;
    }

    NSArray<NSString *>* metadataformats = @[
        AVMetadataKeySpaceCommon,
        AVMetadataFormatQuickTimeUserData,
        AVMetadataQuickTimeUserDataKeyAlbum,
        AVMetadataFormatISOUserData,
        AVMetadataISOUserDataKeyCopyright,
        AVMetadataISOUserDataKeyDate,
        AVMetadataFormatQuickTimeMetadata,
        AVMetadataQuickTimeMetadataKeyAuthor,
        AVMetadataFormatiTunesMetadata,
        AVMetadataiTunesMetadataKeyAlbum,
        AVMetadataFormatID3Metadata,
        AVMetadataID3MetadataKeyAudioEncryption,
        AVMetadataKeySpaceIcy,
        AVMetadataIcyMetadataKeyStreamTitle,
        AVMetadataFormatHLSMetadata,
        AVMetadataKeySpaceHLSDateRange,
        AVMetadataKeySpaceAudioFile,
        AVMetadataFormatUnknown
    ];
    
    for (NSString *format in metadataformats) {
        NSArray<AVMetadataItem *> *metadataForFormat = [asset metadataForFormat:format];
        if (metadataForFormat) {
            for (AVMetadataItem* item in metadataForFormat) {
                QString key = QString::fromNSString(item.commonKey);
                QString value = QString::fromNSString(item.value.description);
                if (!key.isEmpty() || !value.isEmpty()) {
                    metadata.add_pair(key, value);
                }
            }
        }
    }
    
    AVAssetTrack* videotrack = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!videotrack) {
        error = AVStream::API_ERROR;
        errormessage = QString("no video track found in file: %1").arg(filename);
        qWarning() << "warning: " << errormessage;
        return false;
    }
    timerange = videotrack.timeRange;
    
    qDebug() << "open time.value: " << timerange.start.value << " (" << timerange.start.timescale << ")";
    qDebug() << "open time.duration: " << timerange.duration.value << " (" << timerange.duration.timescale << ")";
    
    rate = videotrack.nominalFrameRate;

    AVAssetTrack* timecodetrack = [[asset tracksWithMediaType:AVMediaTypeTimecode] firstObject];
    if (timecodetrack) {
        AVAssetReader* timecodereader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
        if (!reader) {
            error = AVStream::API_ERROR;
            errormessage = QString("unable to create AVAssetReader for timecode: %1").arg(QString::fromNSString(averror.localizedDescription));
            qWarning() << "warning: " << errormessage;
            return false;
        }
        
        AVAssetReaderTrackOutput* timecodeoutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:timecodetrack outputSettings:nil];
        [timecodereader addOutput:timecodeoutput];
        bool success = [timecodereader startReading];
        if (success) {
            CMSampleBufferRef samplebuffer = NULL;
            while ((samplebuffer = [timecodeoutput copyNextSampleBuffer])) {
                CMBlockBufferRef blockbuffer = CMSampleBufferGetDataBuffer(samplebuffer);
                CMFormatDescriptionRef formatdescription =  CMSampleBufferGetFormatDescription(samplebuffer);
                if (blockbuffer && formatdescription) {
                    size_t length = 0;
                    size_t totallength = 0;
                    char* data = NULL;
                    OSStatus status = CMBlockBufferGetDataPointer(blockbuffer, 0, &length, &totallength, &data);
                    if (status == kCMBlockBufferNoErr) {
                        CMMediaType type = CMFormatDescriptionGetMediaSubType(formatdescription);
                        uint32_t framequanta = CMTimeCodeFormatDescriptionGetFrameQuanta(formatdescription);
                        uint32_t flags = CMTimeCodeFormatDescriptionGetTimeCodeFlags(formatdescription);
                        if (type == kCMTimeCodeFormatType_TimeCode32) {
                            int32_t* framenumber = (int32_t*)data;
                            // stored 32-bit big-endian - convert it to native
                            timecode = to_timecode(EndianS32_BtoN(*framenumber), framequanta, flags);
                        }
                        else if (type == kCMTimeCodeFormatType_TimeCode64) {
                            int64_t* framenumber = (int64_t *)data;
                            // stored 64-bit big-endian. Convert it to native
                            timecode = to_timecode(EndianS64_BtoN(*framenumber), framequanta, flags);
                        }
                    }
                    else {
                        error = AVStream::API_ERROR;
                        errormessage = QString("unable to get data from block buffer for timecode");
                        qWarning() << "warning: " << errormessage;
                        return false;
                    }
                }
            }
            if (samplebuffer) {
                CFRelease(samplebuffer);
            }
        }
        else {
            error = AVStream::API_ERROR;
            errormessage = QString("unable to read sample buffer at for timecode");
            qWarning() << "warning: " << errormessage;
            return false;
        }
    }
    videooutput = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:videotrack
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!videooutput) {
        error = AVStream::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return false;
    }
    [reader addOutput:videooutput];
    [reader startReading];
    object->opened(filename);
}

void
AVStreamPrivate::close()
{
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    asset = nil;
    reader = nil;
    videooutput = nil;
    timecode = CVSMPTETime();
    timerange = CMTimeRange();
    time = CMTime();
    abort = false;
    rate = 0.0f;
    metadata = AVMetadata();
    sidecar = AVSidecar();
    error = AVStream::NO_ERROR;
    errormessage = QString();
}

void
AVStreamPrivate::read()
{
    qDebug() << "pre-read sample buffer time.value: " << time.value << " (" << time.timescale << ")";
    
    if (!reader || reader.status != AVAssetReaderStatusReading) {
        error = AVStream::API_ERROR;
        errormessage = "AVAssetReader is not in a valid state";
        qWarning() << "warning: " << errormessage;
        return;
    }
    CMSampleBufferRef samplebuffer = [videooutput copyNextSampleBuffer];
    if (!samplebuffer) {
        error = AVStream::API_ERROR;
        errormessage = "unable to read sample buffer at current frame";
        qWarning() << "warning: " << errormessage;
    }
    CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(samplebuffer);
    if (!imagebuffer) {
        CFRelease(samplebuffer);
        error = AVStream::API_ERROR;
        errormessage = "CMSampleBuffer has no image buffer";
        qWarning() << "warning: " << errormessage;
        return;
    }
    CVPixelBufferLockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);
    void* baseAddress = CVPixelBufferGetBaseAddress(imagebuffer);
    size_t width = CVPixelBufferGetWidth(imagebuffer);
    size_t height = CVPixelBufferGetHeight(imagebuffer);
    size_t bytes = CVPixelBufferGetBytesPerRow(imagebuffer);
    QImage image(static_cast<uchar*>(baseAddress),
                 static_cast<int>(width),
                 static_cast<int>(height),
                 static_cast<int>(bytes),
                 QImage::Format_ARGB32);
    CVPixelBufferUnlockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);
    time = CMSampleBufferGetPresentationTimeStamp(samplebuffer);
    
    qDebug() << "post-read sample buffer time.value: " << time.value << " (" << time.timescale << ")";
    
    CFRelease(samplebuffer);
    debug_time(image, time); // todo: temporary
    object->image_changed(image.copy());
}

void
AVStreamPrivate::seek(CMTime time)
{
    if (!reader || reader.status != AVAssetReaderStatusReading) {
        error = AVStream::API_ERROR;
        errormessage = "AVAssetReader is not in a valid state";
        qWarning() << "warning: " << errormessage;
        return;
    }
    
    if (!asset) {
        error = AVStream::API_ERROR;
        errormessage = "asset is not loaded";
        qWarning() << "warning: " << errormessage;
        return;
    }
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    NSError* averror = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
    if (!reader) {
        error = AVStream::API_ERROR;
        errormessage = QString("failed to recreate AVAssetReader: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << errormessage;
        return;
    }
    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        error = AVStream::API_ERROR;
        errormessage = "no video track found in asset";
        qWarning() << "warning: " << errormessage;
        return;
    }
    videooutput = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!videooutput) {
        error = AVStream::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return;
    }
    [reader addOutput:videooutput];
    reader.timeRange = CMTimeRangeMake(time, timerange.duration);
    if (![reader startReading]) {
        error = AVStream::API_ERROR;
        errormessage = "failed to start reading after seeking";
        qWarning() << "warning: " << errormessage;
        return;
    }
}

void
AVStreamPrivate::play()
{
    qDebug() << "Play not implemented";
    
    //for(int i=frame(); i<end_frame(); i++) {
     //   if (!abort) {
    //        read();
     //       object->frame_changed(i);
    //        QThread::msleep((1 / rate) * 1000);
    //    }
    //    else {
    //        return;
    //    }
    //}

    object->finished();
}

int64_t
AVStreamPrivate::to_frame(CVSMPTETime timecode, uint32_t framequanta, uint32_t flags)
{
    int64_t framenumber = 0;
    framenumber = timecode.frames;
    framenumber += timecode.seconds * framequanta;
    framenumber += (timecode.minutes & ~0x80) * framequanta * 60; // 0x80 negative bit in minutes
    framenumber += timecode.hours * framequanta * 60 * 60;
    int64_t fpm = framequanta * 60;
    if (flags & kCMTimeCodeFlag_DropFrame) {
        int64_t fpm10 = fpm * 10;
        int64_t num10s = framenumber / fpm10;
        int64_t frameadjust = -num10s*(9 * 2);
        int64_t framesleft = framenumber % fpm10;
        
        if (framesleft > 1) {
            int64_t num1s = framesleft / fpm;
            if (num1s > 0) {
                frameadjust -= (num1s-1) * 2;
                framesleft = framesleft % fpm;
                if (framesleft > 1) {
                    frameadjust -= 2;
                }
                else {
                    frameadjust -= (framesleft+1);
                }
            }
        }
        framenumber += frameadjust;
    }
    if (timecode.minutes & 0x80) //check for kCMTimeCodeFlag_NegTimesOK here
        framenumber = -framenumber;
    return framenumber;
}

CVSMPTETime
AVStreamPrivate::to_timecode(int64_t frame, uint32_t framequanta, uint32_t flags)
{
    CVSMPTETime timecode = {0};
    short fps = framequanta;
    BOOL neg = FALSE;
    
    if (frame < 0) {
        neg = TRUE;
        frame = -frame;
    }
    if (flags & kCMTimeCodeFlag_DropFrame) {
        int64_t fpm = fps * 60 - 2;
        int64_t fpm10 = fps * 10 * 60 - 9 * 2;
        int64_t num10s = frame / fpm10;
        int64_t frameadjust = num10s*(9 * 2);
        int64_t framesleft = frame % fpm10;
        if (framesleft >= fps * 60) {
            framesleft -= fps * 60;
            int64_t num1s = framesleft / fpm;
            frameadjust += (num1s + 1) * 2;
        }
        frame += frameadjust;
    }
    timecode.frames = frame % fps;
    frame /= fps;
    timecode.seconds = frame % 60;
    frame /= 60;
    timecode.minutes = frame % 60;
    frame /= 60;
    if (flags & kCMTimeCodeFlag_24HourMax) {
        frame %= 24;
        if (neg && !(flags & kCMTimeCodeFlag_NegTimesOK)) {
            neg = FALSE;
            frame = 23 - frame;
        }
    }
    timecode.hours = frame;
    if (neg) {
        timecode.minutes |= 0x80;
    }
    timecode.flags = kCVSMPTETimeValid;
    return timecode;
}

AVTime
AVStreamPrivate::to_time(CMTime other) {
    AVTime time;
    if (CMTIME_IS_VALID(other)) {
        time.set_time(other.value);
        time.set_timescale(other.timescale);
    }
    return time;
}

CMTime
AVStreamPrivate::to_time(AVTime other) {
    return CMTimeMakeWithEpoch(other.time(), other.timescale(), 0); // default epoch and flags
}

AVTimeRange
AVStreamPrivate::to_timerange(CMTimeRange timerange) {
    AVTimeRange range;
    if (CMTIMERANGE_IS_VALID(timerange)) {
        range.set_start(to_time(timerange.start));
        range.set_duration(to_time(timerange.duration));
    }
    return range;
    
}

AVSmpteTime
AVStreamPrivate::to_timecode(CVSMPTETime other) {
    AVSmpteTime timecode;
    timecode.set_counter(other.counter);
    timecode.set_type(other.type);
    timecode.set_hours(other.hours);
    timecode.set_minutes(other.minutes);
    timecode.set_seconds(other.seconds);
    timecode.set_frames(other.frames);
    timecode.set_subframes(other.subframes);
    timecode.set_subframedivisor(other.subframeDivisor);
    return timecode;
}

bool
AVStreamPrivate::compare_time(CMTime time, AVTime other) {
    return to_time(time) == other;
}

bool
AVStreamPrivate::compare_timecode(CVSMPTETime timecode, AVSmpteTime other) {
    return to_timecode(timecode) == other;
}

AVStream::AVStream()
: p(new AVStreamPrivate())
{
    p->init();
    p->object = this;
}

AVStream::~AVStream()
{
}

void
AVStream::open(const QString& filename)
{
    p->filename = filename;
    p->open();
}

void
AVStream::read()
{
    p->read();
}

void
AVStream::close()
{
    p->close();
}

bool
AVStream::is_open() const
{
    return (p->reader && p->error == AVStream::NO_ERROR);
}

bool
AVStream::is_closed() const
{
    return !p->reader;
}

const QString&
AVStream::filename() const
{
    return p->filename;
}

AVTime
AVStream::time() const
{
    return p->to_time(p->time);
}

AVTimeRange
AVStream::range() const
{
    return p->to_timerange(p->timerange);
}

float
AVStream::rate() const
{
    return p->rate;
}

AVSmpteTime
AVStream::timecode() const
{
    return p->to_timecode(p->timecode);
}

AVStream::Error
AVStream::error() const
{
    return p->error;
}

QString
AVStream::error_message() const
{
    return p->errormessage;
}

AVMetadata
AVStream::metadata()
{
    return p->metadata;
}

AVSidecar
AVStream::sidecar()
{
    return p->sidecar;
}

void
AVStream::seek(AVTime time)
{
    p->seek(p->to_time(time));
}

void
AVStream::play()
{
    p->play();
}

void
AVStream::stop()
{
    p->abort = true;
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avreader.h"

#include <AVFoundation/AVFoundation.h>
#include <CoreMedia/CoreMedia.h>

#include <QPainter>
#include <QPointer>
#include <QThread>
#include <QtGlobal>

#include <QDebug>

class AVReaderPrivate
{
    public:
        AVReaderPrivate();
        ~AVReaderPrivate();
        void ui();
        void init();
        void open();
        void close();
        void read();
        void seek(CMTime time);
        void stream();
    
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
            QPoint position((image.width() - textWidth) / 2, image.height() - textHeight - 120);
            painter.drawText(position, text);
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
        CMTime scale_time(CMTime other, int32_t scale = 240000); // todo: 240000 enough for most needs?
        CMTimeRange scale_timerange(CMTimeRange other, int32_t scale = 240000);
        int64_t ticks_per_frame(CMTimeRange range, qreal fps);
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* videooutput = nil;
        CVSMPTETime timecode;
        QString filename;
        CMTimeRange timerange;
        CMTime timestamp;
        qreal fps = 0.0;
        std::atomic<bool> streaming;
        AVMetadata metadata;
        AVSidecar sidecar;
        QString errormessage;
        AVReader::Error error;
        QPointer<AVReader> object;
};

AVReaderPrivate::AVReaderPrivate()
: streaming(false)
{
}

AVReaderPrivate::~AVReaderPrivate()
{
}

void
AVReaderPrivate::init()
{
}

void
AVReaderPrivate::open()
{
    close();
    NSURL* url = [NSURL fileURLWithPath:filename.toNSString()];
    asset = [AVAsset assetWithURL:url];
    if (!asset) {
        error = AVReader::FILE_ERROR;
        errormessage = QString("unable to load asset from file: %1").arg(filename);
        qWarning() << "warning: " << errormessage;
        return false;
    }
    NSError* averror = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
    if (!reader) {
        error = AVReader::API_ERROR;
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
        error = AVReader::API_ERROR;
        errormessage = QString("no video track found in file: %1").arg(filename);
        qWarning() << "warning: " << errormessage;
        return false;
    }
    timerange = scale_timerange(videotrack.timeRange);
    timestamp = timerange.start;
    fps = videotrack.nominalFrameRate;
    AVAssetTrack* timecodetrack = [[asset tracksWithMediaType:AVMediaTypeTimecode] firstObject];
    if (timecodetrack) {
        AVAssetReader* timecodereader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
        if (!reader) {
            error = AVReader::API_ERROR;
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
                        error = AVReader::API_ERROR;
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
            error = AVReader::API_ERROR;
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
        error = AVReader::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return false;
    }
    [reader addOutput:videooutput];
    [reader startReading];
    object->opened(filename);
}

void
AVReaderPrivate::close()
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
    timestamp = CMTime();
    streaming = false;
    fps = 0.0f;
    metadata = AVMetadata();
    sidecar = AVSidecar();
    error = AVReader::NO_ERROR;
    errormessage = QString();
}

void
AVReaderPrivate::read()
{
    Q_ASSERT(reader || reader.status != AVAssetReaderStatusReading);
    
    CMSampleBufferRef samplebuffer = [videooutput copyNextSampleBuffer];
    if (!samplebuffer) {
        error = AVReader::API_ERROR;
        errormessage = "unable to read sample buffer at current frame";
        qWarning() << "warning: " << errormessage;
    }
    CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(samplebuffer);
    if (!imagebuffer) {
        CFRelease(samplebuffer);
        error = AVReader::API_ERROR;
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
    timestamp = scale_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer));
    CFRelease(samplebuffer);
    debug_time(image, timestamp); // todo: temporary
    object->time_changed(to_time(timestamp));
    object->video_changed(image.copy());
}

void
AVReaderPrivate::seek(CMTime time)
{
    Q_ASSERT(reader || reader.status != AVAssetReaderStatusReading);
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    NSError* averror = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&averror];
    if (!reader) {
        error = AVReader::API_ERROR;
        errormessage = QString("failed to recreate AVAssetReader: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << errormessage;
        return;
    }
    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        error = AVReader::API_ERROR;
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
        error = AVReader::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return;
    }
    [reader addOutput:videooutput];
    CMTime end = CMTimeAdd(timerange.start, timerange.duration);
    CMTime duration = CMTimeSubtract(end, time);
    reader.timeRange = CMTimeRangeMake(time, duration);
    if (![reader startReading]) {
        error = AVReader::API_ERROR;
        errormessage = "failed to start reading after seeking";
        qWarning() << "warning: " << errormessage;
        return;
    }
}

void
AVReaderPrivate::stream()
{
    streaming = true;
    qint64 ticks = ticks_per_frame(timerange, fps); // match the samples in video
    for (CMTime current = timestamp;
         CMTimeCompare(current, timerange.duration) < 0;
         current = CMTimeAdd(current, CMTimeMake(ticks, timerange.duration.timescale))) {
        if (streaming) {
            if (CMTimeCompare(CMTimeAdd(current, CMTimeMake(ticks, timerange.duration.timescale)), timerange.duration) >= 0) {
                timestamp = timerange.duration;
                object->time_changed(to_time(timestamp));
                break; // last frame until duration
            }
            qDebug() << "reading stream";
            qDebug() << "- ticks: " << ticks;
            qDebug() << "- current: " << current.value;
            qDebug() << "- duration: " << timerange.duration.value;
            
            read();
            
            // todo: drop frames, time elapsed after read() from gui
            QThread::msleep((1 / fps) * 1000);
        } else {
            break;
        }
    }
    object->finished();
}

int64_t
AVReaderPrivate::to_frame(CVSMPTETime timecode, uint32_t framequanta, uint32_t flags)
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
AVReaderPrivate::to_timecode(int64_t frame, uint32_t framequanta, uint32_t flags)
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
AVReaderPrivate::to_time(CMTime other) {
    AVTime time;
    if (CMTIME_IS_VALID(other)) {
        time.set_ticks(other.value);
        time.set_timescale(other.timescale);
    }
    return time;
}

CMTime
AVReaderPrivate::to_time(AVTime other) {
    return CMTimeMakeWithEpoch(other.ticks(), other.timescale(), 0); // default epoch and flags
}

AVTimeRange
AVReaderPrivate::to_timerange(CMTimeRange timerange) {
    AVTimeRange range;
    if (CMTIMERANGE_IS_VALID(timerange)) {
        range.set_start(to_time(timerange.start));
        range.set_duration(to_time(timerange.duration));
    }
    return range;
    
}

AVSmpteTime
AVReaderPrivate::to_timecode(CVSMPTETime other) {
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
AVReaderPrivate::compare_time(CMTime time, AVTime other) {
    return to_time(time) == other;
}

bool
AVReaderPrivate::compare_timecode(CVSMPTETime timecode, AVSmpteTime other) {
    return to_timecode(timecode) == other;
}

int64_t
AVReaderPrivate::ticks_per_frame(CMTimeRange range, qreal fps)
{
    int32_t timescale = timerange.duration.timescale;
    return static_cast<int64_t>(std::round(static_cast<qreal>(timescale) / fps));
}

CMTime
AVReaderPrivate::scale_time(CMTime other, int32_t scale)
{
    return CMTimeConvertScale(other, scale, kCMTimeRoundingMethod_Default);
}

CMTimeRange
AVReaderPrivate::scale_timerange(CMTimeRange other, int32_t scale)
{
    CMTimeRange range{ other.start, other.duration };
    range.start = scale_time(range.start, scale);
    range.duration = scale_time(range.duration, scale);
    return range;
}

AVReader::AVReader()
: p(new AVReaderPrivate())
{
    p->init();
    p->object = this;
}

AVReader::~AVReader()
{
}

void
AVReader::open(const QString& filename)
{
    p->filename = filename;
    p->open();
}

void
AVReader::read()
{
    p->read();
}

void
AVReader::close()
{
    p->close();
}

bool
AVReader::is_open() const
{
    return (p->reader && p->error == AVReader::NO_ERROR);
}

bool
AVReader::is_closed() const
{
    return !p->reader;
}

bool
AVReader::is_streaming() const
{
    return p->streaming;
}

const QString&
AVReader::filename() const
{
    return p->filename;
}

AVTime
AVReader::time() const
{
    return p->to_time(p->timestamp);
}

AVTimeRange
AVReader::range() const
{
    return p->to_timerange(p->timerange);
}

qreal
AVReader::fps() const
{
    return p->fps;
}

AVSmpteTime
AVReader::timecode() const
{
    return p->to_timecode(p->timecode);
}

AVReader::Error
AVReader::error() const
{
    return p->error;
}

QString
AVReader::error_message() const
{
    return p->errormessage;
}

AVMetadata
AVReader::metadata()
{
    return p->metadata;
}

AVSidecar
AVReader::sidecar()
{
    return p->sidecar;
}

void
AVReader::seek(AVTime time)
{
    p->seek(p->to_time(time));
}

void
AVReader::stream()
{
    p->stream();
}

void
AVReader::abort()
{
    p->streaming = false;
}

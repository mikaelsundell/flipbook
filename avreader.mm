// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avreader.h"
#include "avtimer.h"

#include <AVFoundation/AVFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <mach/mach_time.h>

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
        void init();
        void open();
        void close();
        void read();
        void drop();
        void seek(const AVTime& time);
        void stream();
    
    public:
        AVSmpteTime timecode() const
        {
            return AVSmpteTime::combine(timestamp, startcode.time());
        }
        CMTime to_time(const AVTime& other);
        CMTimeRange to_timerange(const AVTimeRange& other);
        AVTime to_time(const CMTime& other);
        AVTimeRange to_timerange(const CMTimeRange& other);
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* videooutput = nil;
        AVSmpteTime startcode;
        AVTimeRange timerange;
        AVTime timestamp;
        AVTime ptstamp;
        AVFps fps;
        QString filename;
        QString title;
        std::atomic<bool> loop;
        std::atomic<bool> everyframe;
        std::atomic<bool> streaming;
        AVMetadata metadata;
        AVSidecar sidecar;
        QString errormessage;
        AVReader::Error error;
        QPointer<AVReader> object;
};

AVReaderPrivate::AVReaderPrivate()
: loop(false)
, everyframe(false)
, streaming(false)
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
                    if (key == "title") { // todo: probably to simple but lets keep it for now
                        title = value;
                    }
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
    NSArray *formats = [videotrack formatDescriptions];
    for (id formatDesc in formats) {
        CMFormatDescriptionRef desc = (__bridge CMFormatDescriptionRef)formatDesc;
        CMMediaType mediaType = CMFormatDescriptionGetMediaType(desc);
        FourCharCode codecType = CMFormatDescriptionGetMediaSubType(desc);
        NSString* media = [NSString stringWithFormat:@"%c%c%c%c",
                                     (mediaType >> 24) & 0xFF,
                                     (mediaType >> 16) & 0xFF,
                                     (mediaType >> 8) & 0xFF,
                                     mediaType & 0xFF];

        NSString* codec = [NSString stringWithFormat:@"%c%c%c%c",
                                     (codecType >> 24) & 0xFF,
                                     (codecType >> 16) & 0xFF,
                                     (codecType >> 8) & 0xFF,
                                     codecType & 0xFF];

        metadata.add_pair("media type", QString::fromNSString(media));
        metadata.add_pair("codec type", QString::fromNSString(codec));
    }
    CMTime minduration = videotrack.minFrameDuration; // skip nominal frame rate for precision
    qreal duration = static_cast<qreal>(minduration.value) / minduration.timescale;
    fps = AVFps::guess(1.0 / duration);
    timerange = AVTimeRange::scale(to_timerange(videotrack.timeRange));
    timestamp = timerange.start();
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
                        bool dropframes = flags & kCMTimeCodeFlag_DropFrame;
                        AVFps startfps = AVFps::guess(framequanta);
                        Q_ASSERT("frame quanta does not match" && framequanta == startfps.frame_quanta());
                        if (dropframes) {
                            if (startfps == AVFps::fps_24()) {
                                startfps = AVFps::fps_23_976();
                            }
                            else if (startfps == AVFps::fps_30()) {
                                startfps = AVFps::fps_29_97();
                            }
                            else if (startfps == AVFps::fps_48()) {
                                startfps = AVFps::fps_47_952();
                            }
                            else if (startfps == AVFps::fps_60()) {
                                startfps = AVFps::fps_59_94();
                            }
                        }
                        Q_ASSERT("drop frames does not match" && dropframes == startfps.drop_frame());
                        if (type == kCMTimeCodeFormatType_TimeCode32) {
                            qint64 frame = static_cast<qint64>(EndianS32_BtoN(*reinterpret_cast<int32_t*>(data))); // 32-bit little-endian to native
                            startcode = AVSmpteTime(AVTime(frame, startfps));
                        }
                        else if (type == kCMTimeCodeFormatType_TimeCode64) {
                            qint64 frame = static_cast<qint64>(EndianS64_BtoN(*reinterpret_cast<int64_t*>(data))); // 64-bit big-endian to native
                            startcode = AVSmpteTime(AVTime(frame, startfps));
                        }
                        else {
                            Q_ASSERT("No valid type found for format description" && false);
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
    startcode = AVSmpteTime();
    timerange = AVTimeRange();
    timestamp = AVTime();
    ptstamp = AVTime();
    fps = AVFps();
    title.clear();
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
    ptstamp = AVTime::scale(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)));
    Q_ASSERT("read timestamp and ptstamp does not match" && timestamp == ptstamp);
    CFRelease(samplebuffer);
    object->video_changed(image.copy());
}

void
AVReaderPrivate::drop()
{
    Q_ASSERT(reader || reader.status != AVAssetReaderStatusReading);

    CMSampleBufferRef samplebuffer = [videooutput copyNextSampleBuffer];
    ptstamp = AVTime::scale(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)));
    Q_ASSERT("drop timestamp and ptstamp does not match" && timestamp == ptstamp);
    CFRelease(samplebuffer);
}

void
AVReaderPrivate::seek(const AVTime& time)
{
    Q_ASSERT(reader || reader.status != AVAssetReaderStatusReading);
    Q_ASSERT("ticks are not aligned" && time.ticks() == time.align(time.ticks()));

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
    timestamp = timerange.bound(time, loop);
    AVTime duration = timerange.end() - timestamp;
    reader.timeRange = CMTimeRangeMake(to_time(timestamp), to_time(duration));
    if (![reader startReading]) {
        error = AVReader::API_ERROR;
        errormessage = "failed to start reading after seeking";
        qWarning() << "warning: " << errormessage;
        return;
    }
    object->time_changed(timestamp);
    object->timecode_changed(timecode());
}

void
AVReaderPrivate::stream()
{
    streaming = true;
    object->stream_changed(streaming);
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    
    AVTimer statstimer;
    statstimer.start();
    
    AVTimer fpstimer;
    fpstimer.start();
    
    qint64 frame = 0;
    qint64 ticks = 0;
    qint64 droppedframes = 0;
    qint64 fpsframes = 0;
    while (streaming) {
        qint64 start = timestamp.frames();
        qint64 duration = timerange.duration().frames();
        ticks = timestamp.ticks();
        seek(timestamp);
        statstimer.lap();

        AVTimer frametimer(fps);
        frametimer.start();
        for (frame = start; frame < duration; frame++) {
            if (!streaming) {
                break;
            }
            timestamp.set_ticks(timestamp.ticks(frame));
            read();
            
            object->time_changed(timestamp);
            object->timecode_changed(object->timecode());
            fpsframes++;
            frametimer.wait();
            
            while (!frametimer.next() && !everyframe) {
                frame++;
                fpsframes++;
                droppedframes++;
                timestamp.set_ticks(timestamp.ticks(frame));
                drop();
            }
            if (fpsframes % 10 == 0) {
                qreal actualfps = fpsframes / AVTimer::convert(fpstimer.elapsed(), AVTimer::Unit::SECONDS);
                object->actual_fps_changed(actualfps);
                fpstimer.restart();
                fpsframes = 0;
            }
        }
        if (!loop || !streaming) {
            break;
        }
        timestamp = timerange.start();
    }
    streaming = false;
    statstimer.stop();
    
    object->stream_changed(streaming);
    QThread::currentThread()->setPriority(QThread::NormalPriority);
    
    qreal elapsed = AVTimer::convert(statstimer.elapsed(), AVTimer::Unit::SECONDS);
    qreal expected = AVTime(timestamp.ticks() - ticks, timestamp.timescale(), timestamp.fps()).seconds();
    qreal deviation = elapsed - expected;
    
    qDebug() << "stats: "
             << "timestamp: " << timestamp.to_string() << "|"
             << "elapsed:" << elapsed << "seconds" << AVTime(elapsed, fps).to_string() << "|"
             << "expected:" << expected
             << "deviation:" << deviation << "msecs:" << deviation * 1000 << "%:" << (deviation / expected) * 100
             << "seek:" << AVTimer::convert(statstimer.laps().first(), AVTimer::Unit::SECONDS) * 1000
             << "| frames dropped:" << droppedframes;
}

CMTime
AVReaderPrivate::to_time(const AVTime& other) {
   return CMTimeMakeWithEpoch(other.ticks(), other.timescale(), 0); // default epoch and flags
}

AVTime
AVReaderPrivate::to_time(const CMTime& other) {
    Q_ASSERT("fps is not valid" && fps.valid());
    
    AVTime time;
    if (CMTIME_IS_VALID(other)) {
        time.set_ticks(other.value);
        time.set_timescale(other.timescale);
        time.set_fps(fps);
    }
    return time;
}

AVTimeRange
AVReaderPrivate::to_timerange(const CMTimeRange& timerange) {
    AVTimeRange range;
    if (CMTIMERANGE_IS_VALID(timerange)) {
        range.set_start(to_time(timerange.start));
        range.set_duration(to_time(timerange.duration));
    }
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

QString
AVReader::filename() const
{
    return p->filename;
}

QString
AVReader::title() const
{
    return p->title;
}

AVTime
AVReader::time() const
{
    return p->timestamp;
}

AVTimeRange
AVReader::range() const
{
    return p->timerange;
}

AVFps
AVReader::fps() const
{
    return p->fps;
}

bool
AVReader::loop() const
{
    return p->loop;
}

AVSmpteTime
AVReader::timecode() const
{
    return p->timecode();
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
AVReader::set_loop(bool loop)
{
    if (p->loop != loop) {
        p->loop = loop;
        loop_changed(loop);
    }
}

void
AVReader::set_everyframe(bool everyframe)
{
    if (p->everyframe != everyframe) {
        p->everyframe = everyframe;
        everyframe_changed(everyframe);
    }
}

void
AVReader::seek(const AVTime& time)
{
    p->seek(time);
}

void
AVReader::stream()
{
    p->stream();
}

void
AVReader::stop()
{
    p->streaming = false;
}

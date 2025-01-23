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
        CMTime to_time(const AVTime& other);
        CMTimeRange to_timerange(const AVTimeRange& other);
        AVTime to_time(const CMTime& other);
        AVTimeRange to_timerange(const CMTimeRange& other);
        struct Data
        {
            AVAsset* asset = nil;
            AVAssetReader* reader = nil;
            AVAssetReaderTrackOutput* videooutput = nil;
            AVTimeRange timerange;
            AVTimeRange iorange;
            AVTime startstamp;
            AVTime timestamp;
            AVTime ptstamp;
            AVFps fps;
            qint32 timescale;
            QString filename;
            QString title;
            std::atomic<bool> loop = false;
            std::atomic<bool> everyframe = false;
            std::atomic<bool> streaming = false;
            AVMetadata metadata;
            AVSidecar sidecar;
            QList<QString> extensions;
            QString errormessage;
            AVReader::Error error = AVReader::NO_ERROR;
        };
        Data d;
        QPointer<AVReader> object;
};

AVReaderPrivate::AVReaderPrivate()
{
    d.extensions = {
        "mov", "mp4", "m4v", "avi", "mkv", "flv", "mpg", "mpeg", "3gp", "3g2", "mxf", "wmv"
    };
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
    NSURL* url = [NSURL fileURLWithPath:d.filename.toNSString()];
    d.asset = [AVAsset assetWithURL:url];
    if (!d.asset) {
        d.error = AVReader::FILE_ERROR;
        d.errormessage = QString("unable to load asset from file: %1").arg(d.filename);
        qWarning() << "warning: " << d.errormessage;
        return false;
    }
    NSError* averror = nil;
    d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
    if (!d.reader) {
        d.error = AVReader::API_ERROR;
        d.errormessage = QString("unable to create AVAssetReader for video: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << d.errormessage;
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
        NSArray<AVMetadataItem *> *metadataForFormat = [d.asset metadataForFormat:format];
        if (metadataForFormat) {
            for (AVMetadataItem* item in metadataForFormat) {
                QString key = QString::fromNSString(item.commonKey);
                QString value = QString::fromNSString(item.value.description);
                if (!key.isEmpty() || !value.isEmpty()) {
                    if (key == "title") { // todo: probably to simple but lets keep it for now
                        d.title = value;
                    }
                    d.metadata.add_pair(key, value);
                }
            }
        }
    }
    AVAssetTrack* videotrack = [[d.asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!videotrack) {
        d.error = AVReader::API_ERROR;
        d.errormessage = QString("no video track found in file: %1").arg(d.filename);
        qWarning() << "warning: " << d.errormessage;
        return false;
    }
    NSArray* formats = [videotrack formatDescriptions];
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

        d.metadata.add_pair("media type", QString::fromNSString(media));
        d.metadata.add_pair("codec type", QString::fromNSString(codec));
    }
    CMTime minduration = videotrack.minFrameDuration; // skip nominal frame rate for precision
    qreal duration = static_cast<qreal>(minduration.value) / minduration.timescale;
    d.fps = AVFps::guess(1.0 / duration);
    d.timerange = AVTimeRange::convert(to_timerange(videotrack.timeRange), d.fps);
    d.timestamp = d.timerange.start();
    d.startstamp = d.timestamp;
    AVAssetTrack* timecodetrack = [[d.asset tracksWithMediaType:AVMediaTypeTimecode] firstObject];
    if (timecodetrack) {
        AVAssetReader* timecodereader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
        if (!d.reader) {
            d.error = AVReader::API_ERROR;
            d.errormessage = QString("unable to create AVAssetReader for timecode: %1").arg(QString::fromNSString(averror.localizedDescription));
            qWarning() << "warning: " << d.errormessage;
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
                        qint64 frame = 0;
                        Q_ASSERT("drop frames does not match" && dropframes == startfps.drop_frame());
                        if (type == kCMTimeCodeFormatType_TimeCode32) { // 32-bit little-endian to native
                            frame = static_cast<qint64>(EndianS32_BtoN(*reinterpret_cast<int32_t*>(data)));
                        }
                        else if (type == kCMTimeCodeFormatType_TimeCode64) { // 64-bit big-endian to native
                            frame = static_cast<qint64>(EndianS64_BtoN(*reinterpret_cast<int64_t*>(data)));
                        }
                        else {
                            Q_ASSERT("no valid type found for format description" && false);
                        }
                        if (frame) {
                            frame = AVSmpteTime::convert(frame, startfps, d.fps);
                            d.startstamp = AVTime::convert(AVTime(frame, d.fps), d.fps);
                        }
                    }
                    else {
                        d.error = AVReader::API_ERROR;
                        d.errormessage = QString("unable to get data from block buffer for timecode");
                        qWarning() << "warning: " << d.errormessage;
                        return false;
                    }
                }
            }
            if (samplebuffer) {
                CFRelease(samplebuffer);
            }
        }
        else {
            d.error = AVReader::API_ERROR;
            d.errormessage = QString("unable to read sample buffer at for timecode");
            qWarning() << "warning: " << d.errormessage;
            return false;
        }
    }
    d.videooutput = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:videotrack
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!d.videooutput) {
        d.error = AVReader::API_ERROR;
        d.errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << d.errormessage;
        return false;
    }
    [d.reader addOutput:d.videooutput];
    [d.reader startReading];
    object->opened(d.filename);
}

void
AVReaderPrivate::close()
{
    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }
    d.asset = nil;
    d.reader = nil;
    d.videooutput = nil;
    d.timerange = AVTimeRange();
    d.iorange = AVTimeRange();
    d.startstamp = AVTime();
    d.timestamp = AVTime();
    d.ptstamp = AVTime();
    d.fps = AVFps();
    d.timescale = 0;
    d.title = QString();
    d.loop = false;
    d.everyframe = false;
    d.streaming = false;
    d.metadata = AVMetadata();
    d.sidecar = AVSidecar();
    d.errormessage = QString();
    d.error = AVReader::NO_ERROR;
}

void
AVReaderPrivate::read()
{
    Q_ASSERT(d.reader || d.reader.status != AVAssetReaderStatusReading);
 
    CMSampleBufferRef samplebuffer = [d.videooutput copyNextSampleBuffer];
    if (!samplebuffer) {
        d.error = AVReader::API_ERROR;
        d.errormessage = "unable to read sample buffer at current frame";
        qWarning() << "warning: " << d.errormessage;
    }
    CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(samplebuffer);
    if (!imagebuffer) {
        CFRelease(samplebuffer);
        d.error = AVReader::API_ERROR;
        d.errormessage = "CMSampleBuffer has no image buffer";
        qWarning() << "warning: " << d.errormessage;
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
    d.ptstamp = AVTime::convert(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)), d.fps);
    Q_ASSERT("read timestamp and ptstamp does not match" && d.timestamp == d.ptstamp);
    CFRelease(samplebuffer);
    object->video_changed(image.copy());
}

void
AVReaderPrivate::drop()
{
    Q_ASSERT(d.reader || d.reader.status != AVAssetReaderStatusReading);
    
    CMSampleBufferRef samplebuffer = [d.videooutput copyNextSampleBuffer];
    d.ptstamp = AVTime::convert(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)), d.fps);
    CFRelease(samplebuffer);
}

void
AVReaderPrivate::seek(const AVTime& time)
{
    Q_ASSERT(d.reader || d.reader.status != AVAssetReaderStatusReading);
    Q_ASSERT("ticks are not aligned" && time.ticks() == time.align(time.ticks()));

    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }
    NSError* averror = nil;
    d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
    if (!d.reader) {
        d.error = AVReader::API_ERROR;
        d.errormessage = QString("failed to recreate AVAssetReader: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << d.errormessage;
        return;
    }
    AVAssetTrack* track = [[d.asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        d.error = AVReader::API_ERROR;
        d.errormessage = "no video track found in asset";
        qWarning() << "warning: " << d.errormessage;
        return;
    }
    d.videooutput = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!d.videooutput) {
        d.error = AVReader::API_ERROR;
        d.errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << d.errormessage;
        return;
    }
    [d.reader addOutput:d.videooutput];
    d.timestamp = d.timerange.bound(time, d.loop);
    AVTime duration = d.timerange.end() - d.timestamp;
    d.reader.timeRange = CMTimeRangeMake(to_time(d.timestamp), to_time(duration));
    if (![d.reader startReading]) {
        d.error = AVReader::API_ERROR;
        d.errormessage = "failed to start reading after seeking";
        qWarning() << "warning: " << d.errormessage;
        return;
    }
    object->time_changed(d.timestamp);
    object->timecode_changed(d.startstamp + d.timestamp);
}

void
AVReaderPrivate::stream()
{
    d.streaming = true;
    object->stream_changed(d.streaming);
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    AVTimer statstimer;
    statstimer.start();
    
    AVTimer fpstimer;
    fpstimer.start();
    
    qint64 frame = 0;
    qint64 ticks = 0;
    qint64 droppedframes = 0;
    qint64 fpsframes = 0;
    while (d.streaming) {
        qint64 start = d.timestamp.frames();
        qint64 duration = d.timerange.duration().frames();
        ticks = d.timestamp.ticks();
        seek(d.timestamp);
        statstimer.lap();

        AVTimer frametimer;
        frametimer.start(d.fps);
        for (frame = start; frame < duration; frame++) {
            if (!d.streaming) {
                break;
            }
            d.timestamp.set_ticks(d.timestamp.ticks(frame));
            read();
            
            object->time_changed(d.timestamp);
            object->timecode_changed(d.startstamp + d.timestamp);
            fpsframes++;
            frametimer.wait();
            while (!frametimer.next(d.fps) && !d.everyframe) {
                frame++;
                fpsframes++;
                droppedframes++;
                d.timestamp.set_ticks(d.timestamp.ticks(frame));
                drop();
            }
            if (fpsframes % 10 == 0) {
                qreal actualfps = fpsframes / AVTimer::convert(fpstimer.elapsed(), AVTimer::Unit::SECONDS);
                object->actualfps_changed(actualfps);
                fpstimer.restart();
                fpsframes = 0;
            }
        }
        if (!d.loop || !d.streaming) {
            break;
        }
        d.timestamp = d.timerange.start();
    }
    d.streaming = false;
    statstimer.stop();
    
    object->stream_changed(d.streaming);
    QThread::currentThread()->setPriority(QThread::NormalPriority);
    
    qreal elapsed = AVTimer::convert(statstimer.elapsed(), AVTimer::Unit::SECONDS);
    qreal expected = AVTime(d.timestamp, d.timestamp.ticks() - ticks).seconds();
    qreal deviation = elapsed - expected;
    
    qDebug() << "stats: "
             << "timestamp: " << d.timestamp.to_string() << "|"
             << "elapsed:" << elapsed << "seconds" << AVTime(elapsed, d.fps).to_string() << "|"
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
    Q_ASSERT("fps is not valid" && d.fps.valid());
    
    AVTime time;
    if (CMTIME_IS_VALID(other)) {
        time.set_ticks(other.value);
        time.set_timescale(other.timescale);
        time.set_fps(d.fps);
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
    p->d.filename = filename;
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
    return (p->d.reader && p->d.error == AVReader::NO_ERROR);
}

bool
AVReader::is_closed() const
{
    return !p->d.reader;
}

bool
AVReader::is_streaming() const
{
    return p->d.streaming;
}

bool
AVReader::is_supported(const QString& extension) const
{
    return p->d.extensions.contains(extension.toLower());
}

QString
AVReader::filename() const
{
    return p->d.filename;
}

QString
AVReader::title() const
{
    return p->d.title;
}

AVTimeRange
AVReader::range() const
{
    return p->d.timerange;
}

AVTimeRange
AVReader::io() const
{
    return p->d.iorange;
}

AVTime
AVReader::start() const
{
    return p->d.startstamp;
}

AVTime
AVReader::time() const
{
    return p->d.timestamp;
}

AVFps
AVReader::fps() const
{
    return p->d.fps;
}

bool
AVReader::loop() const
{
    return p->d.loop;
}

QList<QString>
AVReader::extensions() const
{
    return p->d.extensions;
}

AVMetadata
AVReader::metadata()
{
    return p->d.metadata;
}

AVSidecar
AVReader::sidecar()
{
    return p->d.sidecar;
}

AVReader::Error
AVReader::error() const
{
    return p->d.error;
}

QString
AVReader::error_message() const
{
    return p->d.errormessage;
}

void
AVReader::set_loop(bool loop)
{
    if (p->d.loop != loop) {
        p->d.loop = loop;
        loop_changed(loop);
    }
}

void
AVReader::set_io(const AVTimeRange& io)
{
    if (p->d.iorange != io) {
        p->d.iorange = io;
        io_changed(io);
    }
}

void
AVReader::set_everyframe(bool everyframe)
{
    if (p->d.everyframe != everyframe) {
        p->d.everyframe = everyframe;
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
    p->d.streaming = false;
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "qt_stream.h"
#include <AVFoundation/AVFoundation.h>

#include <QDebug>

class qt_stream_private
{
    public:
        qt_stream_private();
        void init();
        bool open();
        void close();
        QImage fetch();
        void seek(int frame);
        int frame() const;
        int start() const;
        int end() const;

    public:
        float seconds(CMTime time) const {
            return CMTimeGetSeconds(time); // time.value / time.timescale, epoch = 0
        }
        QString filename;
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* output = nil;
        CMTimeRange timerange;
        CMTime timestamp;
        float fps = 0;
};

qt_stream_private::qt_stream_private()
{
}

void
qt_stream_private::init()
{
}

bool
qt_stream_private::open()
{
    close();
    NSURL* url = [NSURL fileURLWithPath:filename.toNSString()];
    asset = [AVAsset assetWithURL:url];
    if (!asset) {
        qWarning() << "warning: Unable to load asset from file:" << filename;
        return false;
    }
    NSError* error = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
    if (!reader) {
        qWarning() << "warning: unable to create AVAssetReader:" << error.localizedDescription;
        return false;
    }
    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        qWarning() << "warning: no video track found in file:" << filename;
        return false;
    }
    timerange = track.timeRange;
    fps = track.nominalFrameRate;
    output = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!output) {
        qWarning() << "warning: unable to create AVAssetReaderTrackOutput.";
        return false;
    }
    [reader addOutput:output];
    [reader startReading];
    return true;
}

void
qt_stream_private::close()
{
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    output = nil;
    asset = nil;
    fps = 0;
}

QImage
qt_stream_private::fetch()
{
    if (!reader || reader.status != AVAssetReaderStatusReading) {
        qWarning() << "error: AVAssetReader is not in a valid state.";
        return QImage();
    }
    CMSampleBufferRef samplebuffer = [output copyNextSampleBuffer];
    if (!samplebuffer) {
        qWarning() << "error: Unable to read sample buffer at current frame.";
        return QImage();
    }
    CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(samplebuffer);
    if (!imagebuffer) {
        CFRelease(samplebuffer);
        qWarning() << "error: CMSampleBuffer has no image buffer.";
        return QImage();
    }
    CVPixelBufferLockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);
    void* baseAddress = CVPixelBufferGetBaseAddress(imagebuffer);
    size_t width = CVPixelBufferGetWidth(imagebuffer);
    size_t height = CVPixelBufferGetHeight(imagebuffer);
    size_t bytes = CVPixelBufferGetBytesPerRow(imagebuffer);
    QImage img(static_cast<uchar*>(baseAddress),
               static_cast<int>(width),
               static_cast<int>(height),
               static_cast<int>(bytes),
               QImage::Format_ARGB32);
    CVPixelBufferUnlockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);
    timestamp = CMSampleBufferGetPresentationTimeStamp(samplebuffer);
    CFRelease(samplebuffer);
    return img.copy();
}

void
qt_stream_private::seek(int frame)
{
    if (!asset) {
        qWarning() << "error: asset is not loaded.";
        return;
    }
    CMTime time = CMTimeMake(frame, fps);
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    NSError* error = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
    if (!reader) {
        qWarning() << "error: failed to recreate AVAssetReader:" << error.localizedDescription;
        return;
    }
    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        qWarning() << "error: no video track found in asset.";
        return;
    }
    output = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!output) {
        qWarning() << "error: unable to create AVAssetReaderTrackOutput.";
        return;
    }
    [reader addOutput:output];
    reader.timeRange = CMTimeRangeMake(time, kCMTimePositiveInfinity);
    if (![reader startReading]) {
        qWarning() << "error: failed to start reading after seeking.";
        return;
    }
}

int
qt_stream_private::frame() const
{
    return static_cast<int>(seconds(timestamp) * fps);
}

int
qt_stream_private::start() const
{
    return static_cast<int>(seconds(timerange.start) * fps);
}

int
qt_stream_private::end() const
{
    return static_cast<int>((seconds(timerange.start) + seconds(timerange.duration)) * fps);
}

qt_stream::qt_stream()
: p(new qt_stream_private())
{
    p->init();
}

qt_stream::~qt_stream()
{
}

bool
qt_stream::is_open() const
{
    return p->filename.size();
}

bool
qt_stream::open(const QString& filename)
{
    p->filename = filename;
    return p->open();
}

QString
qt_stream::filename() const
{
    return p->filename;
}

void
qt_stream::close()
{
}

QImage
qt_stream::fetch()
{
    return p->fetch();
}

void
qt_stream::seek(int frame)
{
    return p->seek(frame);
}

int
qt_stream::frame() const
{
    return p->frame();
}

int
qt_stream::start() const
{
    return p->start();
}

int
qt_stream::end() const
{
    return p->end();
}

float
qt_stream::fps()
{
    return p->fps;
}

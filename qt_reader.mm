// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "qt_reader.h"
#include <AVFoundation/AVFoundation.h>

#include <QDebug>

class qt_reader_private
{
    public:
        qt_reader_private();
        void init();
        bool open();
        void close();
        QImage frame(int frame);
        int start() const;
        int end() const;
        int fps() const;

    public:
        QString filename;
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* output = nil;
        int framecount = 0;
        int framerate = 0;

};

qt_reader_private::qt_reader_private()
{
}

void
qt_reader_private::init()
{
}

bool
qt_reader_private::open()
{
    close();
    NSURL* url = [NSURL fileURLWithPath:filename.toNSString()];
    asset = [AVAsset assetWithURL:url];
    if (!asset) {
        qWarning() << "error: Unable to load asset from file:" << filename;
        return false;
    }
    NSError* error = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
    if (!reader) {
        qWarning() << "error: unable to create AVAssetReader:" << error.localizedDescription;
        return false;
    }

    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        qWarning() << "error: no video track found in file:" << filename;
        return false;
    }
    framerate = track.nominalFrameRate;
    framecount = CMTimeGetSeconds(track.timeRange.duration) * framerate;
    
    output = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    
    if (!output) {
        qWarning() << "error: unable to create AVAssetReaderTrackOutput.";
        return false;
    }

    [reader addOutput:output];
    [reader startReading];
    return true;
}

void
qt_reader_private::close()
{
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    output = nil;
    asset = nil;
    framecount = 0;
    framerate = 0;
}

QImage qt_reader_private::frame(int frame)
{
    if (!reader || reader.status != AVAssetReaderStatusReading) {
        qWarning() << "error: AVAssetReader is not in a valid state.";
        return QImage();
    }
    for (int i = 0; i < frame; ++i) {
        CMSampleBufferRef sampleBuffer = [output copyNextSampleBuffer];
        if (!sampleBuffer) {
            qWarning() << "error: unable to read next sample buffer.";
            return QImage();
        }
        CFRelease(sampleBuffer);
    }
    CMSampleBufferRef sampleBuffer = [output copyNextSampleBuffer];
    if (!sampleBuffer) {
        qWarning() << "error: unable to read next sample buffer.";
        return QImage();
    }

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) {
        CFRelease(sampleBuffer);
        qWarning() << "error: CMSampleBuffer has no image buffer.";
        return QImage();
    }
    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    void* baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
    size_t width = CVPixelBufferGetWidth(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    QImage img((uchar*)baseAddress, static_cast<int>(width), static_cast<int>(height), static_cast<int>(bytesPerRow), QImage::Format_ARGB32);
    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    CFRelease(sampleBuffer);
    return img.copy();
}

int
qt_reader_private::start() const
{
    return 0;
}

int
qt_reader_private::end() const
{
    return framecount;
}

int
qt_reader_private::fps() const
{
    return framerate;
}

qt_reader::qt_reader()
: p(new qt_reader_private())
{
    p->init();
}

qt_reader::~qt_reader()
{
}

bool
qt_reader::open(const QString& filename)
{
    p->filename = filename;
    return p->open();
}

QString
qt_reader::filename() const
{
    return p->filename;
}

void
qt_reader::close()
{
}

QImage
qt_reader::frame(int frame)
{
    return p->frame(frame);
}

int
qt_reader::start() const
{
    return p->start();
}

int
qt_reader::end() const
{
    return p->end();
}

int
qt_reader::fps()
{
    return p->fps();
}

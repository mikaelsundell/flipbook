// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avstream.h"
#include <AVFoundation/AVFoundation.h>

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
        void free();
        void open();
        void read();
    
        qint64 position() const;
        qint64 duration() const;

        int frame() const;
        int start_frame() const;
        int end_frame() const;
    
        void seek_frame(int frame);
        void seek_position(qint64 position);
        void seek_time(CMTime time);
        void start();

    public:
        // todo: inline functions will be removed
        double seconds(CMTime time) const {
            return CMTimeGetSeconds(time); // time.value / time.timescale, epoch = 0
        }
        void text(QImage& image, int frame) {
            QPainter painter(&image);
            QFont font("Arial", 100, QFont::Bold);
            painter.setFont(font);
            painter.setPen(QColor(Qt::white));
            QString text = QString::number(frame);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(text);
            int textHeight = fm.height();
            QPoint center((image.width() - textWidth) / 2, (image.height() + textHeight) / 2);
            painter.drawText(center, text);
            painter.end();
        }

        QThread* thread;
    
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* output = nil;
    
        QString filename;
        CMTimeRange timerange;
        CMTime timestamp;
    
        int inframe = 0;
        int outframe = 0;
        float rate = 0.0;
    
        std::atomic<bool> abort;
        std::atomic<bool> stream;
        std::atomic<bool> loop;
    
        QString errormessage;
        AVStream::Error error;
    
        QPointer<AVStream> object;
};

AVStreamPrivate::AVStreamPrivate()
: abort(false)
, stream(false)
, loop(false)
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
AVStreamPrivate::free()
{
    if (reader) {
        [reader cancelReading];
        reader = nil;
    }
    asset = nil;
    reader = nil;
    output = nil;
    timerange = CMTimeRange();
    timestamp = CMTime();
    stream = false;
    abort = false;
    inframe = 0;
    outframe = 0;
    rate = 0;
    error = AVStream::NO_ERROR;
    errormessage = QString();
}

void
AVStreamPrivate::open()
{
    qDebug() << " - open()";
    
    free();
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
        errormessage = QString("unable to create AVAssetReader: %1").arg(QString::fromNSString(averror.localizedDescription));
        qWarning() << "warning: " << errormessage;
        return false;
    }
    AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        error = AVStream::API_ERROR;
        errormessage = QString("no video track found in file: %1").arg(filename);
        qWarning() << "warning: " << errormessage;
        return false;
    }
    timerange = track.timeRange;
    rate = track.nominalFrameRate;
    output = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!output) {
        error = AVStream::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return false;
    }
    [reader addOutput:output];
    [reader startReading];
    object->opened(filename);
}

void
AVStreamPrivate::read()
{
    qDebug() << " - read() frame: " << frame();
    if (!reader && error == AVStream::NO_ERROR) {
        open();
    }
    if (!reader || reader.status != AVAssetReaderStatusReading) {
        error = AVStream::API_ERROR;
        errormessage = "AVAssetReader is not in a valid state";
        qWarning() << "warning: " << errormessage;
        return;
    }
    CMSampleBufferRef samplebuffer = [output copyNextSampleBuffer];
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
    timestamp = CMSampleBufferGetPresentationTimeStamp(samplebuffer);
    CFRelease(samplebuffer);
    text(image, frame()); // todo: temporary
    object->image_changed(image.copy());
}

qint64
AVStreamPrivate::position() const
{
    return timestamp.value / timestamp.timescale;
}

qint64
AVStreamPrivate::duration() const
{
    return timerange.duration.value / timerange.duration.timescale;
}

int
AVStreamPrivate::frame() const
{
    return static_cast<int>(seconds(timestamp) * rate);
}

int
AVStreamPrivate::start_frame() const
{
    return static_cast<int>(seconds(timerange.start) * rate);
}

int
AVStreamPrivate::end_frame() const
{
    return static_cast<int>((seconds(timerange.start) + seconds(timerange.duration)) * rate);
}

void
AVStreamPrivate::seek_frame(int frame)
{
    seek_time(CMTimeMake(frame, rate));
}

void
AVStreamPrivate::seek_position(qint64 position)
{
    double sec = static_cast<double>(position) / static_cast<double>(timestamp.timescale);
    seek_time(CMTimeMake(static_cast<int>(qRound(sec * rate)), rate));
}

void
AVStreamPrivate::seek_time(CMTime time)
{
    if (!reader && error == AVStream::NO_ERROR) {
        open();
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
    output = [[AVAssetReaderTrackOutput alloc]
        initWithTrack:track
        outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!output) {
        error = AVStream::API_ERROR;
        errormessage = "unable to create AVAssetReaderTrackOutput";
        qWarning() << "warning: " << errormessage;
        return;
    }
    [reader addOutput:output];
    reader.timeRange = CMTimeRangeMake(time, kCMTimePositiveInfinity);
    if (![reader startReading]) {
        error = AVStream::API_ERROR;
        errormessage = "failed to start reading after seeking";
        qWarning() << "warning: " << errormessage;
        return;
    }
}

void
AVStreamPrivate::start()
{
    if (!stream) {
        read();
    }
    else {
        for(int i=frame(); i<end_frame(); i++) {
            if (!abort) {
                read();
                object->frame_changed(i);
                QThread::msleep((1 / rate) * 1000);
            }
            else {
                return;
            }
        }
    }
    object->finished();
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
AVStream::free()
{
    p->free();
}

bool
AVStream::is_open() const
{
    return (p->reader && p->error == AVStream::NO_ERROR);
}

QString
AVStream::filename() const
{
    return p->filename;
}

qint64
AVStream::position() const
{
    return p->position();
}

qint64
AVStream::duration() const
{
    return p->duration();
}

int
AVStream::frame() const
{
    return p->frame();
}

int
AVStream::start_frame() const
{
    return p->start_frame();
}

int
AVStream::end_frame() const
{
    return p->end_frame();
}

int
AVStream::in_frame() const
{
    return p->inframe;
}

int
AVStream::out_frame() const
{
    return p->outframe;
}

bool
AVStream::loop() const
{
    return p->loop;
}

float
AVStream::rate() const
{
    return p->rate;
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

AVMetadata*
AVStream::metadata()
{
    return new AVMetadata(); // todo: temporary
}

AVSidecar*
AVStream::sidecar()
{
    return new AVSidecar(); // todo: temporary
}

void
AVStream::set_filename(const QString &filename)
{
    if (p->filename != filename) {
        p->filename = filename;
    }
}

void
AVStream::set_position(qint64 position)
{
    if (p->position() != position) {
        p->seek_position(position);
    }
}

void
AVStream::set_frame(int frame)
{
    if (p->frame() != frame) {
        p->seek_frame(frame);
    }
}

void
AVStream::set_in_frame(int in_frame)
{
    if (p->inframe != in_frame) {
        p->inframe = in_frame;
    }
}

void
AVStream::set_out_frame(int out_frame)
{
    if (p->outframe != out_frame) {
        p->outframe = out_frame;
    }
}

void
AVStream::set_loop(bool loop)
{
    if (p->loop != loop) {
        p->loop = loop;
    }
}

void
AVStream::set_stream(bool stream)
{
    if (p->stream != stream) {
        p->stream = stream;
    }
}

void
AVStream::start()
{
    qDebug() << "start(): begin";
    p->start();
    qDebug() << "start(): end";
}

void
AVStream::stop()
{
    p->abort = true;
}

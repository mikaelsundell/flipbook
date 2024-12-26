// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avmetadata.h"
#include "avsidecar.h"
#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"

#include <QImage>
#include <QObject>
#include <QScopedPointer>

class AVReaderPrivate;
class AVReader : public QObject {
    Q_OBJECT
    public:
        enum Error { NO_ERROR, FILE_ERROR, API_ERROR, OTHER_ERROR };
        Q_ENUM(Error)

    public:
        AVReader();
        virtual ~AVReader();
        void open(const QString& filename);
        void read();
        void close();
        bool is_open() const;
        bool is_closed() const;
        bool is_streaming() const;
        QString filename() const;
        QString title() const;
        AVTimeRange range() const;
        AVTime time() const;
        qreal fps() const;
        bool loop() const;
        AVSmpteTime timecode() const;
        AVMetadata metadata();
        AVSidecar sidecar();
        AVReader::Error error() const;
        QString error_message() const;
    
    public Q_SLOTS:
        void set_loop(bool loop);
    
        void seek(const AVTime& time);
        void stream();
        void stop();

    Q_SIGNALS:
        void opened(const QString& filename);
        void time_changed(const AVTime& time);
        void range_changed(const AVTimeRange& timerange);
        void video_changed(const QImage& image);
        void audio_changed(const QByteArray& buffer);
        void loop_changed(bool looping);
        void stream_changed(bool streaming);

    private:
        QScopedPointer<AVReaderPrivate> p;
};

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

class AVStreamPrivate;
class AVStream : public QObject {
    Q_OBJECT
    public:
        enum Error { NO_ERROR, FILE_ERROR, API_ERROR, OTHER_ERROR };
        Q_ENUM(Error)

    public:
        AVStream();
        virtual ~AVStream();
        void open(const QString& filename);
        void read();
        void close();
        bool is_open() const;
        bool is_closed() const;
        const QString& filename() const;
        AVTimeRange range() const;
        AVTime time() const;
        float rate() const;
        AVSmpteTime timecode() const;
        AVMetadata metadata();
        AVSidecar sidecar();
        AVStream::Error error() const;
        QString error_message() const;
    
    public Q_SLOTS:
        void seek(AVTime time);
        void play();
        void stop();

    Q_SIGNALS:
        void time_changed(AVTime frame);
        void range_changed(AVTimeRange frame);
        void image_changed(const QImage& image);
        void opened(const QString& filename);
        void started();
        void finished();

    private:
        QScopedPointer<AVStreamPrivate> p;
};

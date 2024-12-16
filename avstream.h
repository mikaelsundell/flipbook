// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avmetadata.h"
#include "avsidecar.h"

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
        void free();
        bool is_open() const;
        QString filename() const;
        qint64 position() const;
        qint64 duration() const;
        int frame() const;
        int start_frame() const;
        int end_frame() const;
        int in_frame() const;
        int out_frame() const;
        bool loop() const;
        float rate() const;
        AVStream::Error error() const;
        QString error_message() const;

    public:
        AVMetadata* metadata();
        AVSidecar* sidecar();

    public Q_SLOTS:
        void set_filename(const QString& filename);
        void set_position(qint64 position);
        void set_frame(int frame);
        void set_in_frame(int in_frame);
        void set_out_frame(int out_frame);
        void set_loop(bool loop);
        void set_stream(bool stream);
        void start();
        void stop();

    Q_SIGNALS:
        void frame_changed(int frame);
        void in_changed(int frame_in);
        void out_changed(int frame_out);
        void image_changed(const QImage& image);
        void opened(const QString& filename);
        void started();
        void finished();

    private:
        QScopedPointer<AVStreamPrivate> p;
};

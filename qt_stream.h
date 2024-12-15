// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "qt_sidecar.h"

#include <QImage>
#include <QScopedPointer>

class qt_stream_private;
class qt_stream
{
    public:
        qt_stream();
        virtual ~qt_stream();
        bool is_open() const;
        qt_sidecar sidecar();
        QString filename() const;
        bool open(const QString& filename);
        void close();
        QImage fetch();
        void seek(int frame);
        int frame() const;
        int start() const;
        int end() const;
        float fps();

    private:
        QScopedPointer<qt_stream_private> p;
};

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "qt_sidecar.h"

#include <QImage>
#include <QScopedPointer>

class qt_reader_private;
class qt_reader
{
    public:
        qt_reader();
        virtual ~qt_reader();
        bool is_open() const;
        QString filename() const;
        bool open(const QString& filename);
        void close();
        QImage frame(int frame);
        qt_sidecar sidecar();
        int start() const;
        int end() const;
        float fps();

    private:
        QScopedPointer<qt_reader_private> p;
};

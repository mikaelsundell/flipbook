
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
//

#pragma once

#include <QImage>
#include <QScopedPointer>

class qt_reader_private;
class qt_reader
{
    public:
        qt_reader();
        virtual ~qt_reader();
        QString filename() const;
        bool open(const QString& filename);
        void close();
        QImage frame(int frame);
        int start() const;
        int end() const;
        int fps();

    private:
        QScopedPointer<qt_reader_private> p;
};

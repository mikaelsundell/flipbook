// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avfps.h"

#include <QObject>
#include <QScopedPointer>

class AVTimerPrivate;
class AVTimer : public QObject {
    Q_OBJECT
    public:
        enum Unit { NANOS, SECONDS, MINUTES, HOURS };
        Q_ENUM(Unit)
    public:
        AVTimer();
        AVTimer(const AVFps& fps);
        virtual ~AVTimer();
        void start();
        void stop();
        void restart();
        bool next();
        void wait();
        void sleep(quint64 msecs);
        AVFps fps() const;
        quint64 elapsed() const;

        void set_fps(const AVFps& fps);
    
        static qreal convert(quint64 nano, Unit unit = Unit::NANOS);

        
    private:
        QScopedPointer<AVTimerPrivate> p;
};

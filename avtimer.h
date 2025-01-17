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
        virtual ~AVTimer();
        void start();
        void start(const AVFps& fps);
        void stop();
        void restart();
        void lap();
        bool next(const AVFps& fps);
        void wait();
        void sleep(quint64 msecs);
        quint64 elapsed() const;
        QList<quint64> laps() const;
    
        static qreal convert(quint64 nano, Unit unit = Unit::NANOS);

    private:
        QScopedPointer<AVTimerPrivate> p;
};

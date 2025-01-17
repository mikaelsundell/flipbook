// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avfps.h"

#include <QExplicitlySharedDataPointer>

class AVTimePrivate;
class AVTime
{
    public:
        AVTime();
        AVTime(qint64 ticks, qint32 timescale, const AVFps& fps);
        AVTime(qint64 frame, const AVFps& fps);
        AVTime(qreal seconds, const AVFps& fps);
        AVTime(const AVTime& other, const AVFps& fps);
        AVTime(const AVTime& other);
        ~AVTime();
        AVFps fps() const;
        qint64 ticks() const;
        qint64 ticks(qint64 frame) const;
        qint32 timescale() const;
        qint64 tpf() const;
        qint64 frame(qint64 ticks) const;
        qint64 frames() const;
        qint64 align(qint64 ticks) const;
        qreal seconds() const;
        QString to_string(qint64 ticks) const;
        QString to_string() const;
        void invalidate();
        bool valid() const;
    
        void set_ticks(qint64 ticks);
        void set_timescale(qint32 timescale);
        void set_fps(const AVFps& fps);
    
        AVTime& operator=(const AVTime& other);
        bool operator==(const AVTime& other) const;
        bool operator!=(const AVTime& other) const;
        bool operator<(const AVTime& other) const;
        bool operator>(const AVTime& other) const;
        bool operator<=(const AVTime& other) const;
        bool operator>=(const AVTime& other) const;
        AVTime operator+(const AVTime& other) const;
        AVTime operator-(const AVTime& other) const;
    
        static AVTime convert(const AVTime& time, const AVFps& from, const AVFps& to);
        static AVTime timescale(const AVTime& time, const AVFps& to);
        static AVTime timescale(const AVTime& time, qint32 timescale = 24000);

    private:
        QExplicitlySharedDataPointer<AVTimePrivate> p;
};

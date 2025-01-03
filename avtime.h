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
        AVTime(qint64 ticks, qint32 timescale);
        AVTime(qint64 frame, const AVFps& fps);
        AVTime(const AVTime& other);
        ~AVTime();
        qint64 ticks() const;
        qint64 ticks(const AVFps& fps) const;
        qint64 ticks(qint64 frame, const AVFps& fps) const;
        qint32 timescale() const;
        qint64 tpf(const AVFps& fps) const;
        qint64 frame(const AVFps& fps) const;
        QString to_string() const;
        bool valid() const;
    
        void set_ticks(qint64 ticks);
        void set_timescale(qint32 timescale);
    
        AVTime& operator=(const AVTime& other);
        bool operator==(const AVTime& other) const;
        bool operator!=(const AVTime& other) const;
        bool operator<(const AVTime& other) const;
        bool operator>(const AVTime& other) const;
        bool operator<=(const AVTime& other) const;
        bool operator>=(const AVTime& other) const;
        AVTime operator+(const AVTime& other) const;
        AVTime operator-(const AVTime& other) const;
    
        static AVTime scale(AVTime time, qint32 timescale = 24000);

    private:
        QExplicitlySharedDataPointer<AVTimePrivate> p;
};

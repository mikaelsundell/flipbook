// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avtime.h"
#include "avfps.h"

#include <QExplicitlySharedDataPointer>

class AVSmpteTimePrivate;
class AVSmpteTime {
    public:
        AVSmpteTime();
        AVSmpteTime(const AVTime& time);
        AVSmpteTime(const AVSmpteTime& other);
        ~AVSmpteTime();
        quint32 counter() const;
        qint16 hours() const;
        qint16 minutes() const;
        qint16 seconds() const;
        qint16 frames() const;
        qint16 subframes() const;
        qint16 subframe_divisor() const;
        qint64 frame() const;
        AVTime time() const;
        bool allow_negatives() const;
        bool max_24hours() const;
        QString to_string() const;
        bool valid() const;
        void set_time(const AVTime& time);
        void set_allow_negatives(bool allow_negatives);
        void set_max24hours(bool max_24hours);
    
        AVSmpteTime& operator=(const AVSmpteTime& other);
        bool operator==(const AVSmpteTime& other) const;
        bool operator!=(const AVSmpteTime& other) const;
        bool operator<(const AVSmpteTime& other) const;
        bool operator>(const AVSmpteTime& other) const;
        bool operator<=(const AVSmpteTime& other) const;
        bool operator>=(const AVSmpteTime& other) const;
        AVSmpteTime operator+(const AVSmpteTime& other) const;
        AVSmpteTime operator-(const AVSmpteTime& other) const;
    
        static qint64 dropframe(quint64 frame, const AVFps& fps, bool inverse = false);
        static qint64 frame(quint16 hours, quint16 minutes, quint16 seconds, quint16 frames, const AVFps& fps);

    private:
        QExplicitlySharedDataPointer<AVSmpteTimePrivate> p;
};


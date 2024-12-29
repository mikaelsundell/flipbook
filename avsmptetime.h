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
        AVSmpteTime(const AVTime& time, const AVFps& fps);
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
        AVFps fps() const;
        AVTime time() const;
        bool allow_negatives() const;
        bool max_24hours() const;
        AVTime to_time() const;
        QString to_string() const;
        bool valid() const;
    
        void set_counter(const quint32 counter);
        void set_hours(qint16 hours);
        void set_minutes(qint16 minutes);
        void set_seconds(qint16 seconds);
        void set_frames(qint16 frames);
        void set_subframes(qint16 subframes);
        void set_subframe_divisor(qint16 subframe_divisor);
        void set_time(const AVTime& time, const AVFps& fps = AVFps::fps_24());
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

    private:
        QExplicitlySharedDataPointer<AVSmpteTimePrivate> p;
};


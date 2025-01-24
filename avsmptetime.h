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
        bool negatives() const;
        bool fullhours() const;
        QString to_string() const;
        void invalidate();
        bool valid() const;
        void set_time(const AVTime& time);
        void set_negatives(bool negatives);
        void set_fullhours(bool fullhours);
    
        AVSmpteTime& operator=(const AVSmpteTime& other);
        bool operator==(const AVSmpteTime& other) const;
        bool operator!=(const AVSmpteTime& other) const;
        bool operator<(const AVSmpteTime& other) const;
        bool operator>(const AVSmpteTime& other) const;
        bool operator<=(const AVSmpteTime& other) const;
        bool operator>=(const AVSmpteTime& other) const;
        AVSmpteTime operator+(const AVSmpteTime& other) const;
        AVSmpteTime operator-(const AVSmpteTime& other) const;
    
        static qint64 convert(quint64 frame, const AVFps& from, const AVFps& to);
        static qint64 convert(quint64 frame, const AVFps& fps, bool reverse = false);
                                   
    private:
        QExplicitlySharedDataPointer<AVSmpteTimePrivate> p;
};


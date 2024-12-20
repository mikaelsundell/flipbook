// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avtime.h"

#include <QExplicitlySharedDataPointer>

class AVSmpteTimePrivate;
class AVSmpteTime {
    public:
        AVSmpteTime();
        AVSmpteTime(const AVTime& other);
        AVSmpteTime(const AVSmpteTime& other);
        ~AVSmpteTime();
        quint32 counter() const;
        quint32 type() const;
        qint16 hours() const;
        qint16 minutes() const;
        qint16 seconds() const;
        qint16 frames() const;
        qint16 subframes() const;
        qint16 subframedivisor() const;
        AVTime to_time() const;
        QString to_string() const;
        bool valid() const;
    
        void set_counter(const quint32 counter);
        void set_type(quint32 type);
        void set_hours(qint16 hours);
        void set_minutes(qint16 minutes);
        void set_seconds(qint16 seconds);
        void set_frames(qint16 frames);
        void set_subframes(qint16 subframes);
        void set_subframedivisor(qint16 subframedivisor);
    
        AVSmpteTime& operator=(const AVSmpteTime& other);
        bool operator==(const AVSmpteTime& other) const;
        bool operator!=(const AVSmpteTime& other) const;

    private:
        QExplicitlySharedDataPointer<AVSmpteTimePrivate> p;
};


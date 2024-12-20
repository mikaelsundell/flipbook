// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QExplicitlySharedDataPointer>

class AVTimePrivate;
class AVTime
{
    public:
        AVTime();
        AVTime(qint64 time, qint32 timescale);
        AVTime(const AVTime& other);
        ~AVTime();
        qint64 time() const;
        qint32 timescale() const;
        QString to_string() const;
        bool valid() const;
    
        void set_time(qint64 time);
        void set_timescale(qint32 timescale);
    
        AVTime& operator=(const AVTime& other);
        bool operator==(const AVTime& other) const;
        bool operator!=(const AVTime& other) const;
        bool operator<(const AVTime& other) const;
        bool operator>(const AVTime& other) const;
        bool operator<=(const AVTime& other) const;
        bool operator>=(const AVTime& other) const;
        AVTime operator+(const AVTime& other) const;

    private:
        QExplicitlySharedDataPointer<AVTimePrivate> p;
};

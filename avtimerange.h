// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avtime.h"

#include <QExplicitlySharedDataPointer>

class AVTimeRangePrivate;
class AVTimeRange
{
    public:
        AVTimeRange();
        AVTimeRange(AVTime start, AVTime duration);
        AVTimeRange(const AVTimeRange& other);
        ~AVTimeRange();
        AVTime start() const;
        AVTime duration() const;
        AVTime end() const;
        AVTime bound(const AVTime& time);
        AVTime bound(const AVTime& time, const AVFps& fps, bool loop = false);
        QString to_string() const;
        bool valid() const;
    
        void set_start(AVTime start);
        void set_duration(AVTime duration);

        AVTimeRange& operator=(const AVTimeRange& other);
        bool operator==(const AVTimeRange& other) const;
        bool operator!=(const AVTimeRange& other) const;
    
        static AVTimeRange scale(AVTimeRange timerange, qint32 timescale = 24000);
    
    private:
        QExplicitlySharedDataPointer<AVTimeRangePrivate> p;
};

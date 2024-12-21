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
        AVTime bound(AVTime time);
        bool contains(const AVTime& time) const;
        bool overlaps(const AVTimeRange& other) const;
        QString to_string() const;
        bool valid() const;
    
        void set_start(AVTime start);
        void set_duration(AVTime duration);

        AVTimeRange& operator=(const AVTimeRange& other);
        bool operator==(const AVTimeRange& other) const;
        bool operator!=(const AVTimeRange& other) const;
    
    private:
        QExplicitlySharedDataPointer<AVTimeRangePrivate> p;
};

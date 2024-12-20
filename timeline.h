// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"

#include <QWidget>
#include <QScopedPointer>

class TimelinePrivate;
class Timeline : public QWidget
{
    Q_OBJECT
    public:
        enum Units { TIMECODE, TIME, FRAMES };
        Q_ENUM(Units)
        
    public:
        Timeline(QWidget* parent = nullptr);
        virtual ~Timeline();
        QSize sizeHint() const override;
        Units units() const;
    
    public Q_SLOTS:
        void set_range(const AVTimeRange& range);
        void set_time(const AVTime& time);
        void set_units(Timeline::Units units);
    
    Q_SIGNALS:
        void time_changed(const AVTime& time);
        void timecode_changed(const AVSmpteTime& timecode);
        void slider_pressed();
        void slider_released();
    
    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        QScopedPointer<TimelinePrivate> p;
};

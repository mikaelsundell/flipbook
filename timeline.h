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
        enum Timecode { FRAMES, TIME, SMPTE };
        Q_ENUM(Timecode)
        
    public:
        Timeline(QWidget* parent = nullptr);
        virtual ~Timeline();
        QSize sizeHint() const override;
        AVTimeRange range() const;
        AVTime time() const;
        bool tracking() const;
        Timecode timecode() const;
    
    public Q_SLOTS:
        void set_range(const AVTimeRange& range);
        void set_time(const AVTime& time);
        void set_tracking(bool tracking);
        void set_timecode(Timeline::Timecode timecode);
    
    Q_SIGNALS:
        void time_changed(const AVTime& time);
        void slider_moved(const AVTime& time); // todo: look into name here, good enough?
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

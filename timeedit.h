// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include "avtime.h"

#include <QWidget>
#include <QScopedPointer>

class TimeeditPrivate;
class Timeedit : public QWidget
{
    Q_OBJECT
    public:
        enum Timecode { FRAMES, TIME, SMPTE };
        Q_ENUM(Timecode)
    
    public:
        Timeedit(QWidget* parent = nullptr);
        virtual ~Timeedit();

        AVTime time() const;
        Timecode timecode() const;
    
    public Q_SLOTS:
        void set_time(const AVTime& time);
        void set_timecode(Timeedit::Timecode timecode);
    
    protected:
        void paintEvent(QPaintEvent* event) override;
    
    private:
        QScopedPointer<TimeeditPrivate> p;
};

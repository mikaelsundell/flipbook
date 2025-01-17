// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtimer.h"
#include "avfps.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

#include <QPointer>

#include <QDebug>
#include <QThread>
#include <QtConcurrent>

#include <iostream>
#include <sstream>

class AVTimerPrivate
{
    public:
        AVTimerPrivate();
        ~AVTimerPrivate();
        quint64 nano(const AVFps& fps) {
            return static_cast<quint64>((1e9 * fps.denominator()) / fps.numerator());
        }
        quint64 nano(quint64 ticks) {
            return (ticks * d.timebase.numer) / d.timebase.denom;
        }
        quint64 ticks(quint64 time) {
            return (time * d.timebase.denom) / d.timebase.numer;
        }
        quint64 elapsed() {
            quint64 end = d.stop > 0 ? d.stop : mach_absolute_time();
            return (end - d.start) * d.timebase.numer / d.timebase.denom;
        }
        struct Data
        {
            quint64 start = 0;
            quint64 stop = 0;
            quint64 timer = 0;
            quint64 next = 0;
            QList<quint64> laps;
            mach_timebase_info_data_t timebase;
        };
        Data d;
};

AVTimerPrivate::AVTimerPrivate()
{
    mach_timebase_info(&d.timebase);
}

AVTimerPrivate::~AVTimerPrivate()
{
}

#include "avtimer.moc"

AVTimer::AVTimer()
: p(new AVTimerPrivate())
{
}

AVTimer::~AVTimer()
{
}

void
AVTimer::start()
{
    p->d.start = mach_absolute_time();
    p->d.stop = 0;
}

void
AVTimer::start(const AVFps& fps)
{
    Q_ASSERT("fps is zero" && fps.seconds() > 0);
    
    p->d.timer = mach_absolute_time();
    p->d.next = p->d.timer + p->ticks(p->nano(fps));
    p->d.stop = 0;
}

void
AVTimer::stop()
{
    p->d.stop = mach_absolute_time();
    p->d.next = p->d.stop;
}

void
AVTimer::restart()
{
    p->d.laps.clear();
    start();
}

void
AVTimer::lap()
{
    if (!p->d.laps.isEmpty()) {
        p->d.laps.append(elapsed() - p->d.laps.last());
    }
    else {
        p->d.laps.append(elapsed());
    }
}

bool
AVTimer::next(const AVFps& fps)
{
    quint64 currenttime = mach_absolute_time();
    p->d.next += p->ticks(p->nano(fps));
    return p->d.next > currenttime;
}

void 
AVTimer::wait()
{
    Q_ASSERT("start time must be less than nexttime" && p->d.start < p->d.next);
    
    mach_wait_until(p->d.next);
}

void
AVTimer::sleep(quint64 msecs)
{
    quint64 sleeptime = static_cast<quint64>(msecs * 1e6);
    quint64 currenttime = mach_absolute_time();
    quint64 targettime = currenttime + p->ticks(sleeptime);
    
    Q_ASSERT("sleep time already passed" && currenttime < targettime);
    mach_wait_until(targettime);
}

quint64
AVTimer::elapsed() const
{
    quint64 end = p->d.stop > 0 ? p->d.stop : mach_absolute_time();
    return p->nano(end - p->d.start);
}

QList<quint64>
AVTimer::laps() const
{
    return p->d.laps;
}

qreal
AVTimer::convert(quint64 nano, Unit unit)
{
    switch (unit) {
    case Unit::SECONDS:
        return static_cast<qreal>(nano) / 1e9;
        break;
    case Unit::MINUTES:
        return static_cast<qreal>(nano) / (60 * 1e9);
        break;
    case Unit::HOURS:
        return static_cast<qreal>(nano) / (3600 * 1e9);
        break;
    default:
        return static_cast<qreal>(nano);
    }
}


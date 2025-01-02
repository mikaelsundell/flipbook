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
        quint64 to_nano(const AVFps& fps) {
            return static_cast<quint64>((1e9 * fps.denominator()) / fps.numerator());
        }
        quint64 to_nano(quint64 ticks) {
            return (ticks * timebase.numer) / timebase.denom;
        }
        quint64 to_ticks(quint64 time) {
            return (time * timebase.denom) / timebase.numer;
        }
        quint64 elapsed() {
            quint64 endtime = stoptime > 0 ? stoptime : mach_absolute_time();
            return (endtime - starttime) * timebase.numer / timebase.denom;
        }
        quint64 starttime;
        quint64 stoptime;
        quint64 nexttime;
        quint64 interval;
        AVFps fps;
        mach_timebase_info_data_t timebase;
};

AVTimerPrivate::AVTimerPrivate()
: starttime(0)
, stoptime(0)
{
    mach_timebase_info(&timebase);
}

AVTimerPrivate::~AVTimerPrivate()
{
}

#include "avtimer.moc"

AVTimer::AVTimer()
: p(new AVTimerPrivate())
{
}

AVTimer::AVTimer(const AVFps& fps)
: p(new AVTimerPrivate())
{
    p->fps = fps;
}

AVTimer::~AVTimer()
{
}

void
AVTimer::start()
{
    Q_ASSERT("fps is zero" && p->fps.to_seconds() > 0);
    p->starttime = mach_absolute_time();
    p->nexttime = p->starttime + p->to_ticks(p->to_nano(p->fps));
    p->stoptime = 0;
}

void
AVTimer::stop()
{
    p->stoptime = mach_absolute_time();
    p->nexttime = p->stoptime;
}

void
AVTimer::restart()
{
    start();
}

bool
AVTimer::next()
{
    quint64 currenttime = mach_absolute_time();
    p->nexttime += p->to_ticks(p->to_nano(p->fps));
    return p->nexttime > currenttime;
}

void 
AVTimer::wait()
{
    Q_ASSERT("starttime must be less than nexttime" && p->starttime < p->nexttime);
    mach_wait_until(p->nexttime);
}

void
AVTimer::sleep(quint64 msecs)
{
    quint64 sleeptime = static_cast<quint64>(msecs * 1e6);
    quint64 currenttime = mach_absolute_time();
    quint64 targettime = currenttime + p->to_ticks(sleeptime);
    
    Q_ASSERT("sleep time already passed" && currenttime < targettime);
    mach_wait_until(targettime);
}

quint64
AVTimer::elapsed() const
{
    quint64 endtime = p->stoptime > 0 ? p->stoptime : mach_absolute_time();
    return p->to_nano(endtime - p->starttime);
}

void
AVTimer::set_fps(const AVFps& fps)
{
    if (p->fps != fps) {
        p->fps = fps;
    }
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


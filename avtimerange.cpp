// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtimerange.h"

#include <QtGlobal>

class AVTimeRangePrivate
{
    public:
        struct Data
        {
            AVTime start;
            AVTime duration;
        };
        Data d;
        QAtomicInt ref;
};

AVTimeRange::AVTimeRange()
: p(new AVTimeRangePrivate())
{
}

AVTimeRange::AVTimeRange(AVTime start, AVTime duration)
: p(new AVTimeRangePrivate())
{
    Q_ASSERT(start.timescale() == duration.timescale());
    
    p->d.start = start;
    p->d.duration = duration;
}

AVTimeRange::AVTimeRange(const AVTimeRange& other)
: p(other.p)
{
}

AVTimeRange::~AVTimeRange()
{
}

AVTime
AVTimeRange::start() const
{
    return p->d.start;
}

AVTime
AVTimeRange::duration() const
{
    return p->d.duration;
}

AVTime
AVTimeRange::end() const
{
    return p->d.start + p->d.duration;
}

AVTime
AVTimeRange::bound(const AVTime& time)
{
    Q_ASSERT(time.timescale() == start().timescale());
    return AVTime(qBound(start().ticks(), time.ticks(), end().ticks()), time.timescale(), time.fps());
}

AVTime
AVTimeRange::bound(const AVTime& time, bool loop)
{
    Q_ASSERT(time.timescale() == start().timescale());
    qint64 tpf = time.tpf();
    qint64 lower = start().ticks();
    qint64 upper = end().ticks() - tpf;
    if (loop) {
        qint64 range = upper - lower + tpf;
        qint64 wrapped = lower + ((time.ticks() - lower) % range + range) % range;
        return AVTime(wrapped, time.timescale(), time.fps());
    } else {
        return AVTime(qBound(lower, time.ticks(), upper), time.timescale(), time.fps());
    }
}

QString
AVTimeRange::to_string() const
{
    return QString("%1 / %2").arg(p->d.start.to_string()).arg(p->d.duration.to_string());
}

void
AVTimeRange::invalidate()
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.start.invalidate();
    p->d.duration.invalidate();
}

bool
AVTimeRange::valid() const {
    return p->d.start.valid() && p->d.duration.valid() && p->d.duration.ticks() > 0;
}

void
AVTimeRange::set_start(AVTime start)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.start = start;
}

void
AVTimeRange::set_duration(AVTime duration)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.duration = duration;
}

AVTimeRange&
AVTimeRange::operator=(const AVTimeRange& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AVTimeRange::operator==(const AVTimeRange& other) const
{
    return p->d.start == other.p->d.start && p->d.duration == other.p->d.duration;
}

bool
AVTimeRange::operator!=(const AVTimeRange& other) const
{
    return !(*this == other);
}

AVTimeRange
AVTimeRange::convert(const AVTimeRange& timerange, const AVFps& to)
{
    return AVTimeRange(AVTime::convert(timerange.start(), to), AVTime::convert(timerange.duration(), to));
}

AVTimeRange
AVTimeRange::convert(const AVTimeRange& timerange, qint32 timescale)
{
    return AVTimeRange(AVTime::convert(timerange.start(), timescale), AVTime::convert(timerange.duration(), timescale));
}

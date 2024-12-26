// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtimerange.h"

#include <QtGlobal>

class AVTimeRangePrivate
{
    public:
        AVTime start;
        AVTime duration;
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
    p->start = start;
    p->duration = duration;
}

AVTimeRange::AVTimeRange(const AVTimeRange& other)
: p(other.p)
{
}

AVTimeRange::~AVTimeRange()
{
}

AVTime
AVTimeRange::start() const {
    return p->start;
}

AVTime
AVTimeRange::duration() const {
    return p->duration;
}

AVTime
AVTimeRange::end() const {
    return p->start + p->duration;
}

AVTime
AVTimeRange::bound(const AVTime& time)
{
    Q_ASSERT(time.timescale() == start().timescale());
    return AVTime(qBound(start().ticks(), time.ticks(), end().ticks()), time.timescale());
}

AVTime
AVTimeRange::bound(const AVTime& time, qreal fps)
{
    Q_ASSERT(time.timescale() == start().timescale());
    return AVTime(qBound(start().ticks(), time.ticks(), end().ticks() - time.tpf(fps)), time.timescale());
}

QString
AVTimeRange::to_string() const
{
    return QString("%1 / %2").arg(p->start.to_string()).arg(p->duration.to_string());
}

bool
AVTimeRange::valid() const {
    return p->start.valid() && p->duration.valid() && p->duration.ticks() > 0;
}

void
AVTimeRange::set_start(AVTime start)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->start = start;
}

void
AVTimeRange::set_duration(AVTime duration)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->duration = duration;
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
    return p->start == other.p->start && p->duration == other.p->duration;
}

bool
AVTimeRange::operator!=(const AVTimeRange& other) const
{
    return !(*this == other);
}

AVTimeRange
AVTimeRange::scale(AVTimeRange timerange, qint32 timescale)
{
    return AVTimeRange(AVTime::scale(timerange.start(), timescale), AVTime::scale(timerange.duration(), timescale));
}

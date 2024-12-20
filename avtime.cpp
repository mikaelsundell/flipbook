// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtime.h"

class AVTimePrivate
{
    public:
        qint64 time = 0;
        quint32 timescale = 1;
        QAtomicInt ref;
        qreal to_seconds() const {
            return static_cast<qreal>(time) / timescale;
        }
};

AVTime::AVTime()
: p(new AVTimePrivate())
{
}

AVTime::AVTime(qint64 time, qint32 timescale)
: p(new AVTimePrivate())
{
    p->time = time;
    p->timescale = timescale;
}

AVTime::AVTime(const AVTime& other)
: p(other.p)
{
}

AVTime::~AVTime()
{
}

qint64
AVTime::time() const
{
    return p->time;
}

qint32
AVTime::timescale() const
{
    return p->timescale;
}

void
AVTime::set_time(qint64 time)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->time = time;
}

void
AVTime::set_timescale(qint32 timescale)
{
    if (timescale > 0) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->timescale = timescale;
    }
}

QString
AVTime::to_string() const
{
    qint64 seconds = p->to_seconds();
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

bool
AVTime::valid() const {
    return p->timescale > 0;
}

AVTime&
AVTime::operator=(const AVTime& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AVTime::operator==(const AVTime& other) const
{
    return p->time == other.p->time && p->timescale == other.p->timescale;
}

bool
AVTime::operator!=(const AVTime& other) const {
    return !(*this == other);
}

bool
AVTime::operator<(const AVTime& other) const {
    return p->to_seconds() < other.p->to_seconds();
}

bool
AVTime::operator>(const AVTime& other) const {
    return p->to_seconds() > other.p->to_seconds();
}

bool
AVTime::operator<=(const AVTime& other) const {
    return p->to_seconds() <= other.p->to_seconds();
}

bool
AVTime::operator>=(const AVTime& other) const {
    return p->to_seconds() >= other.p->to_seconds();
}

AVTime
AVTime::operator+(const AVTime& other) const {
    if (p->timescale == other.p->timescale) {
        return AVTime(p->time + other.p->time, p->timescale);
    } else {
        qint64 t = (p->time * other.p->timescale) + (other.p->time * p->timescale);
        qint32 ts = p->timescale * other.p->timescale;
        return AVTime(t, ts);
    }
}

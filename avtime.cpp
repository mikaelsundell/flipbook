// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtime.h"

class AVTimePrivate
{
    public:
        qint64 ticks = 0;
        qint32 timescale = 1;
        QAtomicInt ref;
        qreal to_seconds() const {
            return static_cast<qreal>(ticks) / timescale;
        }
};

AVTime::AVTime()
: p(new AVTimePrivate())
{
}

AVTime::AVTime(qint64 ticks, qint32 timescale)
: p(new AVTimePrivate())
{
    p->ticks = ticks;
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
AVTime::ticks() const
{
    return p->ticks;
}

qint64
AVTime::ticks(qreal fps) const
{
    return p->ticks - tpf(fps);
}

qint64
AVTime::ticks(qint64 frame, qreal fps) const
{
    return static_cast<qint64>(frame * tpf(fps));
}

qint32
AVTime::timescale() const
{
    return p->timescale;
}

qint64
AVTime::tpf(qreal fps) const
{
    return static_cast<qint64>(std::floor(static_cast<qreal>(timescale()) / fps));
}

qint64
AVTime::frame(qreal fps) const
{
    return static_cast<qint64>(ticks() / tpf(fps));
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

void
AVTime::set_ticks(qint64 ticks)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->ticks = ticks;
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
    return p->ticks == other.p->ticks && p->timescale == other.p->timescale;
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
    Q_ASSERT("timescale does not match" && p->timescale == other.p->timescale);
    return AVTime(p->ticks + other.p->ticks, p->timescale);
}

AVTime
AVTime::operator-(const AVTime& other) const {
    Q_ASSERT("timescale does not match" && p->timescale == other.p->timescale);
    return AVTime(p->ticks - other.p->ticks, p->timescale);
}

AVTime
AVTime::scale(AVTime time, qint32 timescale)
{
    qint64 ticks = 0;
    qint64 numerator = time.ticks() * timescale;
    qint64 remainder = numerator % time.timescale();
    ticks = numerator / time.timescale();
    if (remainder != 0) {
        if (time.ticks() > 0) {
            ticks += 1;
        } else {
            ticks -= 1;
        }
    }
    return AVTime(ticks, timescale);
}

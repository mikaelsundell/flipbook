// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtime.h"

#include <QDebug>

class AVTimePrivate
{
    public:
        AVFps fps = AVFps::fps_24();
        qint64 ticks = 0;
        qint32 timescale = 24000;
        QAtomicInt ref;
};

AVTime::AVTime()
: p(new AVTimePrivate())
{
}

AVTime::AVTime(qint64 ticks, qint32 timescale, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->ticks = ticks;
    p->timescale = timescale;
    p->fps = fps;
}

AVTime::AVTime(qint64 frame, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->fps = fps;
    p->ticks = ticks(frame);
}

AVTime::AVTime(qreal seconds, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->fps = fps;
    p->ticks = p->timescale * seconds;
}

AVTime::AVTime(const AVTime& other, const AVFps& fps)
: p(other.p)
{
    p->fps = fps;
}

AVTime::AVTime(const AVTime& other)
: p(other.p)
{
}

AVTime::~AVTime()
{
}

AVFps
AVTime::fps() const
{
    return p->fps;
}

qint64
AVTime::ticks() const
{
    return p->ticks;
}

qint64
AVTime::ticks(qint64 frame) const
{
    return static_cast<qint64>(frame * tpf());
}

qint32
AVTime::timescale() const
{
    return p->timescale;
}

qint64
AVTime::tpf() const
{
    return static_cast<qint64>(std::round(static_cast<qreal>(p->timescale) / p->fps.real()));
}

qint64
AVTime::frames() const
{
    return static_cast<qint64>(ticks() / tpf());
}

qreal
AVTime::seconds() const
{
    return static_cast<qreal>(p->ticks) / p->timescale;
}

QString
AVTime::to_string() const
{
    qint64 secs = seconds();
    qint64 minutes = secs / 60;
    qint64 hours = minutes / 60;
    secs %= 60;
    minutes %= 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

void
AVTime::invalidate() {
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->ticks = 0;
    p->timescale = 0;
    p->fps = AVFps();
}

bool
AVTime::valid() const {
    return p->timescale > 0;
}

void
AVTime::set_ticks(qint64 ticks)
{
    if (p->ticks != ticks) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->ticks = ticks;
    }
}

void
AVTime::set_timescale(qint32 timescale)
{
    if (p->timescale != timescale) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->timescale = timescale;
    }
}

void
AVTime::set_fps(const AVFps& fps)
{
    if (p->fps != fps) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->fps = fps;
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
    return seconds() < other.seconds();
}

bool
AVTime::operator>(const AVTime& other) const {
    return seconds() > other.seconds();
}

bool
AVTime::operator<=(const AVTime& other) const {
    return seconds() <= other.seconds();
}

bool
AVTime::operator>=(const AVTime& other) const {
    return seconds() >= other.seconds();
}

AVTime
AVTime::operator+(const AVTime& other) const {
    Q_ASSERT("timescale does not match" && p->timescale == other.p->timescale);
    return AVTime(p->ticks + other.p->ticks, p->timescale, p->fps);
}

AVTime
AVTime::operator-(const AVTime& other) const {
    Q_ASSERT("timescale does not match" && p->timescale == other.p->timescale);
    return AVTime(p->ticks - other.p->ticks, p->timescale, p->fps);
}

AVTime
AVTime::convert(const AVTime& time, const AVFps& from, const AVFps& to)
{
    return AVTime(AVFps::convert(time.ticks(), from, to), time.timescale(), to);
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
    return AVTime(ticks, timescale, time.fps());
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avtime.h"

#include <QDebug>

class AVTimePrivate
{
    public:
        qreal tpf() {
            return static_cast<qreal>(d.timescale) / d.fps.real();
        };
        qint64 ticks(qint64 frame) {
            return static_cast<qint64>(std::round(frame * tpf()));
        }
        qint64 frame(qint64 ticks) {
            return static_cast<qint64>(std::round(ticks / tpf()));
        }
        qint64 frames() {
            return static_cast<qint64>(std::round(d.ticks / tpf()));
        }
        QString to_string(qreal seconds) {
            qint64 secs = qFloor(seconds); // complete seconds
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
        struct Data
        {
            AVFps fps = AVFps::fps_24();
            qint64 ticks = 0;
            qint32 timescale = 24000; // default timescale
        };
        Data d;
        QAtomicInt ref;
};

AVTime::AVTime()
: p(new AVTimePrivate())
{
}

AVTime::AVTime(qint64 ticks, qint32 timescale, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->d.ticks = ticks;
    p->d.timescale = timescale;
    p->d.fps = fps;
}

AVTime::AVTime(qint64 frame, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->d.fps = fps;
    p->d.ticks = ticks(frame);
}

AVTime::AVTime(qreal seconds, const AVFps& fps)
: p(new AVTimePrivate())
{
    p->d.fps = fps;
    p->d.ticks = p->d.timescale * seconds;
}

AVTime::AVTime(const AVTime& other, const AVFps& fps)
: p(other.p)
{
    p->d.fps = fps;
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
    return p->d.fps;
}

qint64
AVTime::ticks() const
{
    return p->d.ticks;
}

qint64
AVTime::ticks(qint64 frame) const
{
    return p->ticks(frame);
}

qint32
AVTime::timescale() const
{
    return p->d.timescale;
}

qint64
AVTime::tpf() const
{
    return static_cast<qint64>(std::round(p->tpf()));
}

qint64
AVTime::frame(qint64 ticks) const
{
    return p->frame(ticks);
}

qint64
AVTime::frames() const
{
    return p->frames();
}

qint64
AVTime::align(qint64 ticks) const
{
    return this->ticks(p->frame(ticks));
}

qreal
AVTime::seconds() const
{
    return static_cast<qreal>(p->d.ticks) / p->d.timescale;
}

QString
AVTime::to_string(qint64 ticks) const
{
    return p->to_string(static_cast<qreal>(ticks) / p->d.timescale);
}

QString
AVTime::to_string() const
{
    return p->to_string(seconds());
}

void
AVTime::invalidate() {
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.ticks = 0;
    p->d.timescale = 0;
    p->d.fps = AVFps();
}

bool
AVTime::valid() const {
    return p->d.timescale > 0;
}

void
AVTime::set_ticks(qint64 ticks)
{
    if (p->d.ticks != ticks) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.ticks = ticks;
    }
}

void
AVTime::set_timescale(qint32 timescale)
{
    if (p->d.timescale != timescale) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.timescale = timescale;
    }
}

void
AVTime::set_fps(const AVFps& fps)
{
    if (p->d.fps != fps) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.fps = fps;
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
    return p->d.ticks == other.p->d.ticks && p->d.timescale == other.p->d.timescale && p->d.fps == other.p->d.fps;
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
    Q_ASSERT("timescale does not match" && p->d.timescale == other.p->d.timescale);
    return AVTime(p->d.ticks + other.p->d.ticks, p->d.timescale, p->d.fps);
}

AVTime
AVTime::operator-(const AVTime& other) const {
    Q_ASSERT("timescale does not match" && p->d.timescale == other.p->d.timescale);
    return AVTime(p->d.ticks - other.p->d.ticks, p->d.timescale, p->d.fps);
}

AVTime
AVTime::convert(const AVTime& time, const AVFps& from, const AVFps& to)
{
    return AVTime(AVFps::convert(time.ticks(), from, to), time.timescale(), to);
}

AVTime
AVTime::timescale(const AVTime& time, const AVFps& to)
{
    return timescale(time, to.frame_quanta() * 1000);
}

AVTime
AVTime::timescale(const AVTime& time, qint32 timescale)
{
    qint64 numerator = time.ticks() * timescale;
    qint64 remainder = numerator % time.timescale();
    qint64 ticks = numerator / time.timescale();
    if (remainder >= (time.timescale() / 2)) {
        if (time.ticks() > 0) {
            ticks += 1;
        } else if (time.ticks() < 0) {
            ticks -= 1;
        }
    }
    return AVTime(ticks, timescale, time.fps());
}


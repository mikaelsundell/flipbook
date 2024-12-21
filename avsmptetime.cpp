// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avsmptetime.h"

class AVSmpteTimePrivate
{
    public:
        quint32 counter = 0;
        quint32 type = 0;
        qint16 hours = 0;
        qint16 minutes = 0;
        qint16 seconds = 0;
        qint16 frames = 0;
        qint16 subframes = 1;
        qint16 subframedivisor = 0;
        QAtomicInt ref;
};
    
AVSmpteTime::AVSmpteTime()
: p(new AVSmpteTimePrivate())
{
}

AVSmpteTime::AVSmpteTime(const AVTime& other)
: p(new AVSmpteTimePrivate())
{
    if (other.valid() && other.ticks() > 0) {
        qint64 totalframes = other.ticks() / other.timescale();
        p->hours = totalframes / 3600;
        totalframes %= 3600;
        p->minutes = totalframes / 60;
        totalframes %= 60;
        p->seconds = totalframes;
        p->frames = (other.ticks() % other.timescale()) / (other.timescale() / 24);
        p->subframes = 0;
        p->subframedivisor = 1;
    } else {
        p->hours = 0;
        p->minutes = 0;
        p->seconds = 0;
        p->frames = 0;
        p->subframes = 0;
        p->subframedivisor = 1;
    }
}

AVSmpteTime::AVSmpteTime(const AVSmpteTime& other)
: p(other.p)
{
}

AVSmpteTime::~AVSmpteTime()
{
}

quint32
AVSmpteTime::counter() const
{
    return p->counter;
}

quint32
AVSmpteTime::type() const
{
    return p->type;
}

qint16
AVSmpteTime::hours() const
{
    return p->hours;
}

qint16
AVSmpteTime::minutes() const
{
    return p->minutes;
}

qint16
AVSmpteTime::seconds() const
{
    return p->seconds;
}

qint16
AVSmpteTime::frames() const
{
    return p->frames;
}

qint16
AVSmpteTime::subframes() const
{
    return p->subframes;
}

qint16
AVSmpteTime::subframedivisor() const
{
    return p->subframedivisor;
}

void
AVSmpteTime::set_counter(quint32 counter)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->counter = counter;
}

void
AVSmpteTime::set_type(quint32 type)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->type = type;
}

void
AVSmpteTime::set_hours(qint16 hours)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->hours = hours;
}

void
AVSmpteTime::set_minutes(qint16 minutes)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->minutes = minutes;
}

void
AVSmpteTime::set_seconds(qint16 seconds)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->seconds = seconds;
}

void
AVSmpteTime::set_frames(qint16 frames)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->frames = frames;
}

void
AVSmpteTime::set_subframes(qint16 subframes)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->subframes = subframes;
}

void
AVSmpteTime::set_subframedivisor(qint16 subframedivisor)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->subframedivisor = subframedivisor;
}

QString
AVSmpteTime::to_string() const
{
    return QString("%1:%2:%3:%4")
        .arg(p->hours, 2, 10, QChar('0'))
        .arg(p->minutes, 2, 10, QChar('0'))
        .arg(p->seconds, 2, 10, QChar('0'))
        .arg(p->frames, 2, 10, QChar('0'));
}
    
AVTime
AVSmpteTime::to_time() const
{
    if (!valid()) {
        return AVTime();
    }
    qint64 totalFrames = (p->hours * 3600 + p->minutes * 60 + p->seconds) * p->subframedivisor + p->frames;
    qint64 totalTimeUnits = totalFrames * p->subframes; // assume subframes represent the smallest unit
    return AVTime(totalTimeUnits, p->subframedivisor);
}

bool
AVSmpteTime::valid() const {
    return p->hours >= 0 && p->hours < 24 &&
        p->minutes >= 0 && p->minutes < 60 &&
        p->seconds >= 0 && p->seconds < 60 &&
        p->frames >= 0 && p->subframes >= 0 &&
        p->subframedivisor > 0;
}

AVSmpteTime&
AVSmpteTime::operator=(const AVSmpteTime& other)
{
    if (this != &other) {
        *p = *other.p;
    }
    return *this;
}

bool
AVSmpteTime::operator==(const AVSmpteTime& other) const
{
    return p->counter == other.p->counter &&
        p->type == other.p->type &&
        p->hours == other.p->hours &&
        p->minutes == other.p->minutes &&
        p->seconds == other.p->seconds &&
        p->frames == other.p->frames &&
        p->subframes == other.p->subframes &&
        p->subframedivisor == other.p->subframedivisor;
}

bool
AVSmpteTime::operator!=(const AVSmpteTime& other) const
{
    return !(*this == other);
}






// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avsmptetime.h"

#include <QDebug>

class AVSmpteTimePrivate
{
    public:
        AVSmpteTimePrivate();
        qint64 frame() const;
        void update();
        struct Data
        {
            AVTime time;
            quint32 counter = 0;
            qint16 hours = 0;
            qint16 minutes = 0;
            qint16 seconds = 0;
            qint16 frames = 0;
            qint16 subframes = 1;
            qint16 subframe_divisor = 0;
            bool allow_negatives = true;
            bool max_24hours = true;
        };
        Data d;
        QAtomicInt ref;
};

AVSmpteTimePrivate::AVSmpteTimePrivate()
{
}

qint64
AVSmpteTimePrivate::frame() const
{
    Q_ASSERT("time is not valid" && d.time.valid());
    
    return d.time.frames();
}

void
AVSmpteTimePrivate::update()
{
    Q_ASSERT("time is not valid" && d.time.valid());
    
    qint64 frame = d.time.frames();
    qint16 framequanta = d.time.fps().frame_quanta();
    bool is_negative = false;
    if (frame < 0) {
        is_negative = true;
        frame = -frame;
    }
    frame = AVSmpteTime::dropframe(frame, d.time.fps(), true);
    d.frames = frame % framequanta;
    frame /= framequanta;
    d.seconds =  frame % 60;
    frame /= 60;
    d.minutes = frame % 60;
    frame /= 60;
    if (d.max_24hours) {
        frame %= 24;
        if (is_negative && !d.allow_negatives) {
            is_negative = false;
            frame = 23 - frame;
        }
    }
    d.hours = frame;
    if (is_negative) {
        d.minutes |= 0x80; // indicate negative number
    }
}
    
AVSmpteTime::AVSmpteTime()
: p(new AVSmpteTimePrivate())
{
}

AVSmpteTime::AVSmpteTime(const AVTime& time)
: p(new AVSmpteTimePrivate())
{
    p->d.time = time;
    p->update();
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
    return p->d.counter;
}

qint16
AVSmpteTime::hours() const
{
    return p->d.hours;
}

qint16
AVSmpteTime::minutes() const
{
    return p->d.minutes;
}

qint16
AVSmpteTime::seconds() const
{
    return p->d.seconds;
}

qint16
AVSmpteTime::frames() const
{
    return p->d.frames;
}

qint16
AVSmpteTime::subframes() const
{
    return p->d.subframes;
}

qint16
AVSmpteTime::subframe_divisor() const
{
    return p->d.subframe_divisor;
}

qint64
AVSmpteTime::frame() const
{
    return p->frame();
}

AVTime
AVSmpteTime::time() const
{
    return p->d.time;
}

bool
AVSmpteTime::allow_negatives() const
{
    return p->d.allow_negatives;
}

void
AVSmpteTime::set_time(const AVTime& time)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.time = time;
    p->update();
}

void
AVSmpteTime::set_allow_negatives(bool allow_negatives)
{
    if (p->d.allow_negatives != allow_negatives) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.allow_negatives = allow_negatives;
        p->update();
    }
}

void
AVSmpteTime::set_max24hours(bool max_24hours)
{
    if (p->d.max_24hours != max_24hours) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.max_24hours = max_24hours;
        p->update();
    }
}

QString
AVSmpteTime::to_string() const
{
    QString text;
    if (p->d.time.fps().drop_frame()) {
        text = "%1:%2:%3.%4"; // use . for drop frames
    }
    else {
        text = "%1:%2:%3:%4";
    }
    return QString(text)
        .arg(p->d.hours, 2, 10, QChar('0'))
        .arg(p->d.minutes, 2, 10, QChar('0'))
        .arg(p->d.seconds, 2, 10, QChar('0'))
        .arg(p->d.frames, 2, 10, QChar('0'));
}

void
AVSmpteTime::invalidate() {
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.time.invalidate();
}
    
bool
AVSmpteTime::valid() const {
    return p->d.hours >= 0 && p->d.hours < 24 &&
        p->d.minutes >= 0 && p->d.minutes < 60 &&
        p->d.seconds >= 0 && p->d.seconds < 60 &&
        p->d.frames >= 0 && p->d.subframes >= 0 &&
        p->d.subframe_divisor > 0;
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
    return p->d.counter == other.p->d.counter &&
        p->d.hours == other.p->d.hours &&
        p->d.minutes == other.p->d.minutes &&
        p->d.seconds == other.p->d.seconds &&
        p->d.frames == other.p->d.frames &&
        p->d.subframes == other.p->d.subframes &&
        p->d.subframe_divisor == other.p->d.subframe_divisor;
}

bool
AVSmpteTime::operator!=(const AVSmpteTime& other) const
{
    return !(*this == other);
}

bool
AVSmpteTime::operator<(const AVSmpteTime& other) const {
    return this->frame() < other.frame();
}

bool
AVSmpteTime::operator>(const AVSmpteTime& other) const {
    return this->frame() > other.frame();
}

bool
AVSmpteTime::operator<=(const AVSmpteTime& other) const {
    return this->frame() <= other.frame();
}

bool
AVSmpteTime::operator>=(const AVSmpteTime& other) const {
    return this->frame() >= other.frame();
}

AVSmpteTime
AVSmpteTime::operator+(const AVSmpteTime& other) const {
    Q_ASSERT("fps must match" && p->d.time.fps() == other.time().fps());
    qint64 frames = this->time().frames() + other.time().frames();
    AVTime time = AVTime(frames, p->d.time.fps());
    return AVSmpteTime(time);
}

AVSmpteTime
AVSmpteTime::operator-(const AVSmpteTime& other) const {
    Q_ASSERT("fps must match" && p->d.time.fps() == other.time().fps());
    qint64 frames = this->time().frames() - other.time().frames();
    AVTime time = AVTime(frames, p->d.time.fps());
    return AVSmpteTime(time);
}

qint64
AVSmpteTime::dropframe(quint64 frame, const AVFps& fps, bool inverse)
{
    qint64 frametime = frame;
    if (!inverse) {
        if (fps.drop_frame()) {
            qint16 framequanta = fps.frame_quanta();
            qint64 framemin = framequanta * 60;
            qint64 frame10min = framemin * 10 - 9 * 2;
            qint64 num10s = frame / frame10min;
            qint64 adjust = -num10s * (9 * 2);
            qint64 left = frame % frame10min;
            if (left > 1) {
                qint64 num1s = left / framemin;
                if (num1s > 0) {
                    adjust -= (num1s - 1) * 2;
                    left = left % framemin;
                    if (left > 1) {
                        left -= 2;
                    } else {
                        left -= (left + 1);
                    }
                }
            }
            frametime += adjust;
        }
    }
    else {
        if (fps.drop_frame()) {
            qint64 framemin = fps.frame_quanta() * 60;
            qint64 frame10min = framemin * 10 - 9 * 2;
            qint64 frame10minblocks = frametime / frame10min;
            qint64 remaining = frametime % frame10min;
            qint64 adjust = (frame10minblocks * 9 * 2);
            if (remaining >= framemin) {
                adjust += (remaining / framemin) * 2;
            }
            frametime += adjust;
        }
    }
    return frametime;
}

qint64
AVSmpteTime::frame(quint16 hours, quint16 minutes, quint16 seconds, quint16 frames, const AVFps& fps)
{
    int64_t frame = 0;
    qint16 framequanta = fps;
    frame = frames;
    frame += seconds * framequanta;
    frame += (minutes & ~0x80) * framequanta * 60; // 0x80 negative bit in minutes
    frame += hours * framequanta * 60 * 60;
    int64_t fpm = framequanta * 60;
    frame = AVSmpteTime::dropframe(frame, fps);
    if (minutes & 0x80) // check for negative bit
        frame = -frame;
    return frame;
}


AVSmpteTime
AVSmpteTime::combine(const AVTime& time, const AVTime& other)
{
    AVFps fps = time.fps();
    AVFps fpsother = other.fps();
    qint64 frames = other.frames();
    if (fps != fpsother) {
        if (fps.frame_quanta() == fpsother.frame_quanta()) {
            if (fps.drop_frame() && !fpsother.drop_frame()) {
                frames = AVSmpteTime::dropframe(frames, fps, false);
            }
            else if (!fps.drop_frame() && fpsother.drop_frame()) {
                frames = AVSmpteTime::dropframe(frames, fpsother, true);
            }
        } else {
            frames = AVFps::convert(frames, fpsother, fps);
        }
    }
    return AVSmpteTime(AVTime(time.frames() + frames, fps));
}

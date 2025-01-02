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
    
    public:
        AVTime time;
        AVFps fps;
        quint32 counter;
        qint16 hours;
        qint16 minutes;
        qint16 seconds;
        qint16 frames;
        qint16 subframes;
        qint16 subframe_divisor;
        bool allow_negatives;
        bool max_24hours;
        QAtomicInt ref;
};

AVSmpteTimePrivate::AVSmpteTimePrivate()
: time(AVTime(0, AVFps::fps_24()))
, fps(AVFps::fps_24())
, counter(0)
, hours(0)
, minutes(0)
, seconds(0)
, frames(0)
, subframes(1)
, subframe_divisor(0)
, allow_negatives(true)
, max_24hours(true)
{
}

qint64
AVSmpteTimePrivate::frame() const
{
    Q_ASSERT("time is not valid" && time.valid());
    Q_ASSERT("fps is not valid" && fps.valid());
    
    int64_t frame = 0;
    qint16 framequanta = fps.to_frame_quanta();
    
    frame = frames;
    frame += seconds * framequanta;
    frame += (minutes & ~0x80) * framequanta * 60; // 0x80 negative bit in minutes
    frame += hours * framequanta * 60 * 60;
    int64_t fpm = framequanta * 60;
    
    if (fps.drop_frame()) {
        int64_t fpm10 = fpm * 10;
        int64_t num10s = frame / fpm10;
        int64_t frameadjust = -num10s*(9 * 2);
        int64_t framesleft = frame % fpm10;
        
        if (framesleft > 1) {
            int64_t num1s = framesleft / fpm;
            if (num1s > 0) {
                frameadjust -= (num1s-1) * 2;
                framesleft = framesleft % fpm;
                if (framesleft > 1) {
                    frameadjust -= 2;
                }
                else {
                    frameadjust -= (framesleft+1);
                }
            }
        }
        frame += frameadjust;
    }
    if (minutes & 0x80) // check for negative bit
        frame = -frame;
    return frame;
}

void
AVSmpteTimePrivate::update()
{
    Q_ASSERT("time is not valid" && time.valid());
    Q_ASSERT("fps is not valid" && fps.valid());
    
    qint64 frame = time.frame(fps);
    qint16 framequanta = fps.to_frame_quanta();
    bool is_negative = false;
    if (frame < 0) {
        is_negative = true;
        frame = -frame;
    }
    if (fps.drop_frame()) {
        
        qint64 fpm = framequanta * 60 - 2;
        qint64 fpm10 = framequanta * 10 * 60 - 9 * 2;
        qint64 num10s = frame / fpm10;
        qint64 frameadjust = num10s*(9 * 2);
        qint64 framesleft = frame % fpm10;
        if (framesleft >= fps * 60) {
            framesleft -= fps * 60;
            int64_t num1s = framesleft / fpm;
            frameadjust += (num1s + 1) * 2;
        }
        frame += frameadjust;
    }
    frames = frame % framequanta;
    frame /= fps;
    seconds =  frame % 60;
    frame /= 60;
    minutes = frame % 60;
    frame /= 60;
    if (max_24hours) {
        frame %= 24;
        if (is_negative && !allow_negatives) {
            is_negative = false;
            frame = 23 - frame;
        }
    }
    hours = frame;
    if (is_negative) {
        minutes |= 0x80; // indicate negative number
    }
}
    
AVSmpteTime::AVSmpteTime()
: p(new AVSmpteTimePrivate())
{
}

AVSmpteTime::AVSmpteTime(const AVTime& time, const AVFps& fps)
: p(new AVSmpteTimePrivate())
{
    p->time = time;
    p->fps = fps;
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
    return p->counter;
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
AVSmpteTime::subframe_divisor() const
{
    return p->subframe_divisor;
}

qint64
AVSmpteTime::frame() const
{
    return p->frame();
}

bool
AVSmpteTime::allow_negatives() const
{
    return p->allow_negatives;
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
AVSmpteTime::set_subframe_divisor(qint16 subframe_divisor)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->subframe_divisor = subframe_divisor;
}

void
AVSmpteTime::set_time(const AVTime& time, const AVFps& fps)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->time = time;
    p->fps = fps;
    p->update();
}

void
AVSmpteTime::set_allow_negatives(bool allow_negatives)
{
    if (p->allow_negatives != allow_negatives) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->allow_negatives = allow_negatives;
        p->update();
    }
}

void
AVSmpteTime::set_max24hours(bool max_24hours)
{
    if (p->max_24hours != max_24hours) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->max_24hours = max_24hours;
        p->update();
    }
}

QString
AVSmpteTime::to_string() const
{
    QString text;
    if (p->fps.drop_frame()) {
        text = "%1:%2:%3.%4"; // use . for drop frames
    }
    else {
        text = "%1:%2:%3:%4";
    }
    return QString(text)
        .arg(p->hours, 2, 10, QChar('0'))
        .arg(p->minutes, 2, 10, QChar('0'))
        .arg(p->seconds, 2, 10, QChar('0'))
        .arg(p->frames, 2, 10, QChar('0'));
}
    
AVTime
AVSmpteTime::to_time() const
{
    return(AVTime(frame(), p->fps));
}

bool
AVSmpteTime::valid() const {
    return p->hours >= 0 && p->hours < 24 &&
        p->minutes >= 0 && p->minutes < 60 &&
        p->seconds >= 0 && p->seconds < 60 &&
        p->frames >= 0 && p->subframes >= 0 &&
        p->subframe_divisor > 0;
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
        p->hours == other.p->hours &&
        p->minutes == other.p->minutes &&
        p->seconds == other.p->seconds &&
        p->frames == other.p->frames &&
        p->subframes == other.p->subframes &&
        p->subframe_divisor == other.p->subframe_divisor;
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
    qint64 totalframes = this->frame() + other.frame();
    AVTime time = AVTime(totalframes, p->fps);
    return AVSmpteTime(time, p->fps);
}

AVSmpteTime
AVSmpteTime::operator-(const AVSmpteTime& other) const {
    qint64 totalframes = this->frame() - other.frame();
    AVTime time = AVTime(totalframes, p->fps);
    return AVSmpteTime(time, p->fps);
}

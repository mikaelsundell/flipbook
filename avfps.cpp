// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avfps.h"

class AVFpsPrivate
{
    public:
        qint64 numerator = 0;
        qint32 denominator = 0;
        bool drop_frame;
        QAtomicInt ref;
};

AVFps::AVFps()
: p(new AVFpsPrivate())
{
}

AVFps::AVFps(qint32 numerator, qint32 denominator, bool drop_frame)
: p(new AVFpsPrivate())
{
    p->numerator = numerator;
    p->denominator = denominator;
    p->drop_frame = drop_frame;
}

AVFps::AVFps(const AVFps& other)
: p(other.p)
{
}

AVFps::~AVFps()
{
}

qint64
AVFps::numerator() const
{
    return p->numerator;
}

qint32
AVFps::denominator() const
{
    return p->denominator;
}

bool
AVFps::drop_frame() const
{
    return p->drop_frame;
}

QString
AVFps::to_string() const
{
    return QString("%1").arg(to_seconds());
}

qreal
AVFps::to_real() const
{
    return static_cast<qreal>(numerator()) / denominator();
}

qreal
AVFps::to_seconds() const
{
    return 1.0 / to_real();
}

bool
AVFps::valid() const {
    return p->numerator > 0;
}

void
AVFps::set_numerator(qint64 numerator)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->numerator = numerator;
}

void
AVFps::set_denominator(qint32 denominator)
{
    if (denominator > 0) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->denominator = denominator;
    }
}

AVFps&
AVFps::operator=(const AVFps& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AVFps::operator==(const AVFps& other) const
{
    return p->numerator == other.p->numerator &&
           p->denominator == other.p->denominator &&
           p->drop_frame == other.p->drop_frame;
}

bool
AVFps::operator!=(const AVFps& other) const {
    return !(*this == other);
}

bool
AVFps::operator<(const AVFps& other) const {
    return to_seconds() < other.to_seconds();
}

bool
AVFps::operator>(const AVFps& other) const {
    return to_seconds() > other.to_seconds();
}

bool
AVFps::operator<=(const AVFps& other) const {
    return to_seconds() <= other.to_seconds();
}

bool
AVFps::operator>=(const AVFps& other) const {
    return to_seconds() >= other.to_seconds();
}

AVFps
AVFps::fps_23_976()
{
    return AVFps(24000, 1001, true);
}

AVFps
AVFps::fps_24()
{
    return AVFps(24, 1);
}

AVFps
AVFps::fps_25()
{
    return AVFps(25, 1);
}

AVFps
AVFps::fps_29_97()
{
    return AVFps(30000, 1001, true);
}

AVFps
AVFps::fps_30()
{
    return AVFps(30, 1);
}

AVFps
AVFps::fps_47_952()
{
    return AVFps(48000, 1001, true);
}

AVFps
AVFps::fps_48()
{
    return AVFps(48, 1);
}

AVFps
AVFps::fps_50()
{
    return AVFps(50, 1);
}

AVFps
AVFps::fps_59_94()
{
    return AVFps(60000, 1001, true);
}

AVFps
AVFps::fps_60()
{
    return AVFps(60, 1);
}

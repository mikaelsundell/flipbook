// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avfps.h"

#include <QVector>

class AVFpsPrivate
{
    public:
        struct Data
        {
            qint32 numerator = 0;
            qint32 denominator = 0;
            bool drop_frame = false;
        };
        Data d;
        QAtomicInt ref;
};

AVFps::AVFps()
: p(new AVFpsPrivate())
{
}

AVFps::AVFps(qint32 numerator, qint32 denominator, bool drop_frame)
: p(new AVFpsPrivate())
{
    p->d.numerator = numerator;
    p->d.denominator = denominator;
    p->d.drop_frame = drop_frame;
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
    return p->d.numerator;
}

qint32
AVFps::denominator() const
{
    return p->d.denominator;
}

bool
AVFps::drop_frame() const
{
    return p->d.drop_frame;
}

qint16
AVFps::frame_quanta() const
{
    return static_cast<qint64>(std::round(real()));
}

qreal
AVFps::real() const
{
    Q_ASSERT("fps is not valid" && valid());
    
    return static_cast<qreal>(numerator()) / denominator();
}

qreal
AVFps::seconds() const
{
    return 1.0 / real();
}

qreal
AVFps::to_fps(qint64 frame, const AVFps& other) const
{
    return static_cast<qint64>(std::round(frame * (real() / other.real())));
}

QString
AVFps::to_string() const
{
    return QString("%1").arg(seconds());
}

bool
AVFps::valid() const {
    return p->d.denominator > 0;
}

void
AVFps::set_numerator(qint32 numerator)
{
    if (p->d.numerator != numerator) {
        if (p->ref.loadRelaxed() > 1) {
            p.detach();
        }
        p->d.numerator = numerator;
    }
}

void
AVFps::set_denominator(qint32 denominator)
{
    if (p->d.denominator != denominator) {
        if (denominator > 0) {
            if (p->ref.loadRelaxed() > 1) {
                p.detach();
            }
            p->d.denominator = denominator;
        }
    }
}

void
AVFps::set_dropframe(bool dropframe)
{
    if (p->d.drop_frame != dropframe) {
        p->d.drop_frame = dropframe;
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
    return p->d.numerator == other.p->d.numerator &&
           p->d.denominator == other.p->d.denominator &&
           p->d.drop_frame == other.p->d.drop_frame;
}

bool
AVFps::operator!=(const AVFps& other) const {
    return !(*this == other);
}

bool
AVFps::operator<(const AVFps& other) const {
    return real() < other.real();
}

bool
AVFps::operator>(const AVFps& other) const {
    return real() > other.real();
}

bool
AVFps::operator<=(const AVFps& other) const {
    return real() <= other.real();
}

bool
AVFps::operator>=(const AVFps& other) const {
    return real() >= other.real();
}

AVFps::operator double() const
{
    return real();
}

AVFps
AVFps::guess(qreal fps)
{
    const qreal epsilon = 0.005;
    const QVector<AVFps> standards = {
        fps_23_976(),
        fps_24(),
        fps_25(),
        fps_29_97(),
        fps_30(),
        fps_47_952(),
        fps_48(),
        fps_50(),
        fps_59_94(),
        fps_60()
    };
    for (const AVFps& standard : standards) {
        if (qAbs(standard.real() - fps) < epsilon) {
            return standard;
        }
    }
    return AVFps(static_cast<qint32>(fps * 1000), 1000);
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

qint64
AVFps::convert(quint64 value, const AVFps& from, const AVFps& to)
{
    return value * (to / from);
}

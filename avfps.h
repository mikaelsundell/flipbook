// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QExplicitlySharedDataPointer>

class AVFpsPrivate;
class AVFps
{
    public:
        AVFps();
        AVFps(qint32 numerator, qint32 denominator, bool drop_frame = false);
        AVFps(const AVFps& other);
        ~AVFps();
        qint64 numerator() const;
        qint32 denominator() const;
        bool drop_frame() const;
        QString to_string() const;
        qint16 to_frame_quanta() const;
        qreal to_real() const;
        qreal to_seconds() const;
        qreal to_fps(qint64 frame, const AVFps& other) const;
        bool valid() const;
    
        void set_numerator(qint32 nominator);
        void set_denominator(qint32 denominator);
        void set_dropframe(bool dropframe);
    
        AVFps& operator=(const AVFps& other);
        bool operator==(const AVFps& other) const;
        bool operator!=(const AVFps& other) const;
        bool operator<(const AVFps& other) const;
        bool operator>(const AVFps& other) const;
        bool operator<=(const AVFps& other) const;
        bool operator>=(const AVFps& other) const;
        operator double() const;
    
        static AVFps guess(qreal fps);
        static AVFps fps_23_976();
        static AVFps fps_24();
        static AVFps fps_25();
        static AVFps fps_29_97();
        static AVFps fps_30();
        static AVFps fps_47_952();
        static AVFps fps_48();
        static AVFps fps_50();
        static AVFps fps_59_94();
        static AVFps fps_60();

    private:
        QExplicitlySharedDataPointer<AVFpsPrivate> p;
};

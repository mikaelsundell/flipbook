// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"

#include "avtime.h"
#include "avtimerange.h"
#include "avfps.h"

#include <QApplication>

void
test_time() {
    AVTime time;
    time.set_ticks(12000);
    time.set_timescale(24000);
    qreal fps = 24;
    Q_ASSERT("ticks per frame" && time.tpf(fps) == 1000);
    Q_ASSERT("ticks to frame" && time.frame(fps) == 12);
    Q_ASSERT("frame to ticks" && time.ticks(12, fps) == 12000);
    
    qDebug() << "ticks per frame: " << time.tpf(fps);
    qDebug() << "ticks per frame: " << time.frame(fps);
    qDebug() << "frame to ticks: " << time.ticks(12, fps);
}

void test_timerange() {
    AVTimeRange timerange;
    timerange.set_start(AVTime(12000, 24000));
    AVTime duration = AVTime::scale(AVTime(384000, 48000), timerange.start().timescale());
    timerange.set_duration(duration);
    Q_ASSERT("convert timescale" && duration.ticks() == 192000);
    Q_ASSERT("end ticks" && timerange.end().ticks() == 204000);
    
    qDebug() << "timerange: " << timerange.to_string();
}

void test_fps() {
    Q_ASSERT("24 fps" &&  AVFps::fps_24() == AVFps(24, 1));
    
    qDebug() << "fps 24: " << AVFps::fps_24().to_seconds();
    qDebug() << "fps 24: " << AVFps(24, 1).to_seconds();
    qDebug() << "fps 24: " << 1/24.0;
    
    // todo: add in timecode reading from drop frame to verify ntsc codes
    // todo: add epsilon check for doubles to verify to_seconds
}

// main
int
main(int argc, char* argv[])
{
    // tests
    {
        test_time();
        test_timerange();
        test_fps();
    }
    QApplication app(argc, argv);
    Flipbook* flipbook = new Flipbook();
    flipbook->show();
    return app.exec();
}

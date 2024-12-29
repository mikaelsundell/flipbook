// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"

#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"
#include "avfps.h"

#include <QApplication>

void
test_time() {
    AVTime time;
    time.set_ticks(12000);
    time.set_timescale(24000);
    
    AVFps fps = AVFps::fps_24();
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
    
    AVTime time;
    time.set_ticks(24000 * 100); // typical ticks, 100 seconds
    time.set_timescale(24000);
    qint64 ticks;
    
    AVFps guess23_97 = AVFps::guess(23.976);
    Q_ASSERT("23.976 fps has drop frames" && guess23_97.drop_frame());
    qDebug() << "fps 23.976: " << guess23_97;
    
    AVFps guess24 = AVFps::guess(24);
    Q_ASSERT("24 fps is standard" && !guess24.drop_frame());
    qDebug() << "fps 24: " << guess24;
    
    AVFps guess10 = AVFps::guess(10);
    Q_ASSERT("10 fps is standard" && !guess10.drop_frame());
    qDebug() << "fps 10: " << guess10;
    
    AVFps fps23_97 = AVFps::fps_23_976();
    ticks = time.ticks(1, fps23_97);
    Q_ASSERT("23.97 fps ticks" &&  ticks == 1001);
    qDebug() << "ticks 23_97: " << ticks;
    
    AVFps fps24 = AVFps::fps_24();
    ticks = time.ticks(1, fps24);
    Q_ASSERT("24 fps ticks" &&  ticks == 1000);
    qDebug() << "ticks 24: " << ticks;
}

void test_smpte() {
    AVFps fps = AVFps::fps_24();
    qint64 frame = 86496; // typical offset, 24fps, 01:00:04:00
    AVTime time(frame, fps);
    
    AVSmpteTime smpte(time, fps);
    Q_ASSERT("timecode is not correct" && smpte.to_string() == "01:00:04:00");
    Q_ASSERT("frame is not correct" && frame == smpte.frame());
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
        test_smpte();
    }
    QApplication app(argc, argv);
    Flipbook* flipbook = new Flipbook();
    flipbook->show();
    return app.exec();
}

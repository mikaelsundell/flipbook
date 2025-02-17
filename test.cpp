// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "test.h"
#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"
#include "avfps.h"
#include "avtimer.h"

#include <QApplication>
#include "timeedit.h"

#include <QThread>
#include <QtConcurrent>

#include <QDebug>
#include <iostream>

void
test_time() {
    qDebug() << "Testing time";
    
    AVTime time;
    time.set_ticks(12000);
    time.set_timescale(24000);
    time.set_fps(AVFps::fps_24());
    Q_ASSERT("ticks per frame" && time.tpf() == 1000);
    Q_ASSERT("ticks to frame" && time.frames() == 12);
    Q_ASSERT("frame to ticks" && time.ticks(12) == 12000);
    
    qDebug() << "ticks per frame: " << time.tpf();
    qDebug() << "ticks frames: " << time.frames();
    qDebug() << "frame to ticks: " << time.ticks(12);
    
    time.set_ticks(16016);
    time.set_timescale(30000);
    time.set_fps(AVFps::fps_29_97());
    Q_ASSERT("ticks to frame" && time.frames() == 16);
    
    qDebug() << "ticks frames: " << time.frames();
    
    time = AVTime::convert(time, 24000);
    Q_ASSERT("ticks to frame" && time.frames() == 16);
    Q_ASSERT("ticks to frame" && time.frame(time.ticks()) == 16);
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());

    qDebug() << "ticks: " << time.ticks();
    qDebug() << "ticks frames: " << time.frames();
    
    time = AVTime::convert(time, 30000);
    qDebug() << "ticks: " << time.ticks();
    Q_ASSERT("ticks" && time.ticks() == 16016);
    
    time = AVTime(time.ticks() + time.ticks(1), time.timescale(), time.fps());
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());
    
    time.set_ticks(8677230);
    time.set_timescale(90000);
    time.set_fps(AVFps::fps_23_976());
    
    qDebug() << "frames: " << time.frames();
    qDebug() << "ticks: " << time.ticks(time.frames() + 1);
    
    time.set_ticks(time.ticks(time.frames() + 1));
    qDebug() << "frames: " << time.frames();
}

void test_timerange() {
    AVTimeRange timerange;
    timerange.set_start(AVTime(12000, 24000, AVFps::fps_24()));
    AVTime duration = AVTime::convert(AVTime(384000, 48000, AVFps::fps_24()), timerange.start().timescale());
    timerange.set_duration(duration);
    Q_ASSERT("convert timescale" && duration.ticks() == 192000);
    Q_ASSERT("end ticks" && timerange.end().ticks() == 204000);
    
    qDebug() << "timerange: " << timerange.to_string();
}

void test_fps() {
    qDebug() << "Testing fps";
    
    Q_ASSERT("24 fps" &&  AVFps::fps_24() == AVFps(24, 1));
    qDebug() << "fps 24: " << AVFps::fps_24().seconds();
    qDebug() << "fps 24: " << AVFps(24, 1).seconds();
    qDebug() << "fps 24: " << 1/24.0;
    
    AVTime time;
    time.set_ticks(24000 * 100); // typical ticks, 100 seconds
    time.set_timescale(24000);
    time.set_fps(AVFps::fps_24());
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
    ticks = AVTime(time, fps23_97).ticks(1);
    Q_ASSERT("23.97 fps ticks" &&  ticks == 1001);
    qDebug() << "ticks 23_97: " << ticks;
    
    AVFps fps24 = AVFps::fps_24();
    ticks = AVTime(time, fps24).ticks(1);
    Q_ASSERT("24 fps ticks" &&  ticks == 1000);
    qDebug() << "ticks 24: " << ticks;
    
    AVFps fps29_97 = AVFps::fps_29_97();
    ticks = AVTime(time, fps29_97).ticks(2);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" &&  ticks == 1602);
    
    ticks = AVTime(time, fps29_97).ticks(5);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" &&  ticks == 4004);
}

void test_smpte() {
    qDebug() << "Testing SMPTE";
    
    AVFps fps_24 = AVFps::fps_24();
    qint64 frame = 86496; // typical timecode, 01:00:04:00, 24 fps
    AVTime time(frame, fps_24);
    Q_ASSERT("86496 frames is 3604" && qFuzzyCompare(time.seconds(), 3604));
    qDebug() << "time: " << time.seconds();
    
    qint64 frame_fps = frame;
    AVSmpteTime smpte(AVTime(frame_fps, AVFps::fps_24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_24(), AVFps::fps_50());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_50(), AVFps::fps_25());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_25()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_25(), AVFps::fps_50());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_50(), AVFps::fps_23_976());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_23_976()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04.00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_23_976(), AVFps::fps_50());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    
    frame_fps = AVSmpteTime::convert(frame_fps, AVFps::fps_50(), AVFps::fps_24());
    smpte = AVSmpteTime(AVTime(frame_fps, AVFps::fps_24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    qint64 frame_df_23_976 = AVSmpteTime::convert(frame, AVFps::fps_23_976());
    qint64 frame_24 = AVSmpteTime::convert(frame_df_23_976, AVFps::fps_23_976(), true); // inverse
    Q_ASSERT("86496 dropframe is 86388" && frame_df_23_976 == 86388);
    Q_ASSERT("dropframe inverse does not match" && frame == frame_24);
    
    smpte = AVSmpteTime(time);
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    qDebug() << "smpte 24 fps: " << smpte.to_string();
    
    qint64 frame_30 = AVFps::convert(frame, AVFps::fps_24(), AVFps::fps_30());
    smpte = AVSmpteTime(AVTime(frame_30, AVFps::fps_30()));
    Q_ASSERT("smpte is 01:00:04:00 for 30 fps" && smpte.to_string() == "01:00:04:00");
    qDebug() << "smpte 30 fps: " << smpte.to_string();
    
    qint64 frame_23_976 = AVSmpteTime::convert(frame_24, AVFps::fps_23_976());
    smpte = AVSmpteTime(AVTime(frame_23_976, AVFps::fps_23_976()));
    Q_ASSERT("smpte is 01:00:04.00 for 23.976 fps" && smpte.to_string() == "01:00:04.00");
    qDebug() << "smpte 23.976 fps: " << smpte.to_string();
    
    qint64 frame_29_997 = 440658;
    smpte = AVSmpteTime(AVTime(frame_29_997, AVFps::fps_29_97()));
    Q_ASSERT("smpte is 04:05:03.10" && smpte.to_string() == "04:05:03.10");
    qDebug() << "smpte 29_97 fps: " << smpte.to_string();

    frame_29_997 = 442698;
    smpte = AVSmpteTime(AVTime(frame_29_997, AVFps::fps_29_97()));
    Q_ASSERT("smpte is 04:06:11.12" && smpte.to_string() == "04:06:11.12");
    qDebug() << "smpte 29_97 fps: " << smpte.to_string();
    
    // quicktime data:
    // 01:00:04:00
    // 2542
    // 01:46
    // 01:01:49:22 (uses floor rather than round, 22583)
    // 23.976 fps
    time = AVTime(2544542, 24000, AVFps::fps_23_976());
    Q_ASSERT("time is 01:46" && time.to_string() == "01:46");
    Q_ASSERT("frames is 2542" && time.frames() == 2542);
    qDebug() << "time: " << time.to_string();
    qDebug() << "time frames: " << time.frames();
    
    frame = 2541;
    AVTime duration = AVTime(frame, AVFps::fps_23_976()); // 0-2542, 2541 last frame
    Q_ASSERT("frames is 2541" && duration.frames() == frame);
    qDebug()  << "time frames: " << duration.frames();
    
    frame = 86496;
    AVTime offset = AVTime(frame, AVFps::fps_24()); // typical timecode, 01:00:04:00, 24 fps
    qDebug()  << "offset max: " << offset.frames();
    qDebug()  << "offset smpte: " << AVSmpteTime(offset).to_string();
    
    frame = AVSmpteTime::convert(offset.frames(), AVFps::fps_23_976()); // fix it to 01:00:04:00, match 23.976 timecode, drop frame
    Q_ASSERT("drop frame is 86388" && frame == 86388);
    qDebug()  << "offset dropframe: " << frame;

    smpte = AVSmpteTime(AVTime(duration.frames() + frame, AVFps::fps_23_976()));
    Q_ASSERT("smpte is 01:01:49.23" && smpte.to_string() == "01:01:49.23");
    qDebug() << "smpte: " << smpte.to_string();
    
    // ffmpeg data:
    // time_base=1/24000
    // duration_ts=187903716
    // 7829.344000
    // 24000/1001
    // 2:10:29.344000
    time = AVTime(187903716, 24000, AVFps::fps_24()); // typical duration, 02:10:29.344000, 24 fps
    Q_ASSERT("seconds 7829.32" && qFuzzyCompare(time.seconds(), 7829.3215));
    qDebug() << "seconds: " << time.seconds();

    smpte = AVSmpteTime(time);
    Q_ASSERT("smpte is 02:10:29:08 for 24 fps" && smpte.to_string() == "02:10:29:08"); // *:08 equals 8/24 of a second = .344000
    qDebug() << "smpte 24 fps: " << smpte.to_string();
    
    // resolve data:
    // frame: 87040, converted to 87148 at fps: 23.967
    // 01:00:31:04
    // 01:00:30 (wall clock time)
    // 24 NDF is used in Resolve for 23.967 for timecode
    
    frame = 87040;
    time = AVTime(frame, AVFps::fps_23_976());
    Q_ASSERT("time is 01:00:30" && time.to_string() == "01:00:30");
    qDebug() << "time: " << time.to_string();
    
    smpte = AVSmpteTime(time);
    Q_ASSERT("smpte is 01:00:31.04 for 23.976 fps" && smpte.to_string() == "01:00:31.04");
    qDebug() << "smpte 23.976: " << smpte.to_string();
}

void test_timer() {
    qDebug() << "Testing timer";
    
    QFuture<void> future;
    future = QtConcurrent::run([] {
        QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
        AVFps fps = AVFps::fps_23_976();
        qint64 start = 1, duration = 24 * 400;
        AVTimeRange range(AVTime(start, fps), AVTime(duration, fps));
        AVTimer totaltimer;
        totaltimer.start();
        AVTimer timer;
        timer.start(fps);
        
        qDebug() << "range: start:" << range.start().frames() << ", duration: " << range.duration().frames();
        qint64 frames = range.duration().frames();
        
        qint64 dropped = 0;
        for(qint64 frame = range.start().frames(); frame < range.duration().frames(); frame++) {
            quint64 currenttime = timer.elapsed();
            quint64 delay = QRandomGenerator::global()->bounded(1, 80);
            timer.sleep(delay);
            timer.wait();
            
            qreal elapsed = AVTimer::convert(timer.elapsed() - currenttime, AVTimer::Unit::SECONDS);
            qreal deviation = elapsed - fps.seconds();
            
            qDebug() << "frame[" << frame << "/" << frames << "]:" << elapsed << "|"
                     << "deviation:" << deviation << "," << "%:" << (deviation / fps.seconds()) * 100 << ", delay: " << delay;
            
            while (!timer.next(fps)) {
                frame++;
                dropped++;
                qDebug() << "drop frame[" << frame << "] total frames dropped: " << dropped << ", previous delay: " << delay;
            }
        }
        totaltimer.stop();
        qreal elapsed = AVTimer::convert(totaltimer.elapsed(), AVTimer::Unit::SECONDS);
        qreal expected = range.duration().frames() * fps.seconds();
        qreal deviation = elapsed - expected;
        
        qDebug() << "total elapsed:" << elapsed << "|"
                << "expected:" << expected
                << "deviation:" << deviation << ", msecs: " << deviation * 1000 << "%:" << (deviation / expected) * 100
                << "dropped: " << dropped;

        Q_ASSERT("deviation more than 50 msecs" && qAbs(deviation) > 0.05);
    });
    future.waitForFinished();
}

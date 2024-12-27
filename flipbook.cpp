// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "avreader.h"
#include "platform.h"
#include "rhiwidget.h"
#include "timeline.h"

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QPointer>

#include <QtConcurrent>
#include <QtGlobal>

// generated files
#include "ui_flipbook.h"

class FlipbookPrivate : public QObject
{
    Q_OBJECT
    public:
        FlipbookPrivate();
        void init();
    
    public Q_SLOTS:
        void open();
        void seek(AVTime time);
        void seek_start();
        void seek_previous();
        void seek_next();
        void seek_end();
        void seek_frame(qint64 frame);
        void seek_time(const AVTime& time);
        void stream(bool checked);
        void stop();
    
        void set_opened(const QString& filename);
        void set_video(const QImage& image);
        void set_audio(const QByteArray& buffer);
        void set_time(const AVTime& time);
        void fullscreen(bool checked);
        void loop(bool checked);
    
        void power(Platform::Power power);
        void stayawake(bool checked);

    public:
        void run_open(const QString& filename) {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, filename] {
                    reader->open(filename);
                    reader->read();
                    reader->seek(reader->range().start()); // todo: make sure we reset after read()
                });
            } else {
                qWarning() << "could not open reader, thread already running";
            }
        }
        void run_seek(AVTime time) {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, time] {
                    reader->seek(time);
                    reader->read();
                });
            }
            else {
                qWarning() << "could not seek reader at time: " << time.to_string() << ", ticks, " << time.ticks() << ", frame: " << time.frame(reader->fps()) << ", thread already running";
            }
        }
        void run_stream() {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&] {
                    reader->stream();
                });
            }
            else {
                qWarning() << "could not run stream thread already running";
            }
        }
        void run_stop() {
            reader->stop();
            if (future.isRunning()) {
                future.waitForFinished();
            }
            else {
                qWarning() << "could not wait for finished, thread is not running";
            }
        }
    public:
        struct State {
            bool loop;
            bool stream;
            bool fullscreen;
        };
        State state;
        QFuture<void> future;
        QScopedPointer<AVReader> reader;
        QPointer<Flipbook> window;
        QScopedPointer<Platform> platform;
        QScopedPointer<Ui_Flipbook> ui;
};

FlipbookPrivate::FlipbookPrivate()
{
}

void
FlipbookPrivate::init()
{
    platform.reset(new Platform());
    // ui
    ui.reset(new Ui_Flipbook());
    ui->setupUi(window.data());
    window->setFocus();
    
    // reader
    reader.reset(new AVReader());
    
    // connect
    connect(ui->menu_open, &QAction::triggered, this, &FlipbookPrivate::open);
    connect(ui->menu_start, &QAction::triggered, this, &FlipbookPrivate::seek_start);
    connect(ui->menu_previous, &QAction::triggered, this, &FlipbookPrivate::seek_previous);
    connect(ui->menu_play, &QAction::triggered, this, &FlipbookPrivate::stream);
    connect(ui->menu_next, &QAction::triggered, this, &FlipbookPrivate::seek_next);
    connect(ui->menu_end, &QAction::triggered, this, &FlipbookPrivate::seek_end);
    connect(ui->menu_loop, &QAction::triggered, this, &FlipbookPrivate::loop);
    connect(ui->menu_fullscreen, &QAction::triggered, this, &FlipbookPrivate::fullscreen);
    
    connect(ui->tool_open, &QPushButton::pressed, this, &FlipbookPrivate::open);
    connect(ui->tool_start, &QPushButton::pressed, this, &FlipbookPrivate::seek_start);
    connect(ui->tool_previous, &QPushButton::pressed, this, &FlipbookPrivate::seek_previous);
    connect(ui->tool_play, &QPushButton::toggled, this, &FlipbookPrivate::stream);
    connect(ui->tool_next, &QPushButton::pressed, this, &FlipbookPrivate::seek_next);
    connect(ui->tool_end, &QPushButton::pressed, this, &FlipbookPrivate::seek_end);
    connect(ui->tool_loop, &QPushButton::toggled, this, &FlipbookPrivate::loop);
    connect(ui->tool_fullscreen, &QPushButton::toggled, this, &FlipbookPrivate::fullscreen);
    
    // timeline
    connect(ui->timeline, &Timeline::slider_pressed, this, &FlipbookPrivate::stop);
    connect(ui->timeline, &Timeline::slider_moved, this, &FlipbookPrivate::seek_time);
    
    // status
    connect(ui->stayawake, &QCheckBox::clicked, this, &FlipbookPrivate::stayawake);
    
    // reader
    connect(reader.data(), &AVReader::opened, this, &FlipbookPrivate::set_opened);
    connect(reader.data(), &AVReader::video_changed, this, &FlipbookPrivate::set_video);
    connect(reader.data(), &AVReader::audio_changed, this, &FlipbookPrivate::set_audio);
    connect(reader.data(), &AVReader::time_changed, this, &FlipbookPrivate::set_time);
    
    connect(reader.data(), &AVReader::stream_changed, ui->menu_play, &QAction::setChecked);
    connect(reader.data(), &AVReader::stream_changed, ui->tool_play, &QPushButton::setChecked);
    
    connect(reader.data(), &AVReader::loop_changed, ui->menu_loop, &QAction::setChecked);
    connect(reader.data(), &AVReader::loop_changed, ui->tool_loop, &QPushButton::setChecked);

    connect(reader.data(), &AVReader::time_changed, ui->timeline, &Timeline::set_time);
    
    // platform
    connect(platform.data(), &Platform::power_changed, this, &FlipbookPrivate::power);
}

void
FlipbookPrivate::open()
{
    QString filename = QFileDialog::getOpenFileName(
         window.data(),
         tr("Open QuickTime Movie"),
         QDir::homePath(),
         tr("QuickTime Movies (*.mov *.mp4);;All Files (*)")
    );
    if (!filename.isEmpty()) {
        run_open(filename);
    }
}

void
FlipbookPrivate::seek(AVTime time)
{
    if (reader->is_streaming()) {
        stop();
    }
    if (reader->error() == AVReader::NO_ERROR) {
        if (reader->time() != time) {
            run_seek(time);
        }
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::seek_start()
{
    seek(reader->range().start());
}

void
FlipbookPrivate::seek_previous()
{
    seek_frame(-1);
}

void
FlipbookPrivate::seek_next()
{
    seek_frame(1);
}

void
FlipbookPrivate::seek_end()
{
    seek(reader->range().end());
}

void
FlipbookPrivate::seek_frame(qint64 frame)
{
    AVTime time = reader->time();
    run_seek(AVTime(time.ticks() + time.ticks(frame, reader->fps()), time.timescale()));
}

void
FlipbookPrivate::seek_time(const AVTime& time)
{
    run_seek(time);
}

void
FlipbookPrivate::stream(bool checked)
{
    if (state.stream != checked) {
        state.stream = checked;
        if (state.stream) {
            run_stream();
        }
        else {
            reader->stop();
        }
    }
}

void
FlipbookPrivate::stop()
{
    run_stop();
}

void
FlipbookPrivate::fullscreen(bool checked)
{
    if (state.fullscreen != checked) {
        QLayout* layout = qobject_cast<QVBoxLayout*>(ui->central_widget->layout());
        if (checked) {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != ui->render_widget) {
                    widget->hide();
                }
            }
            window->showFullScreen();
        } else {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != ui->render_widget) {
                    widget->show();
                }
            }
            window->showNormal();
        }
        state.fullscreen = checked;
    }
}

void
FlipbookPrivate::loop(bool checked)
{
    if (state.loop != checked) {
        reader->set_loop(checked);
        state.loop = checked;
    }
}

void
FlipbookPrivate::power(Platform::Power power)
{
    qDebug() << "sleep signal recieved, will stop playback";
    
    if (power == Platform::POWEROFF || power == Platform::RESTART || power == Platform::SLEEP) {
        if (reader->is_streaming()) {
            run_stop();
        }
    }
}

void
FlipbookPrivate::stayawake(bool checked)
{
    if (checked) {
        platform->stayawake(checked);
    }
    else {
        platform->stayawake(checked);
    }
}

void
FlipbookPrivate::set_opened(const QString& filename)
{
    if (reader->error() == AVReader::NO_ERROR) {
        AVTimeRange range = reader->range();
        ui->timeline_start->setText(AVSmpteTime(range.start()).to_string());
        ui->timeline_duration->setText(AVSmpteTime(range.end()).to_string());
        ui->timeline->set_time(reader->time());
        ui->timeline->set_range(reader->range());
        ui->timeline->set_fps(reader->fps());
        ui->timeline->setEnabled(true);
        ui->fps->setValue(reader->fps());
        ui->playback_widget->setEnabled(true);
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::set_video(const QImage& image)
{
    if (reader->error() == AVReader::NO_ERROR) {
        QString title = reader->title();
        if (title.isEmpty()) {
            title = QFileInfo(reader->filename()).fileName();
        }
        window->setWindowTitle(QString("Flipbook: %1").arg(title));
        ui->rhi_widget->set_image(image);
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::set_audio(const QByteArray& buffer)
{
    if (reader->error() == AVReader::NO_ERROR) {
        // todo: we don't support audio streaming yet
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::set_time(const AVTime& time)
{
    ui->timecode->setText(AVSmpteTime(time).to_string());
    ui->frame->setText(QString("%1").arg(time.frame(reader->fps()), 4, 10, QChar('0')));
}

#include "flipbook.moc"

Flipbook::Flipbook(QWidget* parent)
: QMainWindow(parent)
, p(new FlipbookPrivate())
{
    p->window = this;
    p->init();
}

Flipbook::~Flipbook()
{
}

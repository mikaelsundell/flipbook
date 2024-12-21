// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "avreader.h"
#include "mac.h"
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
        void start();
        void previous();
        void play(bool checked);
        void next();
        void end();
        void increment(qint64 frame);
        void fullscreen(bool checked);
        void video(const QImage& image);
        void opened(const QString& filename);
        void finished();
        void timecoded(const AVTime& time);

    Q_SIGNALS:
        void play_changed(bool checked);
        void fullscreen_changed(bool checked);

    public:
        void run_open(const QString& filename) {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, filename] {
                    reader->open(filename);
                    reader->read();
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
                qWarning() << "could not seek reader at time: " << time.to_string() << ", thread already running";
            }
        }
        void run_stream() {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&] {
                    reader->stream();
                });
            }
            else {
                qWarning() << "could not play reader, thread already running";
            }
        }
    public:
        class State {
            public:
                bool stream;
                bool fullscreen;
        };
        State state;
        QFuture<void> future;
        QScopedPointer<AVReader> reader;
        QPointer<Flipbook> window;
        QScopedPointer<Ui_Flipbook> ui;
};

FlipbookPrivate::FlipbookPrivate()
{
}

void
FlipbookPrivate::init()
{
    mac::setDarkAppearance();
    // ui
    ui.reset(new Ui_Flipbook());
    ui->setupUi(window.data());
    window->setFocus();
    // reader
    reader.reset(new AVReader());
    
    // connect
    connect(ui->open, &QAction::triggered, this, &FlipbookPrivate::open);
    connect(ui->start, &QAction::triggered, this, &FlipbookPrivate::start);
    connect(ui->previous, &QAction::triggered, this, &FlipbookPrivate::previous);
    connect(ui->play, &QAction::triggered, this, &FlipbookPrivate::play);
    connect(ui->next, &QAction::triggered, this, &FlipbookPrivate::next);
    connect(ui->end, &QAction::triggered, this, &FlipbookPrivate::end);
    
    connect(ui->tool_open, &QPushButton::pressed, this, &FlipbookPrivate::open);
    connect(ui->tool_start, &QPushButton::pressed, this, &FlipbookPrivate::start);
    connect(ui->tool_previous, &QPushButton::pressed, this, &FlipbookPrivate::previous);
    connect(ui->tool_play, &QPushButton::toggled, this, &FlipbookPrivate::play);
    connect(ui->tool_next, &QPushButton::pressed, this, &FlipbookPrivate::next);
    connect(ui->tool_end, &QPushButton::pressed, this, &FlipbookPrivate::end);
    
    connect(this, &FlipbookPrivate::play_changed, ui->play, &QAction::setChecked);
    connect(this, &FlipbookPrivate::play_changed, ui->tool_play, &QPushButton::setChecked);
    
    connect(reader.data(), &AVReader::opened, this, &FlipbookPrivate::opened);
    connect(reader.data(), &AVReader::time_changed, this, &FlipbookPrivate::timecoded);
    connect(reader.data(), &AVReader::time_changed, ui->timeline, &Timeline::set_time);
    
    connect(reader.data(), &AVReader::video_changed, this, &FlipbookPrivate::video);
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
    if (!reader->is_streaming()) {
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
}

void
FlipbookPrivate::start()
{
    seek(reader->range().start());
}

void
FlipbookPrivate::play(bool checked)
{
    qDebug() << "play called by: " << sender();
    
    if (state.stream != checked) {
        
        qDebug() << "- state was changed to: " << checked;
        
        state.stream = checked;
        if (state.stream) {
            run_stream();
        }
        else {
            reader->abort();
        }
    }
    else {
        qDebug() << "- state ignored for now";
    }
    play_changed(checked);
}

void
FlipbookPrivate::previous()
{
    increment(-1);
}

void
FlipbookPrivate::next()
{
    increment(1);
}

void
FlipbookPrivate::end()
{
}

void
FlipbookPrivate::increment(qint64 frame)
{
    AVTime time = reader->time();
    AVTimeRange timerange = reader->range();
    qreal fps = reader->fps();
    run_seek(timerange.bound(AVTime(time.ticks() + frame * time.to_ticks_frame(fps), time.timescale())));
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
        fullscreen_changed(checked);
    }
}

void
FlipbookPrivate::opened(const QString& filename)
{
    if (reader->error() == AVReader::NO_ERROR) {
        AVTimeRange range = reader->range();
        ui->timeline_start->setText(QString("Start: %1").arg(range.start().ticks()));
        ui->timeline_duration->setText(QString("Duration: %1").arg(range.duration().ticks()));
        ui->timeline->set_time(reader->time());
        ui->timeline->set_range(reader->range());
        ui->timeline->setEnabled(true);
        ui->fps->setValue(reader->fps());
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::finished()
{
    play_changed(false);
}

void
FlipbookPrivate::video(const QImage& image)
{
    if (reader->error() == AVReader::NO_ERROR) {
        window->setWindowTitle(QString("Flipbook: %1: %2")
                               .arg(QFileInfo(reader->filename()).fileName())
                               .arg(reader->time().to_string())
                               );
        ui->rhi_widget->set_image(image);
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipbookPrivate::timecoded(const AVTime& time)
{
    ui->timecode->setText(AVSmpteTime(time).to_string());
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

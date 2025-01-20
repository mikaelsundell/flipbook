// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipman.h"
#include "avreader.h"
#include "platform.h"
#include "rhiwidget.h"
#include "timeline.h"

#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QPointer>

#include <QtConcurrent>
#include <QtGlobal>

// generated files
#include "ui_flipman.h"

class FlipmanPrivate : public QObject
{
    Q_OBJECT
    public:
        FlipmanPrivate();
        void init();
        bool eventFilter(QObject* object, QEvent* event);
    
    public Q_SLOTS:
        void open();
        void seek(AVTime time);
        void seek_start();
        void seek_previous();
        void seek_next();
        void seek_end();
        void seek_frame(qint64 frame);
        void seek_time(const AVTime& time);
        void seek_finished();
        void stream(bool checked);
        void stop();
    
        void set_opened(const QString& filename);
        void set_video(const QImage& image);
        void set_audio(const QByteArray& buffer);
        void set_time(const AVTime& time);
        void set_smptetime(const AVSmpteTime& smptetime);
        void set_actual_fps(float fps);
    
        void fullscreen(bool checked);
        void loop(bool checked);
        void everyframe(bool everyframe);
        void frames();
        void time();
        void smpte();
    
        void power(Platform::Power power);
        void stayawake(bool checked);
    
        void debug();

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
                watcher.setFuture(future);
            }
            state.seek = time;
        }
        void run_stream() {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&] {
                    reader->stream();
                });
            }
            else {
                qWarning() << "could not run stream, thread already running";
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
            bool everyframe;
            bool stream;
            bool fullscreen;
            bool ready = false;
            AVTime seek;
        };
        State state;
        QStringList arguments;
        QFuture<void> future;
        QFutureWatcher<void> watcher;
        QScopedPointer<AVReader> reader;
        QPointer<Flipman> window;
        QScopedPointer<Platform> platform;
        QScopedPointer<Ui_Flipman> ui;
};

FlipmanPrivate::FlipmanPrivate()
{
}

void
FlipmanPrivate::init()
{
    platform.reset(new Platform());
    // ui
    ui.reset(new Ui_Flipman());
    ui->setupUi(window.data());
    window->setFocus();
    window->installEventFilter(this);
    // reader
    reader.reset(new AVReader());
    // connect
    connect(ui->menu_open, &QAction::triggered, this, &FlipmanPrivate::open);
    connect(ui->menu_start, &QAction::triggered, this, &FlipmanPrivate::seek_start);
    connect(ui->menu_previous, &QAction::triggered, this, &FlipmanPrivate::seek_previous);
    connect(ui->menu_play, &QAction::triggered, this, &FlipmanPrivate::stream);
    connect(ui->menu_next, &QAction::triggered, this, &FlipmanPrivate::seek_next);
    connect(ui->menu_end, &QAction::triggered, this, &FlipmanPrivate::seek_end);
    connect(ui->menu_loop, &QAction::triggered, this, &FlipmanPrivate::loop);
    connect(ui->menu_fullscreen, &QAction::triggered, this, &FlipmanPrivate::fullscreen);
    connect(ui->tool_open, &QPushButton::pressed, this, &FlipmanPrivate::open);
    connect(ui->tool_start, &QPushButton::pressed, this, &FlipmanPrivate::seek_start);
    connect(ui->tool_previous, &QPushButton::pressed, this, &FlipmanPrivate::seek_previous);
    connect(ui->tool_play, &QPushButton::toggled, this, &FlipmanPrivate::stream);
    connect(ui->tool_next, &QPushButton::pressed, this, &FlipmanPrivate::seek_next);
    connect(ui->tool_end, &QPushButton::pressed, this, &FlipmanPrivate::seek_end);
    connect(ui->tool_loop, &QPushButton::toggled, this, &FlipmanPrivate::loop);
    connect(ui->tool_everyframe, &QCheckBox::clicked, this, &FlipmanPrivate::everyframe);
    connect(ui->tool_fullscreen, &QPushButton::toggled, this, &FlipmanPrivate::fullscreen);
    connect(ui->timeline_frames, &QAction::triggered, this, &FlipmanPrivate::frames);
    connect(ui->timeline_time, &QAction::triggered, this, &FlipmanPrivate::time);
    connect(ui->timeline_smpte, &QAction::triggered, this, &FlipmanPrivate::smpte);
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        {
            actions->addAction(ui->timeline_frames);
            actions->addAction(ui->timeline_time);
            actions->addAction(ui->timeline_smpte);
        }
    }
    // timeline
    connect(ui->timeline, &Timeline::slider_pressed, this, &FlipmanPrivate::stop);
    connect(ui->timeline, &Timeline::slider_moved, this, &FlipmanPrivate::seek_time);
    // status
    connect(ui->stayawake, &QCheckBox::clicked, this, &FlipmanPrivate::stayawake);
    // debug
    connect(ui->debug, &QCheckBox::clicked, this, &FlipmanPrivate::debug);
    // watchers
    connect(&watcher, &QFutureWatcher<void>::finished, this, &FlipmanPrivate::seek_finished);
    // reader
    connect(reader.data(), &AVReader::opened, this, &FlipmanPrivate::set_opened);
    connect(reader.data(), &AVReader::video_changed, this, &FlipmanPrivate::set_video);
    connect(reader.data(), &AVReader::audio_changed, this, &FlipmanPrivate::set_audio);
    connect(reader.data(), &AVReader::time_changed, this, &FlipmanPrivate::set_time);
    connect(reader.data(), &AVReader::smptetime_changed, this, &FlipmanPrivate::set_smptetime);
    connect(reader.data(), &AVReader::actual_fps_changed, this, &FlipmanPrivate::set_actual_fps);
    connect(reader.data(), &AVReader::stream_changed, ui->menu_play, &QAction::setChecked);
    connect(reader.data(), &AVReader::stream_changed, ui->tool_play, &QPushButton::setChecked);
    connect(reader.data(), &AVReader::loop_changed, ui->menu_loop, &QAction::setChecked);
    connect(reader.data(), &AVReader::loop_changed, ui->tool_loop, &QPushButton::setChecked);
    connect(reader.data(), &AVReader::time_changed, ui->timeline, &Timeline::set_time);
    // platform
    connect(platform.data(), &Platform::power_changed, this, &FlipmanPrivate::power);
}

bool
FlipmanPrivate::eventFilter(QObject* object, QEvent* event)
{
    static bool dragging = false;
    static QPointF position;
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mousevent = static_cast<QMouseEvent*>(event);
        if (mousevent->button() == Qt::LeftButton) {
            dragging = true;
            position = mousevent->globalPosition() - window->frameGeometry().topLeft();
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove) {
        if (dragging) {
            QMouseEvent* mousevent = static_cast<QMouseEvent*>(event);
            window->move((mousevent->globalPosition() - position).toPoint());
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        dragging = false;
        return true;
    }
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (window->isMaximized()) {
            window->showNormal();
        }
        else {
            window->showMaximized();
        }
        return true;
    }
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        int delta = wheelEvent->angleDelta().y();
        if (delta > 0) {
            seek_next();
        } else if (delta < 0) {
            seek_previous();
        }
        return true;
    }
    else if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasUrls()) {
            dragEvent->acceptProposedAction();
        }
        return true;
    }
    else if (event->type() == QEvent::Drop) {
        QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
        if (dropEvent->mimeData()->hasUrls()) {
            QList<QUrl> urls = dropEvent->mimeData()->urls();
            for (const QUrl& url : urls) {
                QString filepath = url.toLocalFile();
                if (!filepath.isEmpty()) {
                    if (reader->is_supported(QFileInfo(filepath).suffix())) {
                        stop();
                        run_open(filepath);
                    }
                    else {
                        qWarning() << "warning: file format not supported: " << filepath;
                    }
                }
            }
        }
        return true;
    }
    else if (event->type() == QEvent::Show) {
        if (!state.ready) {
            if (arguments.contains("--open")) {
                qsizetype index = arguments.indexOf("--open");
                if (index != -1 && index + 1 < arguments.size()) {
                    QString filepath = arguments.at(index + 1);
                    if (!filepath.isEmpty()) {
                        run_open(filepath);
                    }
                }
            }
            state.ready = true;
        }
        return true;
    }
    else if (event->type() == QEvent::Close) {
        stop();
        return true;
    }
    return QObject::eventFilter(object, event);
}

void
FlipmanPrivate::open()
{
    QString filename = QFileDialog::getOpenFileName(
         window.data(),
         tr("Open QuickTime Movie"),
         QDir::homePath(),
         tr("QuickTime Movies (*.mov *.mp4);;All Files (*)")
    );
    if (!filename.isEmpty()) {
        stop();
        run_open(filename);
    }
}

void
FlipmanPrivate::seek(AVTime time)
{
    stop();
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
FlipmanPrivate::seek_start()
{
    seek(reader->range().start());
}

void
FlipmanPrivate::seek_previous()
{
    seek_frame(-1);
}

void
FlipmanPrivate::seek_next()
{
    seek_frame(1);
}

void
FlipmanPrivate::seek_end()
{
    seek(reader->range().end());
}

void
FlipmanPrivate::seek_frame(qint64 frame)
{
    AVTime time = reader->time();
    run_seek(AVTime(time.ticks(time.frames() + frame), time.timescale(), time.fps()));
}

void
FlipmanPrivate::seek_time(const AVTime& time)
{
    run_seek(time);
}

void
FlipmanPrivate::seek_finished()
{
    if (state.seek.valid()) {
        if (state.seek.frames() != reader->time().frames()) {
            run_seek(state.seek);
        }
        state.seek.invalidate();
    }
}

void
FlipmanPrivate::stream(bool checked)
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
FlipmanPrivate::stop()
{
    if (reader->is_streaming()) {
        run_stop();
    }
}

void
FlipmanPrivate::fullscreen(bool checked)
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
FlipmanPrivate::loop(bool checked)
{
    if (state.loop != checked) {
        reader->set_loop(checked);
        state.loop = checked;
    }
}

void
FlipmanPrivate::everyframe(bool checked)
{
    if (state.everyframe != checked) {
        reader->set_everyframe(checked);
        state.everyframe = checked;
    }
}

void
FlipmanPrivate::frames()
{
    ui->timeline->set_timecode(Timeline::FRAMES);
    set_time(reader->time());
}

void
FlipmanPrivate::time()
{
    ui->timeline->set_timecode(Timeline::TIME);
    set_time(reader->time());
}

void
FlipmanPrivate::smpte()
{
    ui->timeline->set_timecode(Timeline::SMPTE);
    set_time(reader->time());
}

void
FlipmanPrivate::power(Platform::Power power)
{
    if (power == Platform::POWEROFF || power == Platform::RESTART || power == Platform::SLEEP) {
        stop();
    }
}

void
FlipmanPrivate::stayawake(bool checked)
{
    if (checked) {
        platform->stayawake(checked);
    }
    else {
        platform->stayawake(checked);
    }
}

void
FlipmanPrivate::debug()
{
    // todo: place holder for debug tests
}

void
FlipmanPrivate::set_opened(const QString& filename)
{
    if (reader->error() == AVReader::NO_ERROR) {
        AVTimeRange range = reader->range();
        ui->df->setChecked(reader->fps().drop_frame());
        ui->fps->setValue(reader->fps());
        ui->actual_fps->setText(QString::number(reader->fps()));
        ui->playback_widget->setEnabled(true);
        ui->timeline_start->setText(range.start().to_string());
        ui->timeline_duration->setText(range.end().to_string());
        ui->timeline->set_time(reader->time());
        ui->timeline->set_range(reader->range());
        ui->timeline->setEnabled(true);
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipmanPrivate::set_video(const QImage& image)
{
    if (reader->error() == AVReader::NO_ERROR) {
        QString title = reader->title();
        if (title.isEmpty()) {
            title = QFileInfo(reader->filename()).fileName();
        }
        window->setWindowTitle(QString("Flipman: %1").arg(title));
        {
            int width = image.width();
            int height = image.height();
            QString format;
            switch (image.format()) {
                case QImage::Format_ARGB32: format = "ARGB"; break;
                case QImage::Format_RGB32: format = "RGB"; break;
                case QImage::Format_Grayscale8: format = "Grayscale"; break;
                case QImage::Format_RGB888: format = "RGB"; break;
                case QImage::Format_RGBA8888: format = "RGBA"; break;
                default: format = "Unknown"; break;
            }
            ui->info->setText(QString("%1x%2 %3 %4-bit").arg(width).arg(height).arg(format).arg(image.depth()));
        }
        ui->rhi_widget->set_image(image);
    }
    else {
        ui->status->setText(reader->error_message());
        qWarning() << "warning: " << ui->status->text();
    }
}

void
FlipmanPrivate::set_audio(const QByteArray& buffer)
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
FlipmanPrivate::set_time(const AVTime& time)
{
    ui->frame->setText(QString("%1").arg(time.frames(), 4, 10, QChar('0')));
    if (ui->timeline->timecode() == Timeline::Timecode::FRAMES) {
        ui->timeline_start->setText(QString::number(time.frames()));
        ui->timeline_duration->setText(QString::number(reader->range().duration().frames()));
    }
    else if (ui->timeline->timecode() == Timeline::Timecode::TIME) {
        ui->timeline_start->setText(time.to_string());
        ui->timeline_duration->setText(reader->range().duration().to_string());
    }
    else if (ui->timeline->timecode() == Timeline::Timecode::SMPTE) {
        ui->timeline_start->setText(AVSmpteTime(time).to_string());
        ui->timeline_duration->setText(AVSmpteTime(reader->range().duration()).to_string());
    }
}

void
FlipmanPrivate::set_smptetime(const AVSmpteTime& smptetime)
{
    ui->smptetime->setText(smptetime.to_string());
}

void
FlipmanPrivate::set_actual_fps(float fps)
{
    if (fps < reader->fps()) {
        ui->actual_fps->setText(QString("*%1").arg(QString::number(fps, 'f', 3)));
    }
    else {
        ui->actual_fps->setText(QString("%1").arg(QString::number(fps, 'f', 3)));
    }
}

#include "flipman.moc"

Flipman::Flipman(QWidget* parent)
: QMainWindow(parent)
, p(new FlipmanPrivate())
{
    p->window = this;
    p->init();
}

Flipman::~Flipman()
{
}

void
Flipman::set_arguments(const QStringList& arguments)
{
    p->arguments = arguments;
}

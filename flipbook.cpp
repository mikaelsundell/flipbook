// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "avstream.h"
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

class FlipbookPrivate : public QObject
{
    Q_OBJECT
    public:
        FlipbookPrivate();
        void init();
    
    public Q_SLOTS:
        void open();
        void seek(AVTime time);
        void play(bool checked);
        void next();
        void previous();
        void increment(qint64 frame);
        void fullscreen(bool checked);
        void image(const QImage& image);
        void opened(const QString& filename);
        void timecoded(const AVTime& time);

    Q_SIGNALS:
        void play_changed(bool checked);
        void fullscreen_changed(bool checked);

    public:
        void run_open(const QString& filename) {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, filename] {
                    stream->open(filename);
                    stream->read();
                });
            } else {
                qWarning() << "could not open stream, thread already running";
            }
        }
        void run_seek(AVTime time) {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, time] {
                    stream->seek(time);
                    stream->read();
                });
            }
            else {
                qWarning() << "could not seek stream at time: " << time.to_string() << ", thread already running";
            }
        }
        void run_play() {
            if (!future.isRunning()) {
                future = QtConcurrent::run([&] {
                    stream->play();
                });
            }
            else {
                qWarning() << "could not play stream, thread already running";
            }
        }
    public:
        class State {
            public:
                bool play;
                bool fullscreen;
        };
        State state;
        QWidget* toolswidget;
        QWidget* timelinewidget;
        QWidget* renderwidget;
        QWidget* statuswidget;
        QDoubleSpinBox* fps;
        QLabel* timecode;
        QLabel* start;
        QLabel* duration;
        QLabel* in;
        QLabel* out;
        QLabel* status;
        Timeline* timeline;
        QVBoxLayout* layout;
        RhiWidget* rhiwidget;
        QFuture<void> future;
        QScopedPointer<AVStream> stream;
        QPointer<Flipbook> window;
};

FlipbookPrivate::FlipbookPrivate()
{
}

void
FlipbookPrivate::init()
{
    mac::setDarkAppearance();
    stream.reset(new AVStream());
    connect(stream.data(), &AVStream::opened, this, &FlipbookPrivate::opened);
    connect(stream.data(), &AVStream::image_changed, this, &FlipbookPrivate::image);
    {
        QMenu* file = window->menuBar()->addMenu("&File");
        {
            QAction* open = new QAction("Open ...");
            open->setCheckable(true);
            open->setShortcut(Qt::ControlModifier | Qt::Key_O);
            file->addAction(open);
            
            connect(open, &QAction::triggered, this, &FlipbookPrivate::open);
        }
        QMenu* playback = window->menuBar()->addMenu("&Playback");
        {
            QAction* usecache = new QAction("Use cache");
            usecache->setCheckable(true);
            playback->addAction(usecache);
            playback->addSeparator();
            
            QAction* gotostart = new QAction("Start");
            gotostart->setCheckable(true);
            gotostart->setShortcut(Qt::Key_Down);
            playback->addAction(gotostart);
            
            QAction* previous = new QAction("Previous");
            previous->setCheckable(true);
            previous->setShortcut(Qt::Key_Left);
            playback->addAction(previous);
            
            QAction* play = new QAction("Play");
            play->setCheckable(true);
            play->setShortcut(Qt::Key_Space);
            playback->addAction(play);
            
            QAction* next = new QAction("Next");
            next->setCheckable(true);
            next->setShortcut(Qt::Key_Right);
            playback->addAction(next);
            
            QAction* end = new QAction("End");
            end->setShortcut(Qt::Key_Up);
            playback->addAction(end);
            playback->addSeparator();
            
            QAction* in = new QAction("In");
            in->setCheckable(true);
            in->setShortcut(Qt::Key_I);
            playback->addAction(in);
            
            QAction* out = new QAction("Out");
            out->setCheckable(true);
            out->setShortcut(Qt::Key_O);
            playback->addAction(out);
            playback->addSeparator();
            
            QAction* loop = new QAction("Loop");
            loop->setCheckable(true);
            loop->setShortcut(Qt::Key_L);
            playback->addAction(loop);
            playback->addSeparator();
            
            playback->addSeparator();
            QAction* fullscreen = new QAction("Fullscreen");
            fullscreen->setCheckable(true);
            fullscreen->setShortcut(Qt::Key_F);
            playback->addAction(fullscreen);
            
            connect(previous, &QAction::triggered, this, &FlipbookPrivate::previous);
            connect(play, &QAction::triggered, this, &FlipbookPrivate::play);
            connect(next, &QAction::triggered, this, &FlipbookPrivate::next);
            connect(fullscreen, &QAction::triggered, this, &FlipbookPrivate::fullscreen);
            connect(this, &FlipbookPrivate::play_changed, play, &QAction::setChecked);
            connect(this, &FlipbookPrivate::fullscreen_changed, play, &QAction::setChecked);
        }
    }
    QWidget* centralwidget = new QWidget(window.data());
    layout = new QVBoxLayout(centralwidget);
    centralwidget->setLayout(layout);

    toolswidget = new QWidget();
    {
        QHBoxLayout* toolslayout = new QHBoxLayout(toolswidget);
        QPushButton* open = new QPushButton("Open", toolswidget);
        QPushButton* start = new QPushButton("Start", toolswidget);
        QPushButton* previous = new QPushButton("Previous", toolswidget);
        QPushButton* play = new QPushButton("Play", toolswidget);
        play->setCheckable(true);
        QPushButton* next = new QPushButton("Next", toolswidget);
        QPushButton* end = new QPushButton("End", toolswidget);
        QPushButton* loop = new QPushButton("Loop", toolswidget);
        loop->setCheckable(true);
        QPushButton* in = new QPushButton("In", toolswidget);
        QPushButton* out = new QPushButton("Out", toolswidget);
        QPushButton* sound = new QPushButton("Sound", toolswidget);
        timecode = new QLabel("00:00:00:00", toolswidget);
        timecode->setMinimumWidth(80);
        QLabel* ratelabel = new QLabel("FPS", toolswidget);
        fps = new QDoubleSpinBox(toolswidget);
        fps->setMinimum(1.0);
        fps->setMaximum(120.0);
        fps->setMinimumWidth(80);
        QLabel* memlabel = new QLabel("MEM", toolswidget);
        QDoubleSpinBox* mem = new QDoubleSpinBox(toolswidget);
        mem->setValue(1024.0);
        mem->setMinimum(1.0);
        mem->setMaximum(2048.0);
        mem->setMinimumWidth(80);
        QPushButton* fullscreen = new QPushButton("Fullscreen", toolswidget);
        fullscreen->setCheckable(true);
        toolslayout->addWidget(open);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(start);
        toolslayout->addWidget(previous);
        toolslayout->addWidget(play);
        toolslayout->addWidget(next);
        toolslayout->addWidget(end);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(loop);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(in);
        toolslayout->addWidget(out);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(sound);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(timecode);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(ratelabel);
        toolslayout->addWidget(fps);
        toolslayout->addWidget(memlabel);
        toolslayout->addWidget(mem);
        toolslayout->addStretch(1);
        toolslayout->addWidget(fullscreen);
        layout->addWidget(toolswidget);

        connect(open, &QPushButton::pressed, this, &FlipbookPrivate::open);
        connect(previous, &QPushButton::pressed, this, &FlipbookPrivate::previous);
        connect(play, &QPushButton::toggled, this, &FlipbookPrivate::play);
        connect(next, &QPushButton::pressed, this, &FlipbookPrivate::next);
        connect(fullscreen, &QPushButton::toggled, this, &FlipbookPrivate::fullscreen);
        connect(this, &FlipbookPrivate::play_changed, play, &QPushButton::setChecked);
        connect(this, &FlipbookPrivate::fullscreen_changed, fullscreen, &QPushButton::setChecked);
        connect(stream.data(), &AVStream::time_changed, this, &FlipbookPrivate::timecoded);
    }
    timelinewidget = new QWidget();
    {
        QHBoxLayout* timelinelayout = new QHBoxLayout(timelinewidget);
        start = new QLabel("Start: 0", timelinewidget);
        in = new QLabel("In: 0", timelinewidget);
        timeline = new Timeline(timelinewidget);
        timeline->setEnabled(true);
        out = new QLabel("Out: 0", timelinewidget);
        duration = new QLabel("Duration: 0", timelinewidget);
        timelinelayout->addWidget(start);
        timelinelayout->addWidget(in);
        timelinelayout->addWidget(timeline);
        timelinelayout->addWidget(out);
        timelinelayout->addWidget(duration);
        layout->addWidget(timelinewidget);
        
        //connect(timeline, &Timeline::time_changed, this, &FlipbookPrivate::seek);
        //connect(timeline, &Timeline::timecode_changed, this, &FlipbookPrivate::timecoded);
        connect(stream.data(), &AVStream::time_changed, timeline, &Timeline::set_time);
    }
    renderwidget = new QWidget();
    {
        QHBoxLayout* renderlayout = new QHBoxLayout(renderwidget);
        rhiwidget = new RhiWidget(renderwidget);
        renderlayout->addWidget(rhiwidget);
        layout->addWidget(renderwidget);
    }
    statuswidget = new QWidget();
    {
        QHBoxLayout* statuslayout = new QHBoxLayout(statuswidget);
        status = new QLabel("Ready", statuswidget);
        QLabel* zoom = new QLabel("Zoom: 0, 0", statuswidget);
        QLabel* data = new QLabel("Data: 0, 0, 0", statuswidget);
        QLabel* display = new QLabel("Display: 0, 0, 0", statuswidget);
        QLabel* format = new QLabel("Format: Float32", statuswidget);
        QLabel* colorspace = new QLabel("Colorspace: Linear", statuswidget);
        QLabel* colordisplay = new QLabel("Display: Rec709", statuswidget);
        QPushButton* lut = new QPushButton("LUT", statuswidget);
        QPushButton* icc = new QPushButton("ICC", statuswidget);
        statuslayout->addWidget(status);
        statuslayout->addWidget(zoom);
        statuslayout->addWidget(data);
        statuslayout->addWidget(display);
        statuslayout->addStretch(1);
        statuslayout->addWidget(format);
        statuslayout->addWidget(colorspace);
        statuslayout->addWidget(colordisplay);
        statuslayout->addWidget(lut);
        statuslayout->addWidget(icc);
        layout->addWidget(statuswidget);
    }
    layout->setStretch(0, 0);
    layout->setStretch(1, 0);
    layout->setStretch(2, 1);
    layout->setStretch(3, 0);
    window->setCentralWidget(centralwidget);
    window->resize(960, 600);
    window->setWindowTitle("Flipbook");
    window->setFocus();
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
    if (!stream->is_playing()) {
        if (stream->error() == AVStream::NO_ERROR) {
            if (stream->time() != time) {
                run_seek(time);
            }
        }
        else {
            status->setText(stream->error_message());
            qWarning() << "warning: " << status->text();
        }
    }
}

void
FlipbookPrivate::play(bool checked)
{
    if (state.play != checked) {
        state.play = checked;
        if (state.play) {
            run_play();
        }
        else {
            stream->stop();
        }
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
FlipbookPrivate::increment(qint64 frame)
{
    AVTime time = stream->time();
    AVTimeRange timerange = stream->range();
    qreal fps = stream->fps();
    run_seek(timerange.bound(AVTime(time.ticks() + frame * time.to_ticks_frame(fps), time.timescale())));
}

void
FlipbookPrivate::fullscreen(bool checked)
{
    if (state.fullscreen != checked) {
        if (checked) {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != renderwidget) {
                    widget->hide();
                }
            }
            window->showFullScreen();
        } else {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != renderwidget) {
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
    if (stream->error() == AVStream::NO_ERROR) {
        AVTimeRange range = stream->range();
        start->setText(QString("Start: %1").arg(range.start().ticks()));
        duration->setText(QString("Duration: %1").arg(range.duration().ticks()));
        timeline->set_time(stream->time());
        timeline->set_range(stream->range());
        timeline->setEnabled(true);
        fps->setValue(stream->fps());
    }
    else {
        status->setText(stream->error_message());
        qWarning() << "warning: " << status->text();
    }
}

void
FlipbookPrivate::image(const QImage& image)
{
    if (stream->error() == AVStream::NO_ERROR) {
        window->setWindowTitle(QString("Flipbook: %1: %2")
                               .arg(QFileInfo(stream->filename()).fileName())
                               .arg(stream->time().to_string())
                               );
        rhiwidget->set_image(image);
    }
    else {
        status->setText(stream->error_message());
        qWarning() << "warning: " << status->text();
    }
}

void
FlipbookPrivate::timecoded(const AVTime& time)
{
    timecode->setText(AVSmpteTime(time).to_string());
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

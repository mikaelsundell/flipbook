// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "rhiwidget.h"
#include "avstream.h"

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
        void seek(int frame);
        void play(bool checked);
        void next();
        void previous();
        void fullscreen(bool checked);
        
        void image(const QImage& image);
        void opened(const QString& filename);

    
    Q_SIGNALS:
        void play_changed(bool checked);
        void fullscreen_changed(bool checked);

    public:
        void process() {
            QFuture<void> future = QtConcurrent::run([&] {
                std::lock_guard<std::mutex> lock(mutex);
                stream->start();
            });
        }
        std::mutex mutex;
    
    
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
        QDoubleSpinBox* rate;
        QLabel* begin;
        QLabel* end;
        QLabel* in;
        QLabel* out;
        QLabel* status;
        QSlider* timeline;
        QVBoxLayout* layout;
        RhiWidget* rhiwidget;
        QScopedPointer<AVStream> stream;
        QPointer<Flipbook> window;
};

FlipbookPrivate::FlipbookPrivate()
{
}

void
FlipbookPrivate::init()
{
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
        QLabel* time = new QLabel("00:00:00:00", toolswidget);
        QLabel* ratelabel = new QLabel("FPS", toolswidget);
        rate = new QDoubleSpinBox(toolswidget);
        rate->setMinimum(1.0);
        rate->setMaximum(120.0);
        rate->setMinimumWidth(80);
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
        toolslayout->addWidget(time);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(ratelabel);
        toolslayout->addWidget(rate);
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
    }
    timelinewidget = new QWidget();
    {
        QHBoxLayout* timelinelayout = new QHBoxLayout(timelinewidget);
        begin = new QLabel("0", timelinewidget);
        in = new QLabel("(0)", timelinewidget);
        timeline = new QSlider(Qt::Horizontal, timelinewidget);
        timeline->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        timeline->setEnabled(false);
        out = new QLabel("(0)", timelinewidget);
        end = new QLabel("0", timelinewidget);
        timelinelayout->addWidget(begin);
        timelinelayout->addWidget(in);
        timelinelayout->addWidget(timeline);
        timelinelayout->addWidget(out);
        timelinelayout->addWidget(end);
        timelinelayout->setStretch(2, 3);
        layout->addWidget(timelinewidget);
        
        connect(timeline, &QSlider::valueChanged, this, &FlipbookPrivate::seek);
        connect(stream.data(), &AVStream::frame_changed, timeline, &QSlider::setValue);
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
         tr("QuickTime Movies (*.mov);;All Files (*)")
    );
    if (!filename.isEmpty()) {
        stream->set_filename(filename);
        stream->set_stream(false);
        process();
    }
}

void
FlipbookPrivate::seek(int value)
{
    if (stream->error() == AVStream::NO_ERROR) {
        if (stream->frame() != value) {
            stream->set_stream(false);
            stream->set_frame(value);
            process();
        }
    }
    else {
        status->setText(stream->error_message());
        qWarning() << "warning: " << status->text();
    }
}

void
FlipbookPrivate::play(bool checked)
{
    if (state.play != checked) {
        state.play = checked;
        if (state.play) {
            stream->set_stream(true);
            process();
        }
        else {
            stream->stop();
        }
        play_changed(checked);
    }
}

void
FlipbookPrivate::previous()
{
    if (stream->is_open()) {
        seek(qBound(stream->frame() - 1, stream->start_frame(), stream->end_frame()));
    }
}

void
FlipbookPrivate::next()
{
    if (stream->is_open()) {
        seek(qBound(stream->frame() + 1, stream->start_frame(), stream->end_frame()));
    }
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
        begin->setText(QString::number(stream->start_frame()));
        end->setText(QString::number(stream->end_frame()));
        in->setText(QString("(%1)").arg(stream->in_frame()));
        out->setText(QString("(%2)").arg(stream->out_frame()));
        timeline->setValue(stream->start_frame());
        timeline->setMinimum(stream->start_frame());
        timeline->setMaximum(stream->end_frame());
        timeline->setEnabled(true);
        rate->setValue(stream->rate());
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
                               .arg(stream->frame())
                               );
        rhiwidget->set_image(image);
    }
    else {
        status->setText(stream->error_message());
        qWarning() << "warning: " << status->text();
    }
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

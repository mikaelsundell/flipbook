// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "rhi_widget.h"
#include "qt_stream.h"

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
#include <QtGlobal>

class flipbook_private : public QObject
{
    Q_OBJECT
    public:
        flipbook_private();
        void init();
    
    public Q_SLOTS:
        void open();
        void seek(int frame);
        void play(bool checked);
        void next();
        void previous();
        void fullscreen(bool checked);
    
    Q_SIGNALS:
        void startChanged(int frame);
        void endChanged(int end);
        void playChanged(bool checked);
        void frameChanged(int frame);
        void fullscreenChanged(bool checked);

    public:
        class State {
            public:
                bool play;
                bool fullscreen;
                int frame;
        };
        State state;
        QWidget* toolswidget;
        QWidget* timelinewidget;
        QWidget* renderwidget;
        QWidget* statuswidget;
        QDoubleSpinBox* fps;
        QLabel* start;
        QLabel* end;
        QSlider* timeline;
        QVBoxLayout* layout;
        rhi_widget* rhiwidget;
        QScopedPointer<qt_stream> stream;
        QPointer<flipbook> window;
};

flipbook_private::flipbook_private()
{
}

void
flipbook_private::init()
{
    QMenu* file = window->menuBar()->addMenu("&File");
    {
        QAction* open = new QAction("Open ...");
        open->setCheckable(true);
        open->setShortcut(Qt::ControlModifier | Qt::Key_O);
        file->addAction(open);
        connect(open, &QAction::triggered, this, &flipbook_private::open);
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
        
        QAction* gotoend = new QAction("End");
        gotoend->setShortcut(Qt::Key_Up);
        playback->addAction(gotoend);
        playback->addSeparator();
        
        QAction* gotoin = new QAction("Go to in");
        gotoin->setCheckable(true);
        gotoin->setShortcut(Qt::Key_I);
        playback->addAction(gotoin);
        
        QAction* gotoout = new QAction("Go to out");
        gotoout->setCheckable(true);
        gotoout->setShortcut(Qt::Key_O);
        playback->addAction(gotoout);
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
        
        connect(previous, &QAction::triggered, this, &flipbook_private::previous);
        connect(play, &QAction::triggered, this, &flipbook_private::play);
        connect(next, &QAction::triggered, this, &flipbook_private::next);
        connect(fullscreen, &QAction::triggered, this, &flipbook_private::fullscreen);
        connect(this, &flipbook_private::playChanged, play, &QAction::setChecked);
        connect(this, &flipbook_private::fullscreenChanged, play, &QAction::setChecked);
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
        QLabel* fpslabel = new QLabel("FPS", toolswidget);
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
        toolslayout->addWidget(time);
        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            toolslayout->addWidget(line);
        }
        toolslayout->addWidget(fpslabel);
        toolslayout->addWidget(fps);
        toolslayout->addWidget(memlabel);
        toolslayout->addWidget(mem);
        toolslayout->addStretch(1);
        toolslayout->addWidget(fullscreen);
        layout->addWidget(toolswidget);
        stream.reset(new qt_stream());
        // connect
        connect(open, &QPushButton::pressed, this, &flipbook_private::open);
        connect(previous, &QPushButton::pressed, this, &flipbook_private::previous);
        connect(play, &QPushButton::toggled, this, &flipbook_private::play);
        connect(next, &QPushButton::pressed, this, &flipbook_private::next);
        connect(fullscreen, &QPushButton::toggled, this, &flipbook_private::fullscreen);
        connect(this, &flipbook_private::playChanged, play, &QPushButton::setChecked);
        connect(this, &flipbook_private::fullscreenChanged, fullscreen, &QPushButton::setChecked);
    }
    timelinewidget = new QWidget();
    {
        QHBoxLayout* timelinelayout = new QHBoxLayout(timelinewidget);
        start = new QLabel("0", timelinewidget);
        timeline = new QSlider(Qt::Horizontal, timelinewidget);
        timeline->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        timeline->setEnabled(false);
        end = new QLabel("0", timelinewidget);
        timelinelayout->addWidget(start);
        timelinelayout->addWidget(timeline);
        timelinelayout->addWidget(end);
        timelinelayout->setStretch(1, 2);
        layout->addWidget(timelinewidget);
        
        connect(timeline, &QSlider::valueChanged, this, &flipbook_private::seek);
        connect(this, &flipbook_private::frameChanged, timeline, &QSlider::setValue);
    }
    renderwidget = new QWidget();
    {
        QHBoxLayout* renderlayout = new QHBoxLayout(renderwidget);
        rhiwidget = new rhi_widget(renderwidget);
        renderlayout->addWidget(rhiwidget);
        layout->addWidget(renderwidget);
    }
    statuswidget = new QWidget();
    {
        QHBoxLayout* statuslayout = new QHBoxLayout(statuswidget);
        QLabel* zoom = new QLabel("Zoom: 0, 0", statuswidget);
        QLabel* data = new QLabel("Data: 0, 0, 0", statuswidget);
        QLabel* display = new QLabel("Display: 0, 0, 0", statuswidget);
        QLabel* format = new QLabel("Format: Float32", statuswidget);
        QLabel* colorspace = new QLabel("Colorspace: Linear", statuswidget);
        QLabel* colordisplay = new QLabel("Display: Rec709", statuswidget);
        QPushButton* lut = new QPushButton("LUT", statuswidget);
        QPushButton* icc = new QPushButton("ICC", statuswidget);
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
flipbook_private::open()
{
    QString filename = QFileDialog::getOpenFileName(
         window.data(),
         tr("Open QuickTime Movie"),
         QDir::homePath(),
         tr("QuickTime Movies (*.mov);;All Files (*)")
    );
    if (!filename.isEmpty()) {
        if (stream->open(filename)) {
            stream->seek(stream->start());
            QImage image = stream->fetch();
            if (!image.isNull()) {
                window->setWindowTitle(QString("Flipbook: %1: %2")
                    .arg(QFileInfo(filename).fileName())
                    .arg(stream->start())
                );
                start->setText(QString::number(stream->start()));
                end->setText(QString::number(stream->end()));
                timeline->setEnabled(true);
                timeline->setValue(stream->start());
                timeline->setMinimum(stream->start());
                timeline->setMaximum(stream->end());
                fps->setValue(stream->fps());
                rhiwidget->set_image(image);
            }
            else {
                qWarning() << "error: could not read start frame from quicktime file: " << filename;
            }
        }
        else {
            qWarning() << "error: could not open quicktime file: " << filename;
        }
    }
}

void
flipbook_private::seek(int value)
{
    if (state.frame != value) {
        if (stream->is_open()) {
            stream->seek(value);
            QImage image = stream->fetch();
            if (!image.isNull()) {
                window->setWindowTitle(QString("Flipbook: %1: %2")
                                       .arg(QFileInfo(stream->filename()).fileName())
                                       .arg(value)
                                       );
                rhiwidget->set_image(image);
            }
            else {
                qWarning() << "warning: could not read frame " << value << " from quicktime file: " << stream->filename();
            }
            state.frame = value; // seek frame before fetch
            frameChanged(value);
        }
        else {
            qWarning() << "warning: no quicktime file is open for reading";
        }
    }
}

void
flipbook_private::play(bool checked)
{
    if (state.play != checked) {
        state.play = checked;
        playChanged(checked);
    }
}

void
flipbook_private::fullscreen(bool checked)
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
        fullscreenChanged(checked);
    }
}

void
flipbook_private::previous()
{
    if (stream->is_open()) {
        seek(qBound(state.frame - 1, stream->start(), stream->end()));
    }
}

void
flipbook_private::next()
{
    if (stream->is_open()) {
        seek(qBound(state.frame + 1, stream->start(), stream->end()));
    }
}

#include "flipbook.moc"

flipbook::flipbook(QWidget* parent)
: QMainWindow(parent)
, p(new flipbook_private())
{
    p->window = this;
    p->init();
}

flipbook::~flipbook()
{
}

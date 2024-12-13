// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "rhi_widget.h"
#include "qt_reader.h"

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

class flipbook_private : public QObject
{
    Q_OBJECT
    public:
        flipbook_private();
        void init();
    
    public Q_SLOTS:
        void open();
        void play(bool checked);
        void fullscreen(bool checked);
        void timelineChanged(int value);

    public:
        qt_reader reader;
        QWidget* toolswidget;
        QWidget* timelinewidget;
        QWidget* renderwidget;
        QWidget* statuswidget;
        QDoubleSpinBox* fps;
        QSlider* timeline;
        rhi_widget* rhiwidget;
        QVBoxLayout* layout;
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
        open->setShortcut(Qt::Key_O);
        file->addAction(open);
        connect(open, &QAction::triggered, this, &flipbook_private::open);
    }
    QMenu* playback = window->menuBar()->addMenu("&Playback");
    {
        QAction* play = new QAction("Play");
        play->setCheckable(true);
        play->setShortcut(Qt::Key_Space);
        playback->addAction(play);
        connect(play, &QAction::triggered, this, &flipbook_private::play);

        QAction* fullscreen = new QAction("Fullscreen");
        fullscreen->setCheckable(true);
        fullscreen->setShortcut(Qt::Key_F);
        playback->addAction(fullscreen);
        connect(fullscreen, &QAction::triggered, this, &flipbook_private::fullscreen);
    }
    QWidget* centralwidget = new QWidget(window.data());
    layout = new QVBoxLayout(centralwidget);
    centralwidget->setLayout(layout);

    toolswidget = new QWidget();
    {
        QHBoxLayout* toolslayout = new QHBoxLayout(toolswidget);
        QPushButton* open = new QPushButton("Open", toolswidget);
        QPushButton* backward = new QPushButton("Backward", toolswidget);
        QPushButton* play = new QPushButton("Play", toolswidget);
        play->setCheckable(true);
        QPushButton* forward = new QPushButton("Forward", toolswidget);
        QPushButton* loop = new QPushButton("Loop", toolswidget);
        loop->setCheckable(true);
        QPushButton* in = new QPushButton("In", toolswidget);
        QPushButton* out = new QPushButton("Out", toolswidget);
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
        toolslayout->addWidget(backward);
        toolslayout->addWidget(play);
        toolslayout->addWidget(forward);
        toolslayout->addWidget(loop);
        toolslayout->addWidget(in);
        toolslayout->addWidget(out);
        toolslayout->addWidget(fpslabel);
        toolslayout->addWidget(fps);
        toolslayout->addWidget(memlabel);
        toolslayout->addWidget(mem);
        toolslayout->addStretch(1);
        toolslayout->addWidget(fullscreen);
        layout->addWidget(toolswidget);
        connect(open, &QPushButton::pressed, this, &flipbook_private::open);
        connect(play, &QPushButton::toggled, this, &flipbook_private::play);
        connect(fullscreen, &QPushButton::toggled, this, &flipbook_private::fullscreen);
    }
    timelinewidget = new QWidget();
    {
        QHBoxLayout* timelinelayout = new QHBoxLayout(timelinewidget);
        timeline = new QSlider(Qt::Horizontal, timelinewidget);
        timeline->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        timeline->setEnabled(false);
        timelinelayout->addWidget(timeline);
        timelinelayout->setStretch(0, 1);
        layout->addWidget(timelinewidget);
        connect(timeline, &QSlider::valueChanged, this, &flipbook_private::timelineChanged);
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
        if (reader.open(filename)) {
            QImage image = reader.frame(reader.start());
            if (!image.isNull()) {
                window->setWindowTitle(QString("Flipbook: %1: %2")
                    .arg(QFileInfo(filename).fileName())
                    .arg(reader.start())
                );
                fps->setValue(reader.fps());
                timeline->setEnabled(true);
                timeline->setMinimum(reader.start());
                timeline->setMaximum(reader.end());
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
flipbook_private::play(bool checked)
{
    qDebug() << "Play checked: " << checked;
}

void
flipbook_private::fullscreen(bool checked)
{
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
}

void
flipbook_private::timelineChanged(int value)
{
    if (reader.is_open()) {
        QImage image = reader.frame(value);
        if (!image.isNull()) {
            window->setWindowTitle(QString("Flipbook: %1: %2")
                .arg(QFileInfo(reader.filename()).fileName())
                .arg(value)
            );
            rhiwidget->set_image(image);
        }
        else {
            qWarning() << "warning: could not read frame " << value << " from quicktime file: " << reader.filename();
        }
    }
    else {
        qWarning() << "warning: no quicktime file is open for reading";
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

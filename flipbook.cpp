// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "rhi_widget.h"
#include "qt_reader.h"

#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
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
        void valueChanged(int value);

    public:
        qt_reader reader;
        rhi_widget* rhiwidget;
        QSlider* slider;
        QVBoxLayout* layout;
        QPointer<flipbook> window;
};

flipbook_private::flipbook_private()
{
}

void
flipbook_private::init()
{
    QWidget* centralwidget = new QWidget(window.data());
    layout = new QVBoxLayout(centralwidget);
    centralwidget->setLayout(layout);

    QWidget* buttonRow = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonRow);
    QPushButton* open = new QPushButton("Open", buttonRow);
    QPushButton* backward = new QPushButton("Backward", buttonRow);
    QPushButton* toggle = new QPushButton("Play", buttonRow);
    QPushButton* forward = new QPushButton("Forward", buttonRow);
    QPushButton* fullscreen = new QPushButton("Fullscreen", buttonRow);
    buttonLayout->addWidget(open);
    buttonLayout->addWidget(backward);
    buttonLayout->addWidget(toggle);
    buttonLayout->addWidget(forward);
    buttonLayout->addStretch(1); 
    buttonLayout->addWidget(fullscreen);
    layout->addWidget(buttonRow);
    
    QWidget* sliderRow = new QWidget();
    QHBoxLayout* sliderLayout = new QHBoxLayout(sliderRow);
    slider = new QSlider(Qt::Horizontal, sliderRow);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sliderLayout->addWidget(slider);
    sliderLayout->setStretch(0, 1);
    sliderRow->setLayout(sliderLayout);
    layout->addWidget(sliderRow);
    
    rhiwidget = new rhi_widget(window.data());
    layout->addWidget(rhiwidget);

    QWidget* labelRow = new QWidget();
    QHBoxLayout* labelLayout = new QHBoxLayout(labelRow);
    QLabel* label = new QLabel("Frame: 0", labelRow);
    labelLayout->addWidget(label);
    layout->addWidget(labelRow);
    
    layout->setStretch(0, 0);
    layout->setStretch(1, 0);
    layout->setStretch(2, 1);
    layout->setStretch(3, 0);

    window->setCentralWidget(centralwidget);
    window->resize(800, 600);
    
    // connect
    connect(open, &QPushButton::pressed, this, &flipbook_private::open);
    connect(slider, &QSlider::valueChanged, this, &flipbook_private::valueChanged);
}

void
flipbook_private::open()
{
    QString filepath = QFileDialog::getOpenFileName(
         window.data(),
         tr("Open QuickTime Movie"),
         QDir::homePath(),
         tr("QuickTime Movies (*.mov);;All Files (*)")
    );

    if (!filepath.isEmpty()) {
        if (reader.open(filepath)) {
            QImage image = reader.frame(reader.start());
            if (!image.isNull()) {
                qDebug() << "info: reading start frame successful";
                slider->setMinimum(reader.start());
                slider->setMaximum(reader.end());
                rhiwidget->setImage(image);
            }
            else {
                qWarning() << "error: could not read start frame from quicktime file: " << filepath;
            }
        }
        else {
            qWarning() << "error: could not open quicktime file: " << filepath;
        }
    }
}

void
flipbook_private::valueChanged(int value)
{
    QImage image = reader.frame(reader.start());
    if (!image.isNull()) {
        rhiwidget->setImage(image);
    }
    else {
        qWarning() << "error: could not read frame " << value << " from quicktime file: " << reader.filename();
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

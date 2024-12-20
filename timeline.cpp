// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "timeline.h"
#include "avsmptetime.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

class TimelinePrivate
{
    public:
        TimelinePrivate();
        void init();

    public:
        int to_x(qint64 tick);
        qint64 to_time(int x);
        QPixmap paint();
        AVTime time;
        AVTimeRange range;
        Timeline::Units units;
        bool pressed;
        int margin;
        int radius;
        QPointer<Timeline> widget;
};

TimelinePrivate::TimelinePrivate()
: margin(14)
, radius(6)
, units(Timeline::Units::TIME)
{
}

void
TimelinePrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

int
TimelinePrivate::to_x(qint64 tick)
{
    qreal ratio = static_cast<qreal>(tick) / range.duration().time();
    return margin + ratio * (widget->width() - 2 * margin);
}

qint64
TimelinePrivate::to_time(int x)
{
    x = qBound(margin, x, widget->width() - margin);
    qreal ratio = static_cast<qreal>(x - margin) / (widget->width() - 2 * margin);
    return static_cast<qint64>(ratio * (range.duration().time() - 1));
}

QPixmap
TimelinePrivate::paint()
{
    qreal dpr = widget->devicePixelRatio();
    QPixmap pixmap(widget->width() * dpr, widget->height() * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);
    // painter
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    int y = widget->height() / 2;
    p.setPen(QPen(Qt::lightGray, 1));
    p.drawLine(0, y, widget->width(), y);
    QPen linePen(QPen(Qt::white, 2));
    p.setPen(linePen);
    p.drawLine(margin, y, widget->width() - margin, y);
    QFont font = p.font();
    font.setPointSizeF(font.pointSizeF() * 0.8f);
    p.setFont(font);
    if (range.valid()) {
        qint64 pos = time.time();
        qint64 start = range.start().time();
        qint64 duration = range.duration().time();
        int interval = qMax(1, int(qRound(duration * 0.1)) / 10 * 10);
        p.setPen(QPen(Qt::white, 1));
        for (qint64 tick = start; tick <= duration; tick += interval) {
            int x = to_x(tick);
            p.drawLine(x, y - 4, x, y + 4);
            QString text = QString::number(tick);
            QFontMetrics metrics(font);
            int width = metrics.horizontalAdvance(text);
            int min = to_x(start);
            int max = to_x(duration) - width;
            int textx = qBound(min, x - width / 2, max);
            int texty = y + 4;
            p.drawText(textx, texty + metrics.height(), text);
        }
        int x = to_x(pos);
        if (pressed) {
            QString text = QString::number(pos);
            QFontMetrics metrics(font);
            int pad = 4;
            int textw = metrics.horizontalAdvance(QString().fill('0', text.size())) + 2 * pad;
            int texth = metrics.height() + 2 * pad;
            int min = to_x(start) + textw / 2 - margin / 2;
            int max = to_x(duration) - textw / 2 + margin / 2;
            int bound = qBound(min, x, max);
            QRect rect(bound - textw / 2, y - texth / 2, textw, texth);
            p.setPen(Qt::black);
            p.setBrush(Qt::white);
            p.drawRect(rect);
            p.drawText(rect, Qt::AlignCenter, text);
            widget->timecode_changed(AVSmpteTime(AVTime(pos, range.duration().timescale())));
        }
        else {
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::white);
            p.drawEllipse(QPoint(x, y), radius, radius);
        }
    }
    
    return pixmap;
}

#include "timeline.moc"

Timeline::Timeline(QWidget* parent)
: QWidget(parent)
, p(new TimelinePrivate())
{
    p->widget = this;
    p->init();

}

Timeline::~Timeline()
{
}

QSize
Timeline::sizeHint() const
{
    return QSize(200, 40);
}

Timeline::Units
Timeline::units() const
{
    return p->units;
}

void
Timeline::set_range(const AVTimeRange& range)
{
    if (p->range != range) {
        p->range = range;
        update();
    }
}

void
Timeline::set_time(const AVTime& time)
{
    if (p->time != time) {
        p->time = time;
        time_changed(time);
        update();
    }
}

void
Timeline::set_units(Timeline::Units units)
{
    if (p->units != units) {
        p->units = units;
        update();
    }
}

void
Timeline::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->paint());
    painter.end();
    QWidget::paintEvent(event);
}

void
Timeline::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::BlankCursor);
        qint64 pos = p->to_time(event->pos().x());
        p->time.set_time(pos);
        p->pressed = true;
        slider_pressed();
        update();
    }
}

void
Timeline::mouseMoveEvent(QMouseEvent* event)
{
    if (p->pressed) {
        qint64 pos = p->to_time(event->pos().x());
        p->time.set_time(pos);
        time_changed(p->time);
        update();
    }
}

void
Timeline::mouseReleaseEvent(QMouseEvent* event)
{
    if (p->pressed && event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
        p->pressed = false;
        slider_released();
        update();
    }
}

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
        int to_pos(qint64 ticks);
        qint64 to_ticks(int x);
        QPixmap paint();
        AVTime time;
        AVTimeRange range;
        Timeline::Units units;
        qint64 lasttick;
        qreal fps = 0.0;
        bool tracking;
        bool pressed;
        int margin;
        int radius;
        QPointer<Timeline> widget;
};

TimelinePrivate::TimelinePrivate()
: margin(14)
, radius(2)
, units(Timeline::Units::TIME)
, lasttick(-1)
, tracking(false)
, pressed(false)
{
}

void
TimelinePrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

int
TimelinePrivate::to_pos(qint64 ticks)
{
    qreal ratio = static_cast<qreal>(ticks) / range.duration().ticks();
    return margin + ratio * (widget->width() - 2 * margin);
}

qint64
TimelinePrivate::to_ticks(int x)
{
    x = qBound(margin, x, widget->width() - margin);
    qreal ratio = static_cast<qreal>(x - margin) / (widget->width() - 2 * margin);
    return static_cast<qint64>(ratio * (range.duration().ticks() - 1));
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
    QFont font("Courier New", 11); // fixed font size
    p.setFont(font);
    int y = widget->height() / 2;
    p.setPen(QPen(Qt::lightGray, 1));
    p.drawLine(0, y, widget->width(), y);
    QPen linePen(QPen(Qt::white, 2));
    p.setPen(linePen);
    p.drawLine(margin, y, widget->width() - margin, y);
    font.setPointSizeF(font.pointSizeF() * 0.8f);
    p.setFont(font);
    if (range.valid()) {
        qint64 ticks = time.ticks();
        qint64 start = range.start().ticks();
        qint64 duration = range.duration().ticks();
        int interval = qMax(1, int(qRound(duration * 0.1)) / 10 * 10);
        p.setPen(QPen(Qt::white, 1));
        for (qint64 tick = start; tick < duration; tick += interval) {
            int x = to_pos(tick);
            p.drawLine(x, y - 4, x, y + 4);
            //QString text = QString("%1/%2").arg(QString::number(tick)).arg(AVTime(tick, time.timescale()).frames(fps));
            QString text = QString("%1").arg(AVTime(tick, time.timescale()).frame(fps));
            
            QFontMetrics metrics(font);
            int width = metrics.horizontalAdvance(text);
            int min = to_pos(start);
            int max = to_pos(duration) - width;
            int textx = qBound(min, x - width / 2, max);
            int texty = y + 4;
            p.drawText(textx, texty + metrics.height(), text);
        }
        {   // start
            int pos = to_pos(start);
            QPen thickPen(Qt::white, 4); // 3 is the width of the line
            p.save();
            p.setPen(thickPen);
            p.drawLine(pos, y - 4, pos, y + 4);
            p.restore();
        }
        {   // duration
            int pos = to_pos(duration);
            QPen thickPen(Qt::white, 4); // 3 is the width of the line
            p.save();
            p.setPen(thickPen);
            p.drawLine(pos, y - 4, pos, y + 4);
            //QString text = QString("%1/%2").arg(QString::number(duration)).arg(AVTime(duration, time.timescale()).frames(fps));
            QString text = QString("%1").arg(AVTime(duration, time.timescale()).frame(fps));
            
            QFontMetrics metrics(font);
            int width = metrics.horizontalAdvance(text);
            int min = to_pos(start);
            int max = to_pos(duration) - width;
            int textx = qBound(min, pos - width / 2, max);
            int texty = y + 4;
            p.drawText(textx, texty + metrics.height(), text);
            p.restore();
        }
        // todo: we save this for later, box showing the current time, timecode or frame
        //if (pressed) {
            /*
            //QString text = QString("%1/%2").arg(QString::number(pos)).arg(AVTime(pos, time.timescale()).frames(fps));
            QString text = QString("%1").arg(AVTime(ticks, time.timescale()).frame(fps));
            
            QFontMetrics metrics(font);
            int pad = 4;
            int textw = metrics.horizontalAdvance(QString().fill('0', text.size())) + 2 * pad;
            int texth = metrics.height() + 2 * pad;
            int min = to_pos(start) + textw / 2 - margin / 2;
            int max = to_pos(duration) - textw / 2 + margin / 2;
            int bound = qBound(min, pos, max);
            QRect rect(bound - textw / 2, y - texth / 2, textw, texth);
            p.setPen(Qt::black);
            p.setBrush(Qt::white);
            p.drawRect(rect);
            p.drawText(rect, Qt::AlignCenter, text);
            widget->timecode_changed(AVSmpteTime(AVTime(ticks, range.duration().timescale())));
            */
        //}
        //else {
            int pos = 0;
            AVTime next = AVTime(ticks + time.tpf(fps), time.timescale());
            if (next.ticks() < duration) {
                pos = to_pos(next.ticks());
                p.setPen(QPen(Qt::red, 2));
                p.setBrush(Qt::red);
                p.drawLine(pos, y - 2, pos, y + 2);
            }
            pos = to_pos(ticks);
            p.setPen(QPen(Qt::white, 2));
            p.setBrush(Qt::white);
            p.drawLine(pos, y - 4, pos, y + 4);
            
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::red);
            p.drawEllipse(QPoint(pos, y), radius, radius);
        //}
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

AVTimeRange
Timeline::range() const
{
    return p->range;
}

AVTime
Timeline::time() const
{
    return p->time;
}

qreal
Timeline::fps() const
{
    return p->fps;
}

bool
Timeline::tracking() const
{
    return p->tracking;
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
    if (p->time != time && !p->pressed) { // todo: ignore when pressed
        p->time = time;
        time_changed(time);
        update();
    }
}

void
Timeline::set_fps(qreal fps)
{
    if (p->fps != fps) {
        p->fps = fps;
        update();
    }
}

void
Timeline::set_tracking(bool tracking)
{
    if (p->tracking != tracking) {
        p->tracking = tracking;
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
        setCursor(Qt::SizeHorCursor);
        qint64 pos = p->to_ticks(event->pos().x());
        p->time.set_ticks(pos);
        p->pressed = true;
        slider_pressed();
        update();
    }
}

void
Timeline::mouseMoveEvent(QMouseEvent* event)
{
    if (p->pressed) {
        qint64 tick = p->to_ticks(event->pos().x());
        if (p->lasttick != tick) {
            p->time.set_ticks(tick);
            slider_moved(p->time);
            if (p->tracking) {
                time_changed(p->time);
            }
            p->lasttick = tick;
            update();
        }
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

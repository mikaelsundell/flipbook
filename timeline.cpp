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
        int mapToX(qint64 ticks) const;
        qint64 mapToTicks(int x) const;
        int mapToWidth(qint64 ticks) const;
        QSize mapToSize(const QFont& font, const QString& text) const;
        int mapToText(qint64 start, qint64 duration, int x, int width) const;
        QString labeltick(qint64 ticks) const;
        QSize labelsize(const QFont& font, qint64 ticks) const;
        qint64 ticks(qint64 value) const;
        qint64 steps(qint64 value);
        qint64 substeps(qint64 steps) const;
        qint64 substeps(qint64 value, qint64 max, qint64 steps, int width) const;
        qint64 labelsteps(qint64 steps) const;
        void paint_tick(QPainter& p, int x, int y, int height, qreal width = 1, QBrush brush = Qt::white);
        void paint_text(QPainter& p, int x, int y, qint64 value, qint64 start, qint64 duration, bool bold = false, QBrush brush = Qt::white);
        void paint_timeline(QPainter& p);
        QPixmap paint();
        struct Time
        {
            qint64 top;
            qint64 hours;
            qint64 minutes;
            qint64 seconds;
        };
        Time t;
        struct Data
        {
            AVTime time;
            AVTimeRange range;
            Timeline::Timecode timecode = Timeline::Timecode::TIME;
            qint64 lasttick = -1;
            qreal margintick = 0.5;
            int marginrange = 10;
            int disttick = 5;
            bool tracking = false;
            bool pressed = false;
            int radius = 2;
        };
        Data d;
        QPointer<Timeline> widget;
};

TimelinePrivate::TimelinePrivate()
{
}

void
TimelinePrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

int
TimelinePrivate::mapToX(qint64 ticks) const
{
    qreal ratio = static_cast<qreal>(ticks) / d.range.duration().ticks();
    return qRound(d.marginrange + ratio * (widget->width() - 2 * d.marginrange));
}

qint64
TimelinePrivate::mapToTicks(int x) const
{
    x = qBound(d.marginrange, x, widget->width() - d.marginrange);
    qreal ratio = static_cast<qreal>(x - d.marginrange) / (widget->width() - 2 * d.marginrange);
    return static_cast<qint64>(ratio * (d.range.duration().ticks() - 1));
}

int
TimelinePrivate::mapToWidth(qint64 ticks) const
{
    return mapToX(ticks) - mapToX(0);
}

QSize
TimelinePrivate::mapToSize(const QFont& font, const QString& text) const
{
    QFontMetrics metrics = QFontMetrics(font);
    return QSize(metrics.horizontalAdvance(text), metrics.height());
}

int
TimelinePrivate::mapToText(qint64 start, qint64 duration, int x, int width) const
{
    int min = mapToX(start);
    int max = mapToX(duration) - width;
    return (x - width / 2); // todo: needed? qBound(min, x - width / 2, max);
}

QString
TimelinePrivate::labeltick(qint64 value) const
{
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        return QString::number(d.time.frame(value));
    }
    else if (d.timecode == Timeline::Timecode::TIME) {
        return d.range.duration().to_string(value);
    }
    else if (d.timecode == Timeline::Timecode::SMPTE) {
        return AVSmpteTime(AVTime(d.range.duration(), value)).to_string();
    }
}

QSize
TimelinePrivate::labelsize(const QFont& font, qint64 value) const
{
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        return mapToSize(font, QString::number(d.range.duration().frames()));
    }
    else if (d.timecode == Timeline::Timecode::TIME) {
        return mapToSize(font, d.range.duration().to_string(value));
    }
    else if (d.timecode == Timeline::Timecode::SMPTE) {
        return mapToSize(font, AVSmpteTime(AVTime(d.range.duration(), value)).to_string());
    }
}

qint64
TimelinePrivate::ticks(qint64 value) const
{
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        return d.range.duration().ticks(value);
    }
    else if (d.timecode == Timeline::Timecode::TIME || d.timecode == Timeline::Timecode::SMPTE) {
        return d.range.duration().timescale() * value;
    }
}

qint64
TimelinePrivate::steps(qint64 value) {
    return pow(10, qint64(log10(value)));
}

qint64
TimelinePrivate::substeps(qint64 value, qint64 top, qint64 steps, int limit) const
{
    qint64 max = top * ticks(1);
    for (qint64 tick = steps; tick < max; tick += steps) {
        qint64 substeps = (tick / ticks(1));
        if (top % substeps == 0) {
            if (mapToWidth(tick) * d.margintick > limit) {
                return tick;
            }
        }
    }
    return steps;
}

void
TimelinePrivate::paint_tick(QPainter& p, int x, int y, int height, qreal width, QBrush brush) {
    p.save();
    p.setPen(QPen(brush, width));
    p.drawLine(x, y - height, x, y + height);
    p.restore();
}

void
TimelinePrivate::paint_text(QPainter& p, int x, int y, qint64 value, qint64 start, qint64 duration, bool bold, QBrush brush) {
    p.save();
    p.setPen(QPen(brush, 1));
    if (bold) {
        QFont font = p.font();
        font.setBold(true);
        font.setPointSize(8);
        p.setFont(font);
    }
    else {
        QFont font = p.font();
        font.setPointSize(7);
        p.setFont(font);
    }
    QString text = labeltick(value);
    QSize size = mapToSize(p.font(), text);
    p.drawText(mapToText(start, duration, x, size.width()), y + 4 + size.height(), text);
    p.restore();
}

void
TimelinePrivate::paint_timeline(QPainter& p)
{
    p.save();
    QFont font("Courier New", 8); // fixed font size
    p.setFont(font);
    p.setPen(QPen(Qt::white, 1));
    int y = widget->height() / 2;
    
    qint64 start = d.range.start().ticks();
    qint64 duration = d.range.duration().ticks();
    int limit = labelsize(font, duration).width();
    
    qint64 hoursteps = ticks(t.hours);
    if (hoursteps) {
        qint64 hoursubsteps = 0;
        if (!(mapToWidth(hoursteps) * d.margintick > limit)) {
            hoursubsteps = substeps(t.hours, t.top, hoursteps, limit);
        }
        for(qint64 hour = start; hour < duration; hour += hoursteps) {
            int x = mapToX(hour);
            if (hoursubsteps > 0) {
                if (hour % hoursubsteps == 0) {
                    if (mapToWidth(hoursubsteps) > d.disttick) {
                        paint_tick(p, x, y, 2, 2);
                    }
                    if (mapToWidth(hoursubsteps) * d.margintick > limit && hour > start) {
                        paint_text(p, x, y, hour, start, duration, true);
                    }
                }
                if (mapToWidth(hoursubsteps) > d.disttick) {
                    paint_tick(p, x, y, 2, 2);
                }
            }
            else {
                paint_tick(p, x, y, 2, 2);
                if (hour > start) {
                    paint_text(p, x, y, hour, start, duration, true);
                }
            }
            qint64 minutessteps = ticks(t.minutes);
            if (minutessteps) {
                qint64 minutesubsteps = 0;
                if (!(mapToWidth(minutessteps) * d.margintick > limit)) {
                    minutesubsteps = substeps(t.minutes, t.hours, minutessteps, limit);
                }
                for (qint64 minute = hour; minute < (hour + hoursteps) && minute < duration; minute += minutessteps) {
                    int x = mapToX(minute);
                    if (minutesubsteps > 0) {
                        if (minute % minutesubsteps == 0) {
                            if (mapToWidth(minutesubsteps) > d.disttick) {
                                paint_tick(p, x, y, 2, 1);
                            }
                            if (mapToWidth(minutesubsteps) * d.margintick > limit && minute > hour) {
                                paint_text(p, x, y, minute, start, duration);
                            }
                        }
                        if (mapToWidth(minutessteps) > d.disttick) {
                            paint_tick(p, x, y, 2, 1);
                        }
                    }
                    else {
                        paint_tick(p, x, y, 2, 1);
                        if (minute > hour) {
                            paint_text(p, x, y, minute, start, duration);
                        }
                    }
                    qint64 secondssteps = ticks(t.seconds);
                    if (secondssteps) {
                        qint64 secondssubsteps = 0;
                        if (!(mapToWidth(secondssteps) * d.margintick > limit)) {
                            secondssubsteps = substeps(t.seconds, t.minutes, secondssteps, limit);
                        }
                        for (qint64 second = minute; second < (minute + minutessteps) && second < duration; second += secondssteps) {
                            int x = mapToX(second);
                            if (secondssubsteps > 0) {
                                if (second % secondssubsteps == 0) {
                                    if (mapToWidth(secondssubsteps) > d.disttick) {
                                        paint_tick(p, x, y, 2, 1);
                                    }
                                    if (mapToWidth(secondssubsteps) * d.margintick > limit && second > minute) {
                                        paint_text(p, x, y, second, start, duration);
                                    }
                                }
                                if (mapToWidth(secondssteps) > d.disttick) {
                                    paint_tick(p, x, y, 2, 1);
                                }
                            }
                            else {
                                paint_tick(p, x, y, 2, 1, Qt::green);
                                if (second > minute) {
                                    paint_text(p, x, y, second, start, duration);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    p.restore();
}

QPixmap
TimelinePrivate::paint()
{
    qreal dpr = widget->devicePixelRatio();
    QPixmap pixmap(widget->width() * dpr, widget->height() * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);
    
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QFont font("Courier New", 11); // fixed font size
    p.setFont(font);
    
    int y = widget->height() / 2;
    p.setPen(QPen(Qt::lightGray, 1));
    p.drawLine(0, y, widget->width(), y);
    p.drawLine(d.marginrange, y, widget->width() - d.marginrange, y);
    font.setPointSizeF(font.pointSizeF() * 0.8f);
    p.setFont(font);
    
    if (d.range.valid()) {
        p.setPen(QPen(Qt::white, 1));
        // timeline
        {
            qint64 ticks = d.time.ticks();
            qint64 start = d.range.start().ticks();
            qint64 duration = d.range.duration().ticks();

            if (d.timecode == Timeline::Timecode::FRAMES) {
                
                qint64 frames = d.range.duration().frames();
                t = Time { // redistribute to frames
                    steps(frames) * 10,
                    steps(frames),
                    steps(frames) / 10,
                    steps(frames) / 10 / 10,
                };
            }
            else if (d.timecode == Timeline::Timecode::TIME ||
                     d.timecode == Timeline::Timecode::SMPTE) {
                
                qint64 seconds = d.range.duration().seconds();
                qint64 minutes = seconds / 60;
                qint64 hours = minutes / 60;
                if (hours > 0) {
                    t = Time {
                        60 * 60 * 24,
                        60 * 60,
                        60,
                        1,
                    };
                }
                else if (minutes > 0) {
                    t = Time { // redistribute to mins
                        60 * 60,
                        60,
                        1,
                        0,
                    };
                }
                else {
                    t = Time { // redistribute to secs
                        60,
                        1,
                        0,
                        0
                    };
                }
            }
            paint_timeline(p);
            {
                int pos = mapToX(start);
                QPen thickPen(Qt::darkRed, 4);
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
            }
            {
                int pos = mapToX(duration);
                QPen thickPen(Qt::darkRed, 4);
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
            }
            
            if (d.pressed) {
                QString text = labeltick(ticks);
                int pos = mapToX(ticks);
                QFontMetrics metrics(font);
                int pad = 4;
                int textw = metrics.horizontalAdvance(QString().fill('0', text.size())) + 2 * pad;
                int texth = metrics.height() + 2 * pad;
                
                int min = mapToX(start) + textw / 2 - d.marginrange / 2;
                int max = mapToX(duration) - textw / 2 + d.marginrange / 2;
                int bound = qBound(min, pos, max);
                QRect rect(bound - textw / 2, y - texth / 2, textw, texth);
                p.setPen(Qt::black);
                p.setBrush(Qt::white);
                p.drawRect(rect);
                p.drawText(rect, Qt::AlignCenter, text);
            }
            else {
                int pos = 0;
                AVTime next = AVTime(ticks + d.time.tpf(), d.time.timescale(), d.time.fps());
                if (next.ticks() < duration) {
                    pos = mapToX(next.ticks());
                    p.setPen(QPen(Qt::red, 2));
                    p.setBrush(Qt::red);
                    p.drawLine(pos, y - 2, pos, y + 2);
                }
                pos = mapToX(ticks);
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(Qt::white);
                p.drawLine(pos, y - 4, pos, y + 4);
                
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::red);
                p.drawEllipse(QPoint(pos, y), d.radius, d.radius);
            }
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

AVTimeRange
Timeline::range() const
{
    return p->d.range;
}

AVTime
Timeline::time() const
{
    return p->d.time;
}

bool
Timeline::tracking() const
{
    return p->d.tracking;
}

Timeline::Timecode
Timeline::timecode() const
{
    return p->d.timecode;
}

void
Timeline::set_range(const AVTimeRange& range)
{
    if (p->d.range != range) {
        p->d.range = range;
        update();
    }
}

void
Timeline::set_time(const AVTime& time)
{
    if (p->d.time != time && !p->d.pressed) {
        p->d.time = time;
        time_changed(time);
        update();
    }
}

void
Timeline::set_tracking(bool tracking)
{
    if (p->d.tracking != tracking) {
        p->d.tracking = tracking;
        update();
    }
}

void
Timeline::set_timecode(Timeline::Timecode timecode)
{
    if (p->d.timecode != timecode) {
        p->d.timecode = timecode;
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
        qint64 pos = p->mapToTicks(event->pos().x());
        p->d.time.set_ticks(pos);
        p->d.pressed = true;
        slider_pressed();
        update();
    }
}

void
Timeline::mouseMoveEvent(QMouseEvent* event)
{
    if (p->d.pressed) {
        qint64 tick = p->d.time.align(p->mapToTicks(event->pos().x()));
        if (p->d.lasttick != tick) {
            p->d.time.set_ticks(tick);
            slider_moved(p->d.time);
            if (p->d.tracking) {
                time_changed(p->d.time);
            }
            p->d.lasttick = tick;
            update();
        }
    }
}

void
Timeline::mouseReleaseEvent(QMouseEvent* event)
{
    if (p->d.pressed && event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
        p->d.pressed = false;
        slider_released();
        update();
    }
}

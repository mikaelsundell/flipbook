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
        QList<qint64> subticks(qint64 value, qint64 steps, qint64 duration) const;
        void paint_tick(QPainter& p, int x, int y, int height, qreal width = 1, QBrush brush = Qt::white);
        void paint_text(QPainter& p, int x, int y, qint64 value, qint64 start, qint64 duration, bool bold = false, QBrush brush = Qt::white);
        void paint_timeline(QPainter& p);
        QPixmap paint();
        struct Metric
        {
            qint64 major;
            int majorheight;
            qint64 minor;
            int minorheight;
            qint64 ticks;
            int ticksheight;
        };
        Metric m;
        struct Data
        {
            AVTime time;
            AVTimeRange range;
            Timeline::Timecode timecode = Timeline::Timecode::TIME;
            qint64 lasttick = -1;
            bool tracking = false;
            bool pressed = false;
            int margin = 14;
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
    return qRound(d.margin + ratio * (widget->width() - 2 * d.margin));
}

qint64
TimelinePrivate::mapToTicks(int x) const
{
    x = qBound(d.margin, x, widget->width() - d.margin);
    qreal ratio = static_cast<qreal>(x - d.margin) / (widget->width() - 2 * d.margin);
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
}

QSize
TimelinePrivate::labelsize(const QFont& font, qint64 value) const
{
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        return mapToSize(font, QString::number(d.range.duration().frames()));
    }
    else if (d.timecode == Timeline::Timecode::TIME) {
        return mapToSize(font, d.range.duration().to_string()); // max length of label
    }
}

qint64
TimelinePrivate::ticks(qint64 value) const
{
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        return d.range.duration().ticks(value);
    }
    else if (d.timecode == Timeline::Timecode::TIME) {
        return d.range.duration().timescale() * value;
    }
}

qint64
TimelinePrivate::steps(qint64 value) {
    return pow(10, qint64(log10(value)));
}

QList<qint64>
TimelinePrivate::subticks(qint64 value, qint64 steps, qint64 duration) const
{
    QVector<qint64> list;
    if (d.timecode == Timeline::Timecode::FRAMES)  {
        qint64 p1 = value + qRound(steps * 0.2);
        qint64 p2 = value + qRound(steps * 0.4);
        qint64 p3 = value + qRound(steps * 0.6);
        qint64 p4 = value + qRound(steps * 0.8);
        if (p1 < duration) {
            list.append(p1);
        }
        if (p2 < duration) {
            list.append(p2);
        }
        if (p3 < duration) {
            list.append(p3);
        }
        if (p4 < duration) {
            list.append(p4);
        }
    }
    else if (d.timecode == Timeline::Timecode::TIME) {
        qint64 p1 = value + (steps / 4);
        qint64 p2 = value + (steps / 2);
        qint64 p3 = value + (3 * steps / 4);
        if (p1 < duration) {
            list.append(p1);
        }
        if (p2 < duration) {
            list.append(p2);
        }
        if (p3 < duration) {
            list.append(p3);
        }
    }
    return list;
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
    QSize maxsize = labelsize(font, duration);
    qint64 mindist = 4;
    qreal margin = 0.8;
    
    qint64 majorsteps = ticks(m.major);
    int majorwidth = mapToWidth(majorsteps);
    bool majorlabels = maxsize.width() < (majorwidth * margin);
    bool majorvisible = majorsteps && majorwidth > mindist;
    
    if (majorvisible) {
        for(qint64 major = start; major < duration; major += majorsteps) {
            int x = mapToX(major);
            paint_tick(p, x, y, m.majorheight, 2);
            if (majorlabels) {
                paint_text(p, x, y, major, start, duration, true);
            }
            qint64 minorsteps = ticks(m.minor);
            int minorwidth = mapToWidth(minorsteps);
            bool minorlabels = maxsize.width() < (minorwidth * margin);
            bool minorvisible = minorsteps && minorwidth > mindist;
            
            if (minorvisible) {
                for (qint64 minor = major; minor < (major + majorsteps) && minor < duration; minor += minorsteps) {
                    int x = mapToX(minor);
                    paint_tick(p, x, y, m.minorheight, 1);
                    if (minorlabels && minor > major) {
                        paint_text(p, x, y, minor, start, duration);
                    }
                    qint64 ticksteps = ticks(m.ticks);
                    int tickswidth = mapToWidth(ticksteps);
                    int tickslabels = maxsize.width() < (tickswidth * margin);
                    bool ticksvisible = ticksteps && tickswidth > mindist;
                    
                    if (ticksvisible) {
                        for (qint64 tick = minor; tick < (minor + minorsteps) && tick < duration; tick += ticksteps) {
                            int x = mapToX(tick);
                            paint_tick(p, x, y, m.ticksheight, 1);
                            if (tickslabels && tick > minor) {
                                paint_text(p, x, y, tick, start, duration);
                            }
                        }
                    }
                    if (!tickslabels && ticksteps) {
                        QList<qint64> ticks = subticks(minor, minorsteps, duration);
                        if (ticks.size()) {
                            int tickswidth = mapToWidth(ticks.first() - minor);
                            if (tickswidth > mindist) {
                                for (qint64 tick : ticks) {
                                    int x = mapToX(tick);
                                    if (!ticksvisible) {
                                        paint_tick(p, x, y, m.ticksheight, 1);
                                    }
                                    if ((maxsize.width()) < tickswidth * margin) {
                                        paint_text(p, x, y, tick, start, duration, Qt::darkMagenta);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!minorlabels && minorsteps) { // test subticks
                QList<qint64> ticks = subticks(major, majorsteps, duration);
                if (ticks.size()) {
                    int tickswidth = mapToWidth(ticks.first() - major);
                    if (tickswidth > mindist) {
                        for (qint64 tick : ticks) {
                            int x = mapToX(tick);
                            if (!minorvisible) {
                                paint_tick(p, x, y, m.minorheight, 1);
                            }
                            if (maxsize.width() < tickswidth * margin) {
                                paint_text(p, x, y, tick, start, duration);
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
    p.drawLine(d.margin, y, widget->width() - d.margin, y);
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
                m = Metric {
                    steps(frames),
                    4,
                    steps(frames) / 10,
                    4,
                    steps(frames) / 10 / 10,
                    2,
                };
            }
            else if (d.timecode == Timeline::Timecode::TIME) {
                qint64 seconds = d.range.duration().seconds();
                qint64 minutes = seconds / 60;
                qint64 hours = minutes / 60;
                if (hours > 0) {
                    m = Metric {
                        60 * 60,
                        4,
                        60,
                        4,
                        1,
                        2
                    };
                }
                else if (minutes > 0) {
                    m = Metric {
                        60,
                        4,
                        1,
                        2,
                        0,
                        0,
                    };
                }
                else {
                    m = Metric {
                        1,
                        2,
                        0,
                        0,
                        0,
                        0
                    };
                }
            }
            else if (d.timecode == Timeline::Timecode::SMPTE) {
                
            }
            paint_timeline(p);
            {   
                // start
                int pos = mapToX(start);
                QPen thickPen(Qt::white, 4); // 3 is the width of the line
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
            }
            {
                // duration
                int pos = mapToX(duration);
                QPen thickPen(Qt::white, 4); // 3 is the width of the line
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
                QString text = labeltick(duration);
            }
            
            if (d.pressed) {
                QString text = labeltick(ticks);
                int pos = mapToX(ticks);
                QFontMetrics metrics(font);
                int pad = 4;
                int textw = metrics.horizontalAdvance(QString().fill('0', text.size())) + 2 * pad;
                int texth = metrics.height() + 2 * pad;
                
                int min = mapToX(start) + textw / 2 - d.margin / 2;
                int max = mapToX(duration) - textw / 2 + d.margin / 2;
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

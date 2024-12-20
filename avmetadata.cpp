// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avmetadata.h"

#include <QList>

class AVMetadataPrivate
{
    public:
        QAtomicInt ref;
        QList<QPair<QString, QString>> data;
};

AVMetadata::AVMetadata()
: p(new AVMetadataPrivate())
{
}

AVMetadata::AVMetadata(const AVMetadata& other)
: p(other.p)
{
}

AVMetadata::~AVMetadata()
{
}

void
AVMetadata::clear()
{
    p->data.clear();
}

bool
AVMetadata::contains_key(const QString& key) const
{
    for (const QPair<QString, QString>& pair : p->data) {
        if (pair.first == key) {
            return true;
        }
    }
    return false;
}

bool
AVMetadata::remove_key(const QString& key)
{
    for (int i = 0; i < p->data.size(); ++i) {
        if (p->data[i].first == key) {
            p->data.removeAt(i);
            return true;
        }
    }
    return false;
}

QList<QPair<QString, QString>>
AVMetadata::data() const
{
    return p->data;
}

void
AVMetadata::add_pair(const QString& key, const QString& value)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    for (QPair<QString, QString>& pair : p->data) {
        if (pair.first == key) {
            pair.second = value;
            return;
        }
    }
    p->data.append(qMakePair(key, value));
}

AVMetadata&
AVMetadata::operator=(const AVMetadata& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QExplicitlySharedDataPointer>

class AVMetadataPrivate;
class AVMetadata
{
    public:
        AVMetadata();
        virtual ~AVMetadata();
        AVMetadata(const AVMetadata& other);
        void clear();
        bool contains_key(const QString& key) const;
        bool remove_key(const QString& key);
        QList<QPair<QString, QString>> data() const;
        void add_pair(const QString& key, const QString& value);
    
        AVMetadata& operator=(const AVMetadata& other);

    private:
        QExplicitlySharedDataPointer<AVMetadataPrivate> p;
};

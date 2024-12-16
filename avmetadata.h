// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QImage>
#include <QScopedPointer>

class AVMetadataPrivate;
class AVMetadata
{
    public:
        AVMetadata();
        virtual ~AVMetadata();
        
    private:
        QScopedPointer<AVMetadataPrivate> p;
};

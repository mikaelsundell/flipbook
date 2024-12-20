// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QExplicitlySharedDataPointer>

class AVSidecarPrivate;
class AVSidecar
{
    public:
        AVSidecar();
        virtual ~AVSidecar();
        AVSidecar(const AVSidecar& other);
        void clear();
    
        AVSidecar& operator=(const AVSidecar& other);
    
    private:
        QExplicitlySharedDataPointer<AVSidecarPrivate> p;
};

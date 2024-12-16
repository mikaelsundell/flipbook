// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QScopedPointer>

class AVSidecarPrivate;
class AVSidecar
{
    public:
        AVSidecar();
        virtual ~AVSidecar();
        
    private:
        QScopedPointer<AVSidecarPrivate> p;
};

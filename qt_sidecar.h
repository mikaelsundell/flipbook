// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QScopedPointer>

class qt_sidecar_private;
class qt_sidecar
{
    public:
        qt_sidecar();
        virtual ~qt_sidecar();

    private:
        QScopedPointer<qt_sidecar_private> p;
};

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "qt_sidecar.h"

class qt_sidecar_private
{
    public:
        qt_sidecar_private();
        void init();
};

qt_sidecar_private::qt_sidecar_private()
{
}

void
qt_sidecar_private::init()
{
}

qt_sidecar::qt_sidecar()
: p(new qt_sidecar_private())
{
    p->init();
}

qt_sidecar::~qt_sidecar()
{
}

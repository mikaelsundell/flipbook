// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avsidecar.h"

class AVSidecarPrivate
{
    public:
        AVSidecarPrivate();
        void init();
};

AVSidecarPrivate::AVSidecarPrivate()
{
}

void
AVSidecarPrivate::init()
{
}

AVSidecar::AVSidecar()
: p(new AVSidecarPrivate())
{
    p->init();
}

AVSidecar::~AVSidecar()
{
}

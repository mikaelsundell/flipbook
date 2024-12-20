// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avsidecar.h"

class AVSidecarPrivate
{
    public:
        QAtomicInt ref;
};

AVSidecar::AVSidecar()
: p(new AVSidecarPrivate())
{
}

AVSidecar::AVSidecar(const AVSidecar& other)
: p(other.p)
{
}

AVSidecar::~AVSidecar()
{
}

void
AVSidecar::clear()
{
}

AVSidecar&
AVSidecar::operator=(const AVSidecar& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

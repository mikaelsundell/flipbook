// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "avmetadata.h"

class AVMetadataPrivate
{
    public:
        AVMetadataPrivate();
        void init();
};

AVMetadataPrivate::AVMetadataPrivate()
{
}

void
AVMetadataPrivate::init()
{
}

AVMetadata::AVMetadata()
: p(new AVMetadataPrivate())
{
    p->init();
}

AVMetadata::~AVMetadata()
{
}

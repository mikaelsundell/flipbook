// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

namespace mac
{
    void setDarkAppearance()
    {
        // we force dark aque no matter appearance set in system settings
        [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
    }
}

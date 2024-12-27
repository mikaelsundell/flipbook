// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avmetadata.h"
#include "avsidecar.h"
#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"

#include <QImage>
#include <QObject>
#include <QScopedPointer>

class PlatformPrivate;
class Platform : public QObject {
    Q_OBJECT
    public:
        enum Power { POWEROFF, RESTART, SLEEP  };
        Q_ENUM(Power)
    
    public:
        Platform();
        virtual ~Platform();
        void stayawake(bool awake);

    Q_SIGNALS:
        void power_changed(Power power);
        
    private:
        QScopedPointer<PlatformPrivate> p;
};

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "platform.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/IOMessage.h>

#include <mach/mach.h>
#include <mach/mach_time.h>

#include <QPointer>

#include <QDebug>

class PlatformPrivate
{
    public:
        PlatformPrivate();
        ~PlatformPrivate();
        void init();
        static void power_callback(void* refCon, io_service_t service, natural_t messageType, void* messageArgument);

    public:
        id activity = nullptr;
        IONotificationPortRef notificationport = nullptr;
        io_object_t notifier = 0;
        QPointer<Platform> object;
};

PlatformPrivate::PlatformPrivate()
{
}

PlatformPrivate::~PlatformPrivate()
{
    if (notificationport) {
        IONotificationPortDestroy(notificationport);
    }
    if (notifier) {
        IODeregisterForSystemPower(&notifier);
    }
}

void
PlatformPrivate::init()
{
    io_connect_t rootPort = IORegisterForSystemPower(this, &notificationport, power_callback, &notifier);
    if (rootPort == MACH_PORT_NULL) {
        qWarning() << "failed to register for system power notifications.";
        return;
    }
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(notificationport);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]]; // force dark aque appearance
}

void
PlatformPrivate::power_callback(void* refCon, io_service_t service, natural_t messageType, void* messageArgument)
{
    PlatformPrivate* platformPrivate = static_cast<PlatformPrivate*>(refCon);
    if (!platformPrivate || !platformPrivate->object) {
        return;
    }
    switch (messageType) {
        case kIOMessageSystemWillPowerOff:
            platformPrivate->object->power_changed(Platform::POWEROFF);
            break;
        case kIOMessageSystemWillRestart:
            platformPrivate->object->power_changed(Platform::RESTART);
            break;
        case kIOMessageSystemWillSleep:
            platformPrivate->object->power_changed(Platform::SLEEP);
            IOAllowPowerChange(service, (long)messageArgument);
            break;
        default:
            break;
    }
}

#include "platform.moc"

Platform::Platform()
: p(new PlatformPrivate())
{
    p->object = this;
    p->init();

}

Platform::~Platform()
{
}

void
Platform::stayawake(bool activity)
{
    static IOPMAssertionID assertionid = kIOPMNullAssertionID;
    if (activity) {
        if (assertionid == kIOPMNullAssertionID) {
            IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                        kIOPMAssertionLevelOn,
                                        CFSTR("Preventing idle sleep"),
                                        &assertionid);
        }
    } else {
        if (assertionid != kIOPMNullAssertionID) {
            IOPMAssertionRelease(assertionid);
            assertionid = kIOPMNullAssertionID;
        }
    }
}

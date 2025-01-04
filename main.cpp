// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"
#include "test.h"

#include <QApplication>

// main
int
main(int argc, char* argv[])
{
    if (1) { // type tests
        test_time();
        test_timerange();
        test_fps();
        test_smpte();
    }
    if (1) { // timer tests
        test_timer();
    }
    QApplication app(argc, argv);
    Flipbook* flipbook = new Flipbook();
    flipbook->show();
    return app.exec();
}

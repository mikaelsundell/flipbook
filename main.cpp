// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipman.h"
#include "test.h"

#include <QApplication>

// main
int
main(int argc, char* argv[])
{
    if (0) {
        test_time();
        test_timerange();
        test_fps();
        test_smpte();
    }
    if (0) {
        test_timer();
    }
    QApplication app(argc, argv);
    Flipman* flipman = new Flipman();
    flipman->set_arguments(QCoreApplication::arguments());
    flipman->show();
    return app.exec();
}

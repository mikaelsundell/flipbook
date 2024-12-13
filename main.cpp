// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "flipbook.h"

#include <QApplication>

#include <iostream>

// main
int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    flipbook* window = new flipbook();
    window->show();
    return app.exec();
}

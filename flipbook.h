// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

class FlipbookPrivate;
class Flipbook : public QMainWindow
{
    Q_OBJECT
    public:
        Flipbook(QWidget* parent = nullptr);
        virtual ~Flipbook();

    private:
        QScopedPointer<FlipbookPrivate> p;
};

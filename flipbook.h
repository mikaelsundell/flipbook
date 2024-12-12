//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
//

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

class flipbook_private;
class flipbook : public QMainWindow
{
    Q_OBJECT
    public:
        flipbook(QWidget* parent = nullptr);
        virtual ~flipbook();

    private:
        QScopedPointer<flipbook_private> p;
};

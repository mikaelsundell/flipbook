// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

class FlipmanPrivate;
class Flipman : public QMainWindow
{
    Q_OBJECT
    public:
        Flipman(QWidget* parent = nullptr);
        virtual ~Flipman();
        void set_arguments(const QStringList& arguments);

    private:
        QScopedPointer<FlipmanPrivate> p;
};

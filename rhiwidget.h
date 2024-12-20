// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QScopedPointer>

class RhiWidgetPrivate;
class RhiWidget : public QRhiWidget
{
    Q_OBJECT
    public:
        RhiWidget(QWidget* parent = nullptr);
        virtual ~RhiWidget();
        void set_image(const QImage& image);
    
    protected:
        void initialize(QRhiCommandBuffer* cb) override;
        void render(QRhiCommandBuffer* cb) override;

    private:
        QScopedPointer<RhiWidgetPrivate> p;
};

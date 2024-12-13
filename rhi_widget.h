// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QScopedPointer>

class rhi_widget_private;
class rhi_widget : public QRhiWidget
{
    Q_OBJECT
    public:
        rhi_widget(QWidget* parent = nullptr);
        virtual ~rhi_widget();
        void set_image(const QImage& image);
    
    protected:
        void initialize(QRhiCommandBuffer* cb) override;
        void render(QRhiCommandBuffer* cb) override;

    private:
        QScopedPointer<rhi_widget_private> p;
};

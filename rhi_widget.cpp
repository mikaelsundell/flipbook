// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "rhi_widget.h"

#include <QApplication>
#include <QFile>
#include <QPointer>

class rhi_widget_private : public QObject
{
    Q_OBJECT
    public:
        rhi_widget_private();
        void init();
        void render();
        void reset(QRhiRenderTarget* target);
        QShader shader(const QString& name);
        void setImage(const QImage& image);

    public:
        QImage checkerboard(int width, int height, int size);
    
        QRhi* rhi = nullptr;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::unique_ptr<QRhiBuffer> vertexbuffer;
        std::unique_ptr<QRhiBuffer> mvpbuffer;
        std::unique_ptr<QRhiTexture> texturebuffer;
        std::unique_ptr<QRhiSampler> texturesampler;
        std::unique_ptr<QRhiShaderResourceBindings> shaderresourcebindings;
        QImage texturedata;
        QVector<float> vertexdata;
        QMatrix4x4 mvpdata;
        QPointer<rhi_widget> widget;
};

rhi_widget_private::rhi_widget_private()
{
}

void
rhi_widget_private::init()
{
    vertexdata = {
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 1.0f
    };
    texturedata = checkerboard(256, 256, 32);
}

void
rhi_widget_private::render()
{
}

void
rhi_widget_private::reset(QRhiRenderTarget* target)
{
    
}

QShader
rhi_widget_private::shader(const QString& name)
{
    QString path = QCoreApplication::applicationDirPath() + "/../Resources/" + name;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        return QShader::fromSerialized(file.readAll());
    } else {
        qWarning() << "error: Unable to open shader file:" << path;
        return QShader();
    }
}

void
rhi_widget_private::setImage(const QImage& image)
{
    texturedata = image;
    if (texturedata.format() != QImage::Format_RGBA8888) {
        texturedata = texturedata.convertToFormat(QImage::Format_RGBA8888);
    }
    rhi = nullptr;
    float aspect = static_cast<float>(image.width()) / static_cast<float>(image.height());
    vertexdata = {
        -0.5f * aspect,  0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f * aspect, -0.5f, 0.0f, 0.0f, 1.0f,
         0.5f * aspect,  0.5f, 0.0f, 1.0f, 0.0f,
         0.5f * aspect, -0.5f, 0.0f, 1.0f, 1.0f
    };
    widget->update();
}

QImage
rhi_widget_private::checkerboard(int width, int height, int size)
{
    QImage image(width, height, QImage::Format_RGBA8888);
    QColor black(0, 0, 0, 255);
    QColor white(255, 255, 255, 255);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sx = x / size;
            int sy = y / size;
            QColor color = (sx + sy) % 2 == 0 ? black : white;
            image.setPixel(x, y, color.rgba());
        }
    }
    return image;
}

#include "rhi_widget.moc"

rhi_widget::rhi_widget(QWidget* parent)
: QRhiWidget(parent)
, p(new rhi_widget_private())
{
    p->widget = this;
    p->init();
}

rhi_widget::~rhi_widget()
{
}

void
rhi_widget::setImage(const QImage& image)
{
    p->setImage(image);
}

void
rhi_widget::initialize(QRhiCommandBuffer* cb)
{
    if (p->rhi != rhi()) {
        p->pipeline.reset();
        p->rhi = rhi();
    }
    p->vertexbuffer.reset(p->rhi->newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::VertexBuffer,
        static_cast<quint32>(p->vertexdata.size() * sizeof(float)))
    );
    if (!p->vertexbuffer->create()) {
        qWarning() << "error: could not create vertex buffer";
    }

    p->mvpbuffer.reset(p->rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        sizeof(float) * 4 * 4) // size of 4x4 matrix
    );
    if (!p->mvpbuffer->create()) {
        qWarning() << "error: could not create view buffer";
    }
    
    p->texturebuffer.reset(p->rhi->newTexture(
        QRhiTexture::RGBA8,
        p->texturedata.size(),
        1) // no multi-sampling
    );
    if (!p->texturebuffer->create()) {
        qWarning() << "error: could not create texture buffer";
    }
    
    p->texturesampler.reset(p->rhi->newSampler(
        QRhiSampler::Linear,
        QRhiSampler::Linear,
        QRhiSampler::Linear,
        QRhiSampler::ClampToEdge,
        QRhiSampler::ClampToEdge)
    );
    if (!p->texturesampler->create()) {
        qWarning() << "error: could not create texture sampler";
    }

    p->shaderresourcebindings.reset(p->rhi->newShaderResourceBindings());
    p->shaderresourcebindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, // uniforms
            QRhiShaderResourceBinding::VertexStage,
            p->mvpbuffer.get()
        ),
        QRhiShaderResourceBinding::sampledTexture(
            1, // textures and samplers
            QRhiShaderResourceBinding::FragmentStage,
            p->texturebuffer.get(),
            p->texturesampler.get()
        )
    });
    if (!p->shaderresourcebindings->create()) {
        qWarning() << "error: failed to create shader resource bindings.";
        return;
    }
    
    p->pipeline.reset(p->rhi->newGraphicsPipeline());
    p->pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    p->pipeline->setShaderStages({
        {
            QRhiShaderStage::Vertex, p->shader(QLatin1String("color.vert.qsb"))
        },
        {
            QRhiShaderStage::Fragment, p->shader(QLatin1String("color.frag.qsb"))
        }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 }, // Float3 will match vec4 in shader
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
    });
    
    p->pipeline->setVertexInputLayout(inputLayout);
    p->pipeline->setShaderResourceBindings(p->shaderresourcebindings.get());
    p->pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    p->pipeline->create();

    QRhiResourceUpdateBatch* resourceUpdates = p->rhi->nextResourceUpdateBatch();
    resourceUpdates->uploadStaticBuffer(p->vertexbuffer.get(), p->vertexdata.data());
    resourceUpdates->uploadTexture(p->texturebuffer.get(), p->texturedata);
    cb->resourceUpdate(resourceUpdates);

    const QSize size = renderTarget()->pixelSize();
    p->mvpdata = p->rhi->clipSpaceCorrMatrix();
    p->mvpdata.perspective(45.0f, size.width() / static_cast<float>(size.height()), 0.01f, 1000.0f);
    p->mvpdata.translate(0, 0, -2);
}


void
rhi_widget::render(QRhiCommandBuffer* cb)
{
    if (p->rhi != rhi()) {
        initialize(cb);
    }
    
    QRhiResourceUpdateBatch* resourceUpdates = p->rhi->nextResourceUpdateBatch();
    QMatrix4x4 mvp = p->mvpdata;
    resourceUpdates->updateDynamicBuffer(
        p->mvpbuffer.get(),
        0,
        sizeof(float) * 4 * 4, // size of 4x4 matrix
        p->mvpdata.constData()
    );
    
    cb->beginPass(
        renderTarget(),
        QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f), { 1.0f, 0 },
        resourceUpdates
    );
    {
        cb->setGraphicsPipeline(p->pipeline.get());
        const QSize size = renderTarget()->pixelSize();
        cb->setViewport(QRhiViewport(0, 0, size.width(), size.height()));
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexbinding(p->vertexbuffer.get(), 0);
        cb->setVertexInput(0, 1, &vertexbinding);
        cb->draw(4);
    }
    cb->endPass();
}

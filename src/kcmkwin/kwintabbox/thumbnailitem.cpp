/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2011, 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "thumbnailitem.h"
// Qt
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QStandardPaths>

namespace KWin
{

BrightnessSaturationShader::BrightnessSaturationShader()
    : QSGMaterialShader()
    , m_id_matrix(0)
    , m_id_opacity(0)
    , m_id_saturation(0)
    , m_id_brightness(0)
{
}

const char *BrightnessSaturationShader::vertexShader() const
{
    return "attribute highp vec4 vertex;          \n"
           "attribute highp vec2 texCoord;        \n"
           "uniform highp mat4 u_matrix;          \n"
           "varying highp vec2 v_coord;           \n"
           "void main() {                         \n"
           "    v_coord = texCoord;               \n"
           "    gl_Position = u_matrix * vertex;  \n"
           "}";
}

const char *BrightnessSaturationShader::fragmentShader() const
{
    return "uniform sampler2D qt_Texture;                          \n"
           "uniform lowp float u_opacity;                          \n"
           "uniform highp float u_saturation;                      \n"
           "uniform highp float u_brightness;                      \n"
           "varying highp vec2 v_coord;                            \n"
           "void main() {                                          \n"
           "    lowp vec4 tex = texture2D(qt_Texture, v_coord); \n"
           "    if (u_saturation != 1.0) {                      \n"
           "        tex.rgb = mix(vec3(dot( vec3( 0.30, 0.59, 0.11 ), tex.rgb )), tex.rgb, u_saturation); \n"
           "    }                                               \n"
           "    tex.rgb = tex.rgb * u_brightness;               \n"
           "    gl_FragColor = tex * u_opacity; \n"
           "}";
}

const char *const *BrightnessSaturationShader::attributeNames() const
{
    static char const *const names[] = {"vertex", "texCoord", nullptr};
    return names;
}

void BrightnessSaturationShader::updateState(const QSGMaterialShader::RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(program()->isLinked());
    if (state.isMatrixDirty()) {
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
    }
    if (state.isOpacityDirty()) {
        program()->setUniformValue(m_id_opacity, state.opacity());
    }

    auto *tx = static_cast<BrightnessSaturationMaterial *>(newMaterial);
    auto *oldTx = static_cast<BrightnessSaturationMaterial *>(oldMaterial);
    QSGTexture *t = tx->texture();
    t->setFiltering(QSGTexture::Linear);

    if (!oldTx || oldTx->texture()->textureId() != t->textureId()) {
        t->bind();
    } else {
        t->updateBindOptions();
    }

    program()->setUniformValue(m_id_saturation, static_cast<float>(tx->saturation));
    program()->setUniformValue(m_id_brightness, static_cast<float>(tx->brightness));
}

void BrightnessSaturationShader::initialize()
{
    QSGMaterialShader::initialize();
    m_id_matrix = program()->uniformLocation("u_matrix");
    m_id_opacity = program()->uniformLocation("u_opacity");
    m_id_saturation = program()->uniformLocation("u_saturation");
    m_id_brightness = program()->uniformLocation("u_brightness");
}

WindowThumbnailItem::WindowThumbnailItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_wId(0)
    , m_image()
    , m_clipToItem(nullptr)
    , m_brightness(1.0)
    , m_saturation(1.0)
    , m_sourceSize(QSize())
{
    setFlag(ItemHasContents);
}

WindowThumbnailItem::~WindowThumbnailItem()
{
}

void WindowThumbnailItem::setWId(qulonglong wId)
{
    m_wId = wId;
    Q_EMIT wIdChanged(wId);
    findImage();
}

void WindowThumbnailItem::setClipTo(QQuickItem *clip)
{
    if (m_clipToItem == clip) {
        return;
    }
    m_clipToItem = clip;
    Q_EMIT clipToChanged();
}

void WindowThumbnailItem::findImage()
{
    QString imagePath;
    switch (m_wId) {
    case Konqueror:
        imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin/kcm_kwintabbox/konqueror.png");
        break;
    case Systemsettings:
        imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin/kcm_kwintabbox/systemsettings.png");
        break;
    case KMail:
        imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin/kcm_kwintabbox/kmail.png");
        break;
    case Dolphin:
        imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin/kcm_kwintabbox/dolphin.png");
        break;
    case Desktop:
        imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "wallpapers/Next/contents/screenshot.png");
        if (imagePath.isNull()) {
            imagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin/kcm_kwintabbox/desktop.png");
        }
        break;
    default:
        // ignore
        break;
    }
    if (imagePath.isNull()) {
        m_image = QImage();
    } else {
        m_image = QImage(imagePath);
    }

    setImplicitSize(m_image.width(), m_image.height());
}

QSGNode *WindowThumbnailItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(oldNode);
    if (!node) {
        node = new QSGGeometryNode();
        auto *material = new BrightnessSaturationMaterial;
        material->setFlag(QSGMaterial::Blending);
        material->setTexture(window()->createTextureFromImage(m_image));
        node->setMaterial(material);
        QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        node->setGeometry(geometry);
    }
    auto *material = static_cast<BrightnessSaturationMaterial *>(node->material());
    const QSize size(material->texture()->textureSize().scaled(boundingRect().size().toSize(), Qt::KeepAspectRatio));
    const qreal x = boundingRect().x() + (boundingRect().width() - size.width()) / 2;
    const qreal y = boundingRect().y() + (boundingRect().height() - size.height()) / 2;
    QSGGeometry::updateTexturedRectGeometry(node->geometry(), QRectF(QPointF(x, y), size), QRectF(0.0, 0.0, 1.0, 1.0));
    material->brightness = m_brightness;
    material->saturation = m_saturation;
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

qreal WindowThumbnailItem::brightness() const
{
    return m_brightness;
}

qreal WindowThumbnailItem::saturation() const
{
    return m_saturation;
}

QSize WindowThumbnailItem::sourceSize() const
{
    return m_sourceSize;
}

void WindowThumbnailItem::setBrightness(qreal brightness)
{
    if (m_brightness == brightness) {
        return;
    }
    m_brightness = brightness;
    update();
    Q_EMIT brightnessChanged();
}

void WindowThumbnailItem::setSaturation(qreal saturation)
{
    if (m_saturation == saturation) {
        return;
    }
    m_saturation = saturation;
    update();
    Q_EMIT saturationChanged();
}

void WindowThumbnailItem::setSourceSize(const QSize &size)
{
    if (m_sourceSize == size) {
        return;
    }
    m_sourceSize = size;
    update();
    Q_EMIT sourceSizeChanged();
}

} // namespace KWin

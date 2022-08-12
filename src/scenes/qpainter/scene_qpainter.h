/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_SCENE_QPAINTER_H
#define KWIN_SCENE_QPAINTER_H

#include "qpainterbackend.h"

#include "decorationitem.h"
#include "scene.h"
#include "shadow.h"

namespace KWin
{

class KWIN_EXPORT SceneQPainter : public Scene
{
    Q_OBJECT

public:
    ~SceneQPainter() override;
    void paint(RenderTarget *renderTarget, const QRegion &region) override;
    void paintGenericScreen(int mask, const ScreenPaintData &data) override;
    bool initFailed() const override;
    Shadow *createShadow(Window *window) override;
    DecorationRenderer *createDecorationRenderer(Decoration::DecoratedClientImpl *impl) override;
    SurfaceTexture *createSurfaceTextureInternal(SurfacePixmapInternal *pixmap) override;
    SurfaceTexture *createSurfaceTextureWayland(SurfacePixmapWayland *pixmap) override;
    void render(Item *item, int mask, const QRegion &region, const WindowPaintData &data) override;

    bool animationsSupported() const override
    {
        return false;
    }

    QPainter *scenePainter() const override;

    QPainterBackend *backend() const
    {
        return m_backend;
    }

    static SceneQPainter *createScene(QPainterBackend *backend, QObject *parent);

protected:
    void paintBackground(const QRegion &region) override;
    void paintOffscreenQuickView(OffscreenQuickView *w) override;

private:
    explicit SceneQPainter(QPainterBackend *backend, QObject *parent = nullptr);

    void renderSurfaceItem(QPainter *painter, SurfaceItem *surfaceItem) const;
    void renderDecorationItem(QPainter *painter, DecorationItem *decorationItem) const;
    void renderItem(QPainter *painter, Item *item) const;

    QPainterBackend *m_backend;
    QScopedPointer<QPainter> m_painter;
};

class SceneQPainterShadow : public Shadow
{
public:
    SceneQPainterShadow(Window *window);
    ~SceneQPainterShadow() override;

protected:
    bool prepareBackend() override;
};

class SceneQPainterDecorationRenderer : public DecorationRenderer
{
    Q_OBJECT
public:
    enum class DecorationPart : int {
        Left,
        Top,
        Right,
        Bottom,
        Count
    };
    explicit SceneQPainterDecorationRenderer(Decoration::DecoratedClientImpl *client);

    void render(const QRegion &region) override;

    QImage image(DecorationPart part) const;

private:
    void resizeImages();
    QImage m_images[int(DecorationPart::Count)];
};

inline QPainter *SceneQPainter::scenePainter() const
{
    return m_painter.data();
}

} // KWin

#endif // KWIN_SCENEQPAINTER_H

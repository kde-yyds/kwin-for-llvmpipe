/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_SCREEN_EDGE_EFFECT_H
#define KWIN_SCREEN_EDGE_EFFECT_H
#include <kwineffects.h>

class QTimer;
namespace Plasma
{
class Svg;
}

namespace KWin
{
class Glow;
class GLTexture;

class ScreenEdgeEffect : public Effect
{
    Q_OBJECT
public:
    ScreenEdgeEffect();
    ~ScreenEdgeEffect() override;
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(int mask, const QRegion &region, ScreenPaintData &data) override;
    bool isActive() const override;

    int requestedEffectChainPosition() const override
    {
        return 10;
    }

private Q_SLOTS:
    void edgeApproaching(ElectricBorder border, qreal factor, const QRect &geometry);
    void cleanup();

private:
    void ensureGlowSvg();
    Glow *createGlow(ElectricBorder border, qreal factor, const QRect &geometry);
    template<typename T>
    T *createCornerGlow(ElectricBorder border);
    template<typename T>
    T *createEdgeGlow(ElectricBorder border, const QSize &size);
    QSize cornerGlowSize(ElectricBorder border);
    Plasma::Svg *m_glow = nullptr;
    QHash<ElectricBorder, Glow *> m_borders;
    QTimer *m_cleanupTimer;
};

class Glow
{
public:
    QScopedPointer<GLTexture> texture;
    QScopedPointer<QImage> image;
    QSize pictureSize;
    qreal strength;
    QRect geometry;
    ElectricBorder border;
};

}

#endif

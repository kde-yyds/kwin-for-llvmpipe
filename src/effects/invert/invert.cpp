/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Rivo Laks <rivolaks@hot.ee>
    SPDX-FileCopyrightText: 2008 Lucas Murray <lmurray@undefinedfire.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "invert.h"

#include <KGlobalAccel>
#include <KLocalizedString>
#include <QAction>
#include <QFile>
#include <QStandardPaths>
#include <kwinglplatform.h>
#include <kwinglutils.h>

#include <QMatrix4x4>

Q_LOGGING_CATEGORY(KWIN_INVERT, "kwin_effect_invert", QtWarningMsg)

static void ensureResources()
{
    // Must initialize resources manually because the effect is a static lib.
    Q_INIT_RESOURCE(invert);
}

namespace KWin
{

InvertEffect::InvertEffect()
    : m_inited(false)
    , m_valid(true)
    , m_shader(nullptr)
    , m_allWindows(false)
{
    QAction *a = new QAction(this);
    a->setObjectName(QStringLiteral("Invert"));
    a->setText(i18n("Toggle Invert Effect"));
    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << (Qt::CTRL | Qt::META | Qt::Key_I));
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << (Qt::CTRL | Qt::META | Qt::Key_I));
    effects->registerGlobalShortcut(Qt::CTRL | Qt::META | Qt::Key_I, a);
    connect(a, &QAction::triggered, this, &InvertEffect::toggleScreenInversion);

    QAction *b = new QAction(this);
    b->setObjectName(QStringLiteral("InvertWindow"));
    b->setText(i18n("Toggle Invert Effect on Window"));
    KGlobalAccel::self()->setDefaultShortcut(b, QList<QKeySequence>() << (Qt::CTRL | Qt::META | Qt::Key_U));
    KGlobalAccel::self()->setShortcut(b, QList<QKeySequence>() << (Qt::CTRL | Qt::META | Qt::Key_U));
    effects->registerGlobalShortcut(Qt::CTRL | Qt::META | Qt::Key_U, b);
    connect(b, &QAction::triggered, this, &InvertEffect::toggleWindow);

    connect(effects, &EffectsHandler::windowClosed, this, &InvertEffect::slotWindowClosed);
}

InvertEffect::~InvertEffect()
{
    delete m_shader;
}

bool InvertEffect::supported()
{
    return effects->compositingType() == OpenGLCompositing;
}

bool InvertEffect::loadData()
{
    ensureResources();
    m_inited = true;

    m_shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture, QString(), QStringLiteral(":/effects/invert/shaders/invert.frag"));
    if (!m_shader->isValid()) {
        qCCritical(KWIN_INVERT) << "The shader failed to load!";
        return false;
    }

    return true;
}

void InvertEffect::drawWindow(EffectWindow *w, int mask, const QRegion &region, WindowPaintData &data)
{
    // Load if we haven't already
    if (m_valid && !m_inited) {
        m_valid = loadData();
    }

    bool useShader = m_valid && (m_allWindows != m_windows.contains(w));
    if (useShader) {
        ShaderManager *shaderManager = ShaderManager::instance();
        shaderManager->pushShader(m_shader);

        data.shader = m_shader;
    }

    effects->drawWindow(w, mask, region, data);

    if (useShader) {
        ShaderManager::instance()->popShader();
    }
}

void InvertEffect::slotWindowClosed(EffectWindow *w)
{
    m_windows.removeOne(w);
}

void InvertEffect::toggleScreenInversion()
{
    m_allWindows = !m_allWindows;
    effects->addRepaintFull();
}

void InvertEffect::toggleWindow()
{
    if (!effects->activeWindow()) {
        return;
    }
    if (!m_windows.contains(effects->activeWindow())) {
        m_windows.append(effects->activeWindow());
    } else {
        m_windows.removeOne(effects->activeWindow());
    }
    effects->activeWindow()->addRepaintFull();
}

bool InvertEffect::isActive() const
{
    return m_valid && (m_allWindows || !m_windows.isEmpty());
}

bool InvertEffect::provides(Feature f)
{
    return f == ScreenInversion;
}

} // namespace

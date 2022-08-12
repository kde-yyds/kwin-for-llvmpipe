/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kwin_wayland_test.h"

#include "composite.h"
#include "deleted.h"
#include "effectloader.h"
#include "effects.h"
#include "platform.h"
#include "renderbackend.h"
#include "wayland_server.h"
#include "window.h"
#include "workspace.h"

#include <KWayland/Client/surface.h>

using namespace KWin;

static const QString s_socketName = QStringLiteral("wayland_test_effects_toplevel_open_close_animation-0");

class ToplevelOpenCloseAnimationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testAnimateToplevels_data();
    void testAnimateToplevels();
    void testDontAnimatePopups_data();
    void testDontAnimatePopups();
};

void ToplevelOpenCloseAnimationTest::initTestCase()
{
    qputenv("XDG_DATA_DIRS", QCoreApplication::applicationDirPath().toUtf8());

    qRegisterMetaType<KWin::Window *>();
    qRegisterMetaType<KWin::Deleted *>();
    QSignalSpy applicationStartedSpy(kwinApp(), &Application::started);
    QVERIFY(applicationStartedSpy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    QVERIFY(waylandServer()->init(s_socketName));

    auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
    KConfigGroup plugins(config, QStringLiteral("Plugins"));
    const auto builtinNames = EffectLoader().listOfKnownEffects();
    for (const QString &name : builtinNames) {
        plugins.writeEntry(name + QStringLiteral("Enabled"), false);
    }
    config->sync();
    kwinApp()->setConfig(config);

    qputenv("KWIN_COMPOSE", QByteArrayLiteral("O2"));
    qputenv("KWIN_EFFECTS_FORCE_ANIMATIONS", QByteArrayLiteral("1"));

    kwinApp()->start();
    QVERIFY(applicationStartedSpy.wait());
    Test::initWaylandWorkspace();

    QCOMPARE(Compositor::self()->backend()->compositingType(), KWin::OpenGLCompositing);
}

void ToplevelOpenCloseAnimationTest::init()
{
    QVERIFY(Test::setupWaylandConnection());
}

void ToplevelOpenCloseAnimationTest::cleanup()
{
    auto effectsImpl = qobject_cast<EffectsHandlerImpl *>(effects);
    QVERIFY(effectsImpl);
    effectsImpl->unloadAllEffects();
    QVERIFY(effectsImpl->loadedEffects().isEmpty());

    Test::destroyWaylandConnection();
}

void ToplevelOpenCloseAnimationTest::testAnimateToplevels_data()
{
    QTest::addColumn<QString>("effectName");

    QTest::newRow("Fade") << QStringLiteral("kwin4_effect_fade");
    QTest::newRow("Glide") << QStringLiteral("glide");
    QTest::newRow("Scale") << QStringLiteral("kwin4_effect_scale");
}

void ToplevelOpenCloseAnimationTest::testAnimateToplevels()
{
    // This test verifies that window open/close animation effects try to
    // animate the appearing and the disappearing of toplevel windows.

    // Make sure that we have the right effects ptr.
    auto effectsImpl = qobject_cast<EffectsHandlerImpl *>(effects);
    QVERIFY(effectsImpl);

    // Load effect that will be tested.
    QFETCH(QString, effectName);
    QVERIFY(effectsImpl->loadEffect(effectName));
    QCOMPARE(effectsImpl->loadedEffects().count(), 1);
    QCOMPARE(effectsImpl->loadedEffects().first(), effectName);
    Effect *effect = effectsImpl->findEffect(effectName);
    QVERIFY(effect);
    QVERIFY(!effect->isActive());

    // Create the test window.
    using namespace KWayland::Client;
    QScopedPointer<KWayland::Client::Surface> surface(Test::createSurface());
    QVERIFY(!surface.isNull());
    QScopedPointer<Test::XdgToplevel> shellSurface(Test::createXdgToplevelSurface(surface.data()));
    QVERIFY(!shellSurface.isNull());
    Window *window = Test::renderAndWaitForShown(surface.data(), QSize(100, 50), Qt::blue);
    QVERIFY(window);
    QVERIFY(effect->isActive());

    // Eventually, the animation will be complete.
    QTRY_VERIFY(!effect->isActive());

    // Close the test window, the effect should start animating the disappearing
    // of the window.
    QSignalSpy windowClosedSpy(window, &Window::windowClosed);
    QVERIFY(windowClosedSpy.isValid());
    shellSurface.reset();
    surface.reset();
    QVERIFY(windowClosedSpy.wait());
    QVERIFY(effect->isActive());

    // Eventually, the animation will be complete.
    QTRY_VERIFY(!effect->isActive());
}

void ToplevelOpenCloseAnimationTest::testDontAnimatePopups_data()
{
    QTest::addColumn<QString>("effectName");

    QTest::newRow("Fade") << QStringLiteral("kwin4_effect_fade");
    QTest::newRow("Glide") << QStringLiteral("glide");
    QTest::newRow("Scale") << QStringLiteral("kwin4_effect_scale");
}

void ToplevelOpenCloseAnimationTest::testDontAnimatePopups()
{
    // This test verifies that window open/close animation effects don't try
    // to animate popups(e.g. popup menus, tooltips, etc).

    // Make sure that we have the right effects ptr.
    auto effectsImpl = qobject_cast<EffectsHandlerImpl *>(effects);
    QVERIFY(effectsImpl);

    // Create the main window.
    using namespace KWayland::Client;
    QScopedPointer<KWayland::Client::Surface> mainWindowSurface(Test::createSurface());
    QVERIFY(!mainWindowSurface.isNull());
    QScopedPointer<Test::XdgToplevel> mainWindowShellSurface(Test::createXdgToplevelSurface(mainWindowSurface.data()));
    QVERIFY(!mainWindowShellSurface.isNull());
    Window *mainWindow = Test::renderAndWaitForShown(mainWindowSurface.data(), QSize(100, 50), Qt::blue);
    QVERIFY(mainWindow);

    // Load effect that will be tested.
    QFETCH(QString, effectName);
    QVERIFY(effectsImpl->loadEffect(effectName));
    QCOMPARE(effectsImpl->loadedEffects().count(), 1);
    QCOMPARE(effectsImpl->loadedEffects().first(), effectName);
    Effect *effect = effectsImpl->findEffect(effectName);
    QVERIFY(effect);
    QVERIFY(!effect->isActive());

    // Create a popup, it should not be animated.
    QScopedPointer<KWayland::Client::Surface> popupSurface(Test::createSurface());
    QVERIFY(!popupSurface.isNull());
    QScopedPointer<Test::XdgPositioner> positioner(Test::createXdgPositioner());
    QVERIFY(positioner);
    positioner->set_size(20, 20);
    positioner->set_anchor_rect(0, 0, 10, 10);
    positioner->set_gravity(Test::XdgPositioner::gravity_bottom_right);
    positioner->set_anchor(Test::XdgPositioner::anchor_bottom_left);
    QScopedPointer<Test::XdgPopup> popupShellSurface(Test::createXdgPopupSurface(popupSurface.data(), mainWindowShellSurface->xdgSurface(), positioner.data()));
    QVERIFY(!popupShellSurface.isNull());
    Window *popup = Test::renderAndWaitForShown(popupSurface.data(), QSize(20, 20), Qt::red);
    QVERIFY(popup);
    QVERIFY(popup->isPopupWindow());
    QCOMPARE(popup->transientFor(), mainWindow);
    QVERIFY(!effect->isActive());

    // Destroy the popup, it should not be animated.
    QSignalSpy popupClosedSpy(popup, &Window::windowClosed);
    QVERIFY(popupClosedSpy.isValid());
    popupShellSurface.reset();
    popupSurface.reset();
    QVERIFY(popupClosedSpy.wait());
    QVERIFY(!effect->isActive());

    // Destroy the main window.
    mainWindowSurface.reset();
    QVERIFY(Test::waitForWindowDestroyed(mainWindow));
}

WAYLANDTEST_MAIN(ToplevelOpenCloseAnimationTest)
#include "toplevel_open_close_animation_test.moc"

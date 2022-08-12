/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "screenlockerwatcher.h"
#include "wayland_server.h"

// dbus generated
#include "kscreenlocker_interface.h"
#include "screenlocker_interface.h"

namespace KWin
{

KWIN_SINGLETON_FACTORY(ScreenLockerWatcher)

static const QString SCREEN_LOCKER_SERVICE_NAME = QStringLiteral("org.freedesktop.ScreenSaver");

ScreenLockerWatcher::ScreenLockerWatcher(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(new QDBusServiceWatcher(this))
    , m_locked(false)
{
    if (waylandServer() && waylandServer()->hasScreenLockerIntegration()) {
        connect(waylandServer(), &WaylandServer::initialized, this, &ScreenLockerWatcher::initialize);
    } else {
        initialize();
    }
}

ScreenLockerWatcher::~ScreenLockerWatcher()
{
}

void ScreenLockerWatcher::initialize()
{
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &ScreenLockerWatcher::serviceOwnerChanged);
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    m_serviceWatcher->addWatchedService(SCREEN_LOCKER_SERVICE_NAME);

    m_interface = new OrgFreedesktopScreenSaverInterface(SCREEN_LOCKER_SERVICE_NAME, QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus(), this);
    m_kdeInterface = new OrgKdeScreensaverInterface(SCREEN_LOCKER_SERVICE_NAME, QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus(), this);
    connect(m_interface, &OrgFreedesktopScreenSaverInterface::ActiveChanged,
            this, &ScreenLockerWatcher::setLocked);
    connect(m_kdeInterface, &OrgKdeScreensaverInterface::AboutToLock, this, &ScreenLockerWatcher::aboutToLock);

    queryActive();
}

void ScreenLockerWatcher::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)
    Q_UNUSED(serviceName)

    m_locked = false;

    if (!newOwner.isEmpty()) {
        queryActive();
    }
}

void ScreenLockerWatcher::queryActive()
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(m_interface->GetActive(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &ScreenLockerWatcher::activeQueried);
}

void ScreenLockerWatcher::activeQueried(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool> reply = *watcher;
    if (!reply.isError()) {
        setLocked(reply.value());
    }
    watcher->deleteLater();
}

void ScreenLockerWatcher::setLocked(bool activated)
{
    if (m_locked == activated) {
        return;
    }
    m_locked = activated;
    Q_EMIT locked(m_locked);
}

}

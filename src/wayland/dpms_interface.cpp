/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
#include "display.h"
#include "dpms_interface_p.h"
#include "output_interface.h"

namespace KWaylandServer
{
static const quint32 s_version = 1;

DpmsManagerInterfacePrivate::DpmsManagerInterfacePrivate(DpmsManagerInterface *_q, Display *display)
    : QtWaylandServer::org_kde_kwin_dpms_manager(*display, s_version)
    , q(_q)
{
}

void DpmsManagerInterfacePrivate::org_kde_kwin_dpms_manager_get(Resource *resource, uint32_t id, wl_resource *output)
{
    OutputInterface *o = OutputInterface::get(output);

    wl_resource *dpms_resource = wl_resource_create(resource->client(), &org_kde_kwin_dpms_interface, resource->version(), id);
    if (!dpms_resource) {
        wl_client_post_no_memory(resource->client());
        return;
    }
    auto dpms = new DpmsInterface(o, dpms_resource);

    dpms->sendSupported();
    dpms->sendMode();
    dpms->sendDone();
}

DpmsManagerInterface::DpmsManagerInterface(Display *display, QObject *parent)
    : QObject(parent)
    , d(new DpmsManagerInterfacePrivate(this, display))
{
}

DpmsManagerInterface::~DpmsManagerInterface() = default;

DpmsInterface::DpmsInterface(OutputInterface *output, wl_resource *resource)
    : QObject()
    , QtWaylandServer::org_kde_kwin_dpms(resource)
    , output(output)
{
    connect(output, &OutputInterface::dpmsSupportedChanged, this, [this] {
        sendSupported();
        sendDone();
    });
    connect(output, &KWaylandServer::OutputInterface::dpmsModeChanged, this, [this] {
        sendMode();
        sendDone();
    });
}

DpmsInterface::~DpmsInterface() = default;

void DpmsInterface::org_kde_kwin_dpms_release(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void DpmsInterface::org_kde_kwin_dpms_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

void DpmsInterface::org_kde_kwin_dpms_set(Resource *resource, uint32_t mode)
{
    Q_UNUSED(resource)
    KWin::Output::DpmsMode dpmsMode;
    switch (mode) {
    case ORG_KDE_KWIN_DPMS_MODE_ON:
        dpmsMode = KWin::Output::DpmsMode::On;
        break;
    case ORG_KDE_KWIN_DPMS_MODE_STANDBY:
        dpmsMode = KWin::Output::DpmsMode::Standby;
        break;
    case ORG_KDE_KWIN_DPMS_MODE_SUSPEND:
        dpmsMode = KWin::Output::DpmsMode::Suspend;
        break;
    case ORG_KDE_KWIN_DPMS_MODE_OFF:
        dpmsMode = KWin::Output::DpmsMode::Off;
        break;
    default:
        return;
    }
    Q_EMIT output->dpmsModeRequested(dpmsMode);
}

void DpmsInterface::sendSupported()
{
    send_supported(output->isDpmsSupported() ? 1 : 0);
}

void DpmsInterface::sendMode()
{
    const auto mode = output->dpmsMode();
    org_kde_kwin_dpms_mode wlMode;
    switch (mode) {
    case KWin::Output::DpmsMode::On:
        wlMode = ORG_KDE_KWIN_DPMS_MODE_ON;
        break;
    case KWin::Output::DpmsMode::Standby:
        wlMode = ORG_KDE_KWIN_DPMS_MODE_STANDBY;
        break;
    case KWin::Output::DpmsMode::Suspend:
        wlMode = ORG_KDE_KWIN_DPMS_MODE_SUSPEND;
        break;
    case KWin::Output::DpmsMode::Off:
        wlMode = ORG_KDE_KWIN_DPMS_MODE_OFF;
        break;
    default:
        Q_UNREACHABLE();
    }
    send_mode(wlMode);
}

void DpmsInterface::sendDone()
{
    send_done();
}

}

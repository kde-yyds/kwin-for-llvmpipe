/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "output.h"
#include "wayland/output_interface.h"
#include "wayland/utils.h"
#include "wayland/xdgoutput_v1_interface.h"

#include <QTimer>

namespace KWin
{

class WaylandOutput : public QObject
{
    Q_OBJECT

public:
    explicit WaylandOutput(Output *output, QObject *parent = nullptr);

    KWaylandServer::OutputInterface *waylandOutput() const;

private Q_SLOTS:
    void handleDpmsModeChanged();
    void handleDpmsModeRequested(KWin::Output::DpmsMode dpmsMode);

    void update();
    void scheduleUpdate();

private:
    Output *m_platformOutput;
    QTimer m_updateTimer;
    KWaylandServer::ScopedGlobalPointer<KWaylandServer::OutputInterface> m_waylandOutput;
    KWaylandServer::XdgOutputV1Interface *m_xdgOutputV1;
};

} // namespace KWin

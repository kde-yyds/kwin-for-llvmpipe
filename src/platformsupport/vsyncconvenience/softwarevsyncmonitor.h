/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "vsyncmonitor.h"

#include <QTimer>

namespace KWin
{

/**
 * The SoftwareVsyncMonitor class provides synthetic vblank events with constant interval.
 *
 * The software vsync monitor can never fail and it is always available. It can be used as
 * fallback if hardware based approaches to monitor vsync events are unavailable.
 *
 * The vblank interval can be changed by calling the setRefreshRate() function.
 */
class KWIN_EXPORT SoftwareVsyncMonitor : public VsyncMonitor
{
    Q_OBJECT

public:
    static SoftwareVsyncMonitor *create(QObject *parent);

    int refreshRate() const;
    void setRefreshRate(int refreshRate);

public Q_SLOTS:
    void arm() override;

private:
    explicit SoftwareVsyncMonitor(QObject *parent = nullptr);
    void handleSyntheticVsync();

    QTimer *m_softwareClock = nullptr;
    int m_refreshRate = 60000;
    std::chrono::nanoseconds m_vblankTimestamp = std::chrono::nanoseconds::zero();
};

} // namespace KWin

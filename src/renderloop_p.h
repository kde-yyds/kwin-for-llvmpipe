/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "renderjournal.h"
#include "renderloop.h"

#include <QTimer>

#include <optional>

namespace KWin
{

class KWIN_EXPORT RenderLoopPrivate
{
public:
    static RenderLoopPrivate *get(RenderLoop *loop);
    explicit RenderLoopPrivate(RenderLoop *q);

    void dispatch();
    void invalidate();

    void delayScheduleRepaint();
    void scheduleRepaint();
    void maybeScheduleRepaint();

    void notifyFrameFailed();
    void notifyFrameCompleted(std::chrono::nanoseconds timestamp);

    RenderLoop *q;
    std::chrono::nanoseconds lastPresentationTimestamp = std::chrono::nanoseconds::zero();
    std::chrono::nanoseconds nextPresentationTimestamp = std::chrono::nanoseconds::zero();
    QTimer compositeTimer;
    RenderJournal renderJournal;
    int refreshRate = 60000;
    int pendingFrameCount = 0;
    int inhibitCount = 0;
    bool pendingReschedule = false;
    bool pendingRepaint = false;
    RenderLoop::VrrPolicy vrrPolicy = RenderLoop::VrrPolicy::Never;
    std::optional<LatencyPolicy> latencyPolicy;
    Item *fullscreenItem = nullptr;

    enum class SyncMode {
        Fixed,
        Adaptive,
    };
    SyncMode presentMode = SyncMode::Fixed;
};

} // namespace KWin

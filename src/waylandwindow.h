/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "window.h"

namespace KWin
{

class WaylandWindow : public Window
{
    Q_OBJECT

public:
    WaylandWindow(KWaylandServer::SurfaceInterface *surface);

    QString captionNormal() const override;
    QString captionSuffix() const override;
    pid_t pid() const override;
    bool isClient() const override;
    bool isLockScreen() const override;
    bool isLocalhost() const override;
    Window *findModal(bool allow_itself = false) override;
    void resizeWithChecks(const QSize &size) override;
    void killWindow() override;
    QByteArray windowRole() const override;
    bool isShown() const override;
    bool isHiddenInternal() const override;
    void hideClient() override;
    void showClient() override;

    virtual QRect frameRectToBufferRect(const QRect &rect) const;
    bool isHidden() const;

    void updateDepth();
    void setCaption(const QString &caption);

protected:
    bool belongsToSameApplication(const Window *other, SameApplicationChecks checks) const override;
    bool belongsToDesktop() const override;
    void doSetActive() override;
    void updateCaption() override;
    WindowItem *createItem() override;

    void cleanGrouping();
    void updateGeometry(const QRect &rect);

private:
    void updateClientOutputs();
    void updateIcon();
    void updateResourceName();

    QString m_captionNormal;
    QString m_captionSuffix;
    bool m_isHidden = false;
};

} // namespace KWin

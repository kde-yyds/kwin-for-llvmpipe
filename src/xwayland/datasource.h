/*
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "wayland/abstract_data_source.h"

namespace KWin
{
namespace Xwl
{

/**
 * The XwlDataSource class represents a data source owned by the Xwayland data bridge. It's
 * used as a source in data transfers from X11 clients to Wayland clients.
 *
 * The XwlDataSource class is sealed as its destructor emits the aboutToBeDestroyed() signal.
 * If you decide to unseal it, ensure that the about to be destroyed signal is emitted properly!
 */
class XwlDataSource final : public KWaylandServer::AbstractDataSource
{
    Q_OBJECT

public:
    ~XwlDataSource() override;

    void requestData(const QString &mimeType, qint32 fd) override;
    void cancel() override;
    QStringList mimeTypes() const override;
    void setMimeTypes(const QStringList &mimeTypes);

    void accept(const QString &mimeType) override;
    KWaylandServer::DataDeviceManagerInterface::DnDActions supportedDragAndDropActions() const override;
    void setSupportedDndActions(KWaylandServer::DataDeviceManagerInterface::DnDActions dndActions);

    void dndAction(KWaylandServer::DataDeviceManagerInterface::DnDAction action) override;

    KWaylandServer::DataDeviceManagerInterface::DnDAction selectedDragAndDropAction()
    {
        return m_dndAction;
    }

    void dropPerformed() override
    {
        Q_EMIT dropped();
    }
    void dndFinished() override
    {
        Q_EMIT finished();
    }
    void dndCancelled() override
    {
        Q_EMIT cancelled();
    }

    bool isAccepted() const override;

Q_SIGNALS:
    void dataRequested(const QString &mimeType, qint32 fd);
    void dropped();
    void finished();
    void cancelled();

private:
    QStringList m_mimeTypes;
    KWaylandServer::DataDeviceManagerInterface::DnDActions m_supportedDndActions;
    KWaylandServer::DataDeviceManagerInterface::DnDAction m_dndAction = KWaylandServer::DataDeviceManagerInterface::DnDAction::None;
    bool m_accepted = false;
};
}
}

/*
    SPDX-FileCopyrightText: 2017 Martin Graesslin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/
#ifndef KWIN_POPUP_INPUT_FILTER
#define KWIN_POPUP_INPUT_FILTER

#include "input.h"

#include <QObject>
#include <QVector>

namespace KWin
{
class Window;

class PopupInputFilter : public QObject, public InputEventFilter
{
    Q_OBJECT
public:
    explicit PopupInputFilter();
    bool pointerEvent(QMouseEvent *event, quint32 nativeButton) override;
    bool keyEvent(QKeyEvent *event) override;
    bool touchDown(qint32 id, const QPointF &pos, quint32 time) override;

private:
    void handleWindowAdded(Window *client);
    void handleWindowRemoved(Window *client);
    void disconnectClient(Window *client);
    void cancelPopups();

    QVector<Window *> m_popupWindows;
};
}

#endif

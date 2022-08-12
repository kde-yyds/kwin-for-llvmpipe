/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2006 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KWIN_DELETED_H
#define KWIN_DELETED_H

#include "window.h"

namespace KWin
{

class KWIN_EXPORT Deleted : public Window
{
    Q_OBJECT

public:
    static Deleted *create(Window *c);
    // used by effects to keep the window around for e.g. fadeout effects when it's destroyed
    void refWindow();
    void unrefWindow();
    void discard();
    QMargins frameMargins() const override;
    int desktop() const override;
    QStringList activities() const override;
    QVector<VirtualDesktop *> desktops() const override;
    QPoint clientPos() const override;
    bool isDeleted() const override;
    xcb_window_t frameId() const override;
    void layoutDecorationRects(QRect &left, QRect &top, QRect &right, QRect &bottom) const override;
    Layer layer() const override
    {
        return m_layer;
    }
    bool isShade() const override
    {
        return m_shade;
    }
    bool isMinimized() const
    {
        return m_minimized;
    }
    bool isModal() const
    {
        return m_modal;
    }
    QList<Window *> mainWindows() const override
    {
        return m_mainWindows;
    }
    NET::WindowType windowType(bool direct = false, int supported_types = 0) const override;
    bool wasClient() const
    {
        return m_wasClient;
    }
    QByteArray windowRole() const override;

    bool isFullScreen() const override
    {
        return m_fullscreen;
    }

    bool keepAbove() const
    {
        return m_keepAbove;
    }
    bool keepBelow() const
    {
        return m_keepBelow;
    }

    QString caption() const
    {
        return m_caption;
    }

    QString captionNormal() const override { return m_caption; }
    QString captionSuffix() const override { return {}; }
    bool isCloseable() const override { return false; }
    bool isShown() const override { return false; }
    bool isHiddenInternal() const override { return false; }
    void hideClient() override { /* nothing to do */ }
    void showClient() override { /* nothing to do */ }
    Window *findModal(bool /*allow_itself*/) override { return nullptr; }
    bool isResizable() const override { return false; }
    bool isMovable() const override { return false; }
    bool isMovableAcrossScreens() const override { return false; }
    bool takeFocus() override { return false; }
    bool wantsInput() const override { return false; }
    void killWindow() override { /* nothing to do */ }
    void destroyWindow() override { /* nothing to do */ }
    void closeWindow() override { /* nothing to do */ }
    bool acceptsFocus() const override { return false; }
    bool belongsToSameApplication(const Window *other, SameApplicationChecks /*checks*/) const override { return other == this; }
    void moveResizeInternal(const QRect & /*rect*/, KWin::Window::MoveResizeMode /*mode*/) override { /* nothing to do */ }
    void updateCaption() override { /* nothing to do */ }
    void resizeWithChecks(const QSize&) override { /* nothing to do */ }
    WindowItem *createItem() override;

    /**
     * Returns whether the client was a popup.
     *
     * @returns @c true if the client was a popup, @c false otherwise.
     */
    bool isPopupWindow() const override
    {
        return m_wasPopupWindow;
    }

    QVector<uint> x11DesktopIds() const;

    /**
     * Whether this Deleted represents the outline.
     */
    bool isOutline() const override
    {
        return m_wasOutline;
    }
    bool isLockScreen() const override
    {
        return m_wasLockScreen;
    }

private Q_SLOTS:
    void mainWindowClosed(KWin::Window *window);

private:
    Deleted(); // use create()
    void copyToDeleted(Window *c);
    ~Deleted() override; // deleted only using unrefWindow()

    QMargins m_frameMargins;

    int delete_refcount;
    int desk;
    QStringList activityList;
    QRect contentsRect; // for clientPos()/clientSize()
    xcb_window_t m_frame;
    QVector<VirtualDesktop *> m_desktops;

    QRect decoration_left;
    QRect decoration_right;
    QRect decoration_top;
    QRect decoration_bottom;
    Layer m_layer;
    bool m_shade;
    bool m_minimized;
    bool m_modal;
    QList<Window *> m_mainWindows;
    bool m_wasClient;
    NET::WindowType m_type = NET::Unknown;
    QByteArray m_windowRole;
    bool m_fullscreen;
    bool m_keepAbove;
    bool m_keepBelow;
    QString m_caption;
    bool m_wasPopupWindow;
    bool m_wasOutline;
    bool m_wasLockScreen;
};

inline void Deleted::refWindow()
{
    ++delete_refcount;
}

} // namespace

Q_DECLARE_METATYPE(KWin::Deleted *)

#endif
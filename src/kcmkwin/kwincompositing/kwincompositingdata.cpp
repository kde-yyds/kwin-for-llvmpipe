/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kwincompositingdata.h"

#include "kwincompositing_setting.h"

KWinCompositingData::KWinCompositingData(QObject *parent, const QVariantList &args)
    : KCModuleData(parent, args)
    , m_settings(new KWinCompositingSetting(this))

{
}

bool KWinCompositingData::isDefaults() const
{
    bool defaults = true;

    const KConfigSkeletonItem::List itemList = m_settings->items();
    for (const auto &item : itemList) {
        if (item->key() != QStringLiteral("OpenGLIsUnsafe")) {
            defaults &= item->isDefault();
        }
    }
    return defaults;
}

#include "kwincompositingdata.moc"

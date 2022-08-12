/*
    SPDX-FileCopyrightText: 2017 Demitrius Belai <demitriusbelai@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AURORAEPACKAGE_H
#define AURORAEPACKAGE_H

#include <KPackage/PackageStructure>

class AuroraePackage : public KPackage::PackageStructure
{
public:
    AuroraePackage(QObject *, const QVariantList &)
    {
    }
    void initPackage(KPackage::Package *package) override;
    void pathChanged(KPackage::Package *package) override;
};

#endif

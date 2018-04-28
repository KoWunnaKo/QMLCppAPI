/*
 *   This is part of the FaceLift project
 *   Copyright (C) 2017 Pelagicore AB
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the LGPL 2.1 license.
 *   Please see the LICENSE file for details.
 */

#include "FaceliftModel.h"

namespace facelift {

ModelElementID StructureBase::s_nextID = 0;
constexpr int StructureBase::ROLE_ID;

ServiceRegistry::~ServiceRegistry()
{
}

void ServiceRegistry::registerObject(InterfaceBase *i)
{
    m_objects.append(i);

    // Notify later since our object is not yet fully constructed at this point in time
    QTimer::singleShot(0, [this, i] () {
            objectRegistered(i);
        });
}

ServiceRegistry &ServiceRegistry::instance()
{
    static ServiceRegistry reg;
    return reg;
}

void ModuleBase::registerQmlTypes(const char *uri, int majorVersion, int minorVersion)
{
    qmlRegisterUncreatableType<facelift::StructureBase>(uri, majorVersion, minorVersion, "StructureBase", "");
    qmlRegisterUncreatableType<facelift::InterfaceBase>(uri, majorVersion, minorVersion, "InterfaceBase", "");
}

}

/****************************************************************************
 * This file is part of Hawaii Shell.
 *
 * Copyright (C) 2013 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:LGPL2.1+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

#include <QtDBus/QDBusReply>
#include <QtCore/QProcess>

#include "upowerpowerbackend.h"

#define UPOWER_SERVICE QStringLiteral("org.freedesktop.UPower")
#define UPOWER_PATH QStringLiteral("/org/freedesktop/UPower")
#define UPOWER_OBJECT QStringLiteral("org.freedesktop.UPower")

QString UPowerPowerBackend::service()
{
    return UPOWER_SERVICE;
}

UPowerPowerBackend::UPowerPowerBackend()
{
    m_interface = new QDBusInterface(UPOWER_SERVICE, UPOWER_PATH,
                                     UPOWER_OBJECT, QDBusConnection::systemBus());
}

UPowerPowerBackend::~UPowerPowerBackend()
{
    delete m_interface;
}

QString UPowerPowerBackend::name() const
{
    return QStringLiteral("upower");
}

PowerCapabilities UPowerPowerBackend::capabilities() const
{
    PowerCapabilities caps = PowerCapability::None;

    QDBusReply<QString> reply;

    reply = m_interface->call(QStringLiteral("SuspendAllowed"));
    if (reply.isValid() && reply.value() == QStringLiteral("yes"))
        caps |= PowerCapability::Suspend;

    reply = m_interface->call(QStringLiteral("HibernateAllowed"));
    if (reply.isValid() && reply.value() == QStringLiteral("yes"))
        caps |= PowerCapability::Hibernate;

    return caps;
}

void UPowerPowerBackend::powerOff()
{
    QProcess::execute(QStringLiteral("/sbin/poweroff"));
}

void UPowerPowerBackend::restart()
{
    QProcess::execute(QStringLiteral("/sbin/reboot"));
}

void UPowerPowerBackend::suspend()
{
    m_interface->call(QStringLiteral("Suspend"));
}

void UPowerPowerBackend::hibernate()
{
    m_interface->call(QStringLiteral("Hibernate"));
}

void UPowerPowerBackend::hybridSleep()
{
}

/****************************************************************************
 * This file is part of Liri.
 *
 * Copyright (C) 2018 Pier Luigi Fiorini
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:GPL3+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

#include <QtCore/QStandardPaths>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include <qt5xdg/xdgdesktopfile.h>

#include "processlauncher.h"
#include "processlauncher_adaptor.h"

Q_LOGGING_CATEGORY(LAUNCHER, "liri.launcher")

ProcessLauncher::ProcessLauncher(QObject *parent)
    : QObject(parent)
{
}

ProcessLauncher::~ProcessLauncher()
{
    closeApplications();
}

QString ProcessLauncher::waylandSocketName() const
{
    return m_waylandSocketName;
}

void ProcessLauncher::setWaylandSocketName(const QString &name)
{
    if (m_waylandSocketName == name)
        return;

    m_waylandSocketName = name;
    Q_EMIT waylandSocketNameChanged();
}

void ProcessLauncher::closeApplications()
{
    qCDebug(LAUNCHER) << "Terminate applications";

    // Terminate all process launched by us
    ApplicationMapIterator i(m_apps);
    while (i.hasNext()) {
        i.next();

        QString fileName = i.key();
        QProcess *process = i.value();

        i.remove();

        qCDebug(LAUNCHER) << "Terminating application from" << fileName << "with pid"
                          << process->pid();

        process->terminate();
        if (!process->waitForFinished())
            process->kill();
        process->deleteLater();
    }
}

bool ProcessLauncher::registerWithDBus(ProcessLauncher *instance)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    new ProcessLauncherAdaptor(instance);
    if (!bus.registerObject(QStringLiteral("/ProcessLauncher"), instance)) {
        qCWarning(LAUNCHER, "Couldn't register /ProcessLauncher D-Bus object: %s",
                  qPrintable(bus.lastError().message()));
        return false;
    }

    return true;
}

bool ProcessLauncher::launchApplication(const QString &appId)
{
    if (appId.isEmpty()) {
        qCWarning(LAUNCHER) << "Empty appId passed to ProcessLauncher::launchApplication()";
        return false;
    }

    const QString fileName = QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                                    appId + QStringLiteral(".desktop"));
    if (fileName.isEmpty()) {
        qCWarning(LAUNCHER) << "Cannot find" << appId << "desktop entry";
        return false;
    }

    XdgDesktopFile *entry = XdgDesktopFileCache::getFile(fileName);
    if (!entry) {
        qCWarning(LAUNCHER) << "No desktop entry found for" << appId;
        return false;
    }

    return launchEntry(*entry);
}

bool ProcessLauncher::launchDesktopFile(const QString &fileName)
{
    if (fileName.isEmpty()) {
        qCWarning(LAUNCHER) << "Empty file name passed to ProcessLauncher::launchDesktopFile()";
        return false;
    }

    XdgDesktopFile *entry = XdgDesktopFileCache::getFile(fileName);
    if (!entry) {
        qCWarning(LAUNCHER) << "Failed to open desktop file" << fileName;
        return false;
    }

    return launchEntry(*entry);
}

bool ProcessLauncher::launchCommand(const QString &command)
{
    if (command.isEmpty()) {
        qCWarning(LAUNCHER) << "Empty command passed to ProcessLauncher::launchCommand()";
        return false;
    }

    qCInfo(LAUNCHER) << "Launching command" << command;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_waylandSocketName.isEmpty())
        env.insert(QStringLiteral("WAYLAND_DISPLAY"), m_waylandSocketName);
    env.insert(QStringLiteral("SAL_USE_VCLPLUGIN"), QStringLiteral("kde"));
    env.insert(QStringLiteral("QT_PLATFORM_PLUGIN"), QStringLiteral("liri"));
    env.remove(QStringLiteral("QSG_RENDER_LOOP"));
    env.remove(QStringLiteral("EGL_PLATFORM"));

    QProcess *process = new QProcess(this);
    process->setProcessEnvironment(env);
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            &ProcessLauncher::finished);
    process->start(command);
    if (!process->waitForStarted()) {
        qCWarning(LAUNCHER) << "Failed to launch command" << command;
        return false;
    }

    qCInfo(LAUNCHER) << "Launched command" << command << "with pid" << process->pid();

    return true;
}

bool ProcessLauncher::closeApplication(const QString &appId)
{
    const QString fileName = appId + QStringLiteral(".desktop");
    return closeEntry(fileName);
}

bool ProcessLauncher::closeDesktopFile(const QString &fileName)
{
    return closeEntry(fileName);
}

bool ProcessLauncher::launchEntry(const XdgDesktopFile &entry)
{
    QStringList args = entry.expandExecString();
    QString command = args.takeAt(0);

    qCDebug(LAUNCHER) << "Launching" << entry.expandExecString().join(QStringLiteral(" ")) << "from"
                      << entry.fileName();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_waylandSocketName.isEmpty())
        env.insert(QStringLiteral("WAYLAND_DISPLAY"), m_waylandSocketName);
    env.insert(QStringLiteral("SAL_USE_VCLPLUGIN"), QStringLiteral("kde"));
    env.insert(QStringLiteral("QT_PLATFORM_PLUGIN"), QStringLiteral("liri"));
    env.remove(QStringLiteral("QSG_RENDER_LOOP"));
    env.remove(QStringLiteral("EGL_PLATFORM"));

    QProcess *process = new QProcess(this);
    process->setProgram(command);
    process->setArguments(args);
    process->setProcessEnvironment(env);
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_apps[entry.fileName()] = process;
    connect(process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            &ProcessLauncher::finished);
    process->start();
    if (!process->waitForStarted()) {
        qCWarning(LAUNCHER, "Failed to launch \"%s\" (%s)", qPrintable(entry.fileName()),
                  qPrintable(entry.name()));
        return false;
    }

    qCDebug(LAUNCHER, "Launched \"%s\" (%s) with pid %lld", qPrintable(entry.fileName()),
            qPrintable(entry.name()), process->pid());

    return true;
}

bool ProcessLauncher::closeEntry(const QString &fileName)
{
    if (!m_apps.contains(fileName))
        return false;

    qCInfo(LAUNCHER) << "Closing application for" << fileName;

    QProcess *process = m_apps[fileName];
    process->terminate();
    if (!process->waitForFinished())
        process->kill();
    return true;
}

void ProcessLauncher::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    QString fileName = m_apps.key(process);
    if (!fileName.isEmpty()) {
        qCDebug(LAUNCHER) << "Application for" << fileName << "finished with exit code" << exitCode;
        m_apps.remove(fileName);
    }

    process->deleteLater();
}

#include "moc_processlauncher.cpp"

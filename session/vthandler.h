/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:GPL2+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#ifndef VTHANDLER_H
#define VTHANDLER_H

#include <QtCore/QObject>

class VtHandler : public QObject
{
    Q_OBJECT
public:
    VtHandler(QObject *parent = 0);
    ~VtHandler();

    bool isActive() const;

    void activate(int vt);

Q_SIGNALS:
    void activeChanged(bool value);

private:
    int m_oldKbdMode;
    bool m_active;

    void setActive(bool value);
};

#endif // VTHANDLER_H

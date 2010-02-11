/***************************************************************************
 *   Copyright (C) 2010 Daniel Nicoletti <dantti85-pk@yahoo.com.br>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef PRINT_QUEUE_INTERFACE_H
#define PRINT_QUEUE_INTERFACE_H

#include <QtDBus/QDBusContext>
#include <QTimer>

class PrintQueueUi;

class PrintQueueInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.PrintQueue")
public:
    PrintQueueInterface(QObject *parent = 0);
    ~PrintQueueInterface();

    bool canQuit();

signals:
    void quit();

public slots:
    void ShowQueue(const QString &destName);

private slots:
    void RemoveQueue();

private:
    QTimer *m_updateUi;
    QHash<QString, PrintQueueUi *> m_uis;
};

#endif
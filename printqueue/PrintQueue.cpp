/***************************************************************************
 *   Copyright (C) 2010 by Daniel Nicoletti                                *
 *   dantti85-pk@yahoo.com.br                                              *
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
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include "PrintQueue.h"

#include "PrintQueueInterface.h"

#include <KCmdLineArgs>
#include <KDebug>

PrintQueue::PrintQueue()
 : KUniqueApplication()
{
    m_pqInterface = new PrintQueueInterface(this);
    connect(m_pqInterface, SIGNAL(quit()), this, SLOT(quit()));
}

int PrintQueue::newInstance()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->isSet("show-queue")) {
        m_pqInterface->ShowQueue(args->getOption("show-queue"));
    }

    if (m_pqInterface->canQuit()) {
        quit();
    }

    return 0;
}

PrintQueue::~PrintQueue()
{
}

#include "PrintQueue.moc"
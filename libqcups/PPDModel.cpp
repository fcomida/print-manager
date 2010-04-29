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

#include "PPDModel.h"

#include <KDebug>

#include "QCups.h"

using namespace QCups;

PPDModel::PPDModel(const QList<QHash<QString, QVariant> > &ppds, QObject *parent)
 : QAbstractListModel(parent),
   m_ppds(ppds)
{
}

QVariant PPDModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int row = index.row();
    switch (role) {
    case Qt::DisplayRole:
        return QString("%1 (%2)")
               .arg(m_ppds.at(row)["ppd-make-and-model"].toString())
               .arg(m_ppds.at(row)["ppd-natural-language"].toString());
    case PPDMake:
        return m_ppds.at(row)["ppd-make"].toString();
    case PPDName:
        return m_ppds.at(row)["ppd-name"].toString();
    case PPDMakeAndModel:
        return m_ppds.at(row)["ppd-make-and-model"].toString();
    default:
        return QVariant();
    }
}

int PPDModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_ppds.size();
}

Qt::ItemFlags PPDModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

#include "PPDModel.moc"

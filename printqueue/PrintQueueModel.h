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

#ifndef PRINT_QUEUE_MODEL_H
#define PRINT_QUEUE_MODEL_H

#include <QStandardItemModel>

#include <cups/cups.h>

class PrintQueueModel : public QStandardItemModel
{
    Q_OBJECT
    Q_ENUMS(JobAction)
    Q_ENUMS(Role)
public:
    typedef enum {
        JobId = Qt::UserRole + 2,
        JobState,
        DestName
    } Role;

    typedef enum {
        Cancel,
        Hold,
        Release
    } JobAction;

    typedef enum {
        ColStatus = 0,
        ColName,
        ColUser,
        ColCreated,
        ColCompleted,
        ColPrinter
    } Columns;

    PrintQueueModel(const QString &destName, WId parentId, QObject *parent = 0);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setWhichJobs(int whichjobs);
    bool modifyJob(int row, JobAction action, const QModelIndex &parent = QModelIndex());
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent);

public slots:
    void updateModel();

private:
    QString m_destName;
    int m_whichjobs;
    bool m_showPrinterColumn;
    WId m_parentId;

    int jobRow(int jobId);
    void insertJob(int pos, cups_job_s job);
    void updateJob(int pos, cups_job_s job);
    QString jobStatus(ipp_jstate_t job_state);
};

#endif
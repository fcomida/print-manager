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

#include "QCups.h"
#include "cupsActions.h"
#include <cups/cups.h>

#include <KPasswordDialog>
#include <KLocale>
#include <KDebug>

using namespace QCups;

#define RUN_ACTION(blurb) \
                password_retries = 0; \
                do { \
                    blurb; \
                } while (retry()); \
                return !cupsLastError(); \

static uint password_retries = 0;
bool retry();

const char * my_password_cb(const char *)
{
    kDebug() << "password_retries" << password_retries;
    if (password_retries == 3) {
        // cancel the authentication
        cupsSetUser(NULL);
        return NULL;
    }
    KPasswordDialog dlg(0, KPasswordDialog::ShowUsernameLine);
    dlg.setPrompt(i18n("Enter an username and a password to complete the task"));
    dlg.setUsername(QString::fromLocal8Bit(cupsUser()));
    // check if the password retries is more than 0 and show an error
    if (password_retries++) {
        dlg.showErrorMessage(QString(), KPasswordDialog::UsernameError);
        dlg.showErrorMessage(i18n("Wrong username or password"), KPasswordDialog::PasswordError);
    }
    dlg.setUsername(QString::fromLocal8Bit(cupsUser()));
    if (dlg.exec()) {
        cupsSetUser(dlg.username().toLocal8Bit().data());
        return dlg.password().toLocal8Bit().data();
    }
    // the dialog was canceled
    password_retries = -1;
    cupsSetUser(NULL);
    return NULL;
}

bool retry()
{
//     kDebug() << "cupsLastErrorString()" << cupsLastErrorString();
    if (cupsLastError() == IPP_FORBIDDEN ||
        cupsLastError() == IPP_NOT_AUTHORIZED ||
        cupsLastError() == IPP_NOT_AUTHENTICATED) {
        switch (password_retries) {
        case 0:
            // try to authenticate as the root user
            cupsSetUser("root");
            break;
        case -1:
        case 3:
            // the authentication failed 3 times
            // OR the dialog was canceld (-1)
            // reset to 0 and quit the do-while loop
            password_retries = 0;
            return false;
        }

        // force authentication
        if (cupsDoAuthentication(CUPS_HTTP_DEFAULT, "POST", "/") == 0) {
            // tries to do the action again
            // sometimes just trying to be root works
        }

        return true;
    }
    // the action was not forbidden
    return false;
}

void QCups::initialize()
{
    cupsSetPasswordCB(my_password_cb);
}

bool QCups::moveJob(const QString &name, int job_id, const QString &dest_name)
{
    RUN_ACTION(cupsMoveJob(name.toLocal8Bit().data(), job_id, dest_name.toLocal8Bit().data()))
}

bool QCups::pausePrinter(const QString &name)
{
    RUN_ACTION(cupsPauseResumePrinter(name.toLocal8Bit().data(), true))
}

bool QCups::resumePrinter(const QString &name)
{
    RUN_ACTION(cupsPauseResumePrinter(name.toLocal8Bit().data(), false))
}

bool QCups::setDefaultPrinter(const QString &name)
{
    RUN_ACTION(cupsSetDefaultPrinter(name.toLocal8Bit().data()))
}

bool QCups::deletePrinter(const QString &name)
{
    RUN_ACTION(cupsDeletePrinter(name.toLocal8Bit().data()))
}

bool QCups::cancelJob(const QString &name, int job_id)
{
    RUN_ACTION(cupsCancelJob(name.toLocal8Bit().data(), job_id))
}

bool QCups::holdJob(const QString &name, int job_id)
{
    RUN_ACTION(cupsHoldReleaseJob(name.toLocal8Bit().data(), job_id, true))
}

bool QCups::releaseJob(const QString &name, int job_id)
{
    RUN_ACTION(cupsHoldReleaseJob(name.toLocal8Bit().data(), job_id, false))
}

bool QCups::addModifyPrinter(const QString &name, const QHash<QString, QVariant> values)
{
    RUN_ACTION(cupsAddModifyPrinter(name.toLocal8Bit().data(), values))
}

bool Printer::setShared(const QString &destName, bool shared)
{
    QHash<QString, QVariant> values;
    values["printer-is-shared"] = shared;
    RUN_ACTION(cupsAddModifyPrinter(destName.toLocal8Bit().data(), values))
}

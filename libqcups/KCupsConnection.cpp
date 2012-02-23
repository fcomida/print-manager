/***************************************************************************
 *   Copyright (C) 2010-2012 by Daniel Nicoletti                           *
 *   dantti12@gmail.com                                                    *
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

#include "KCupsConnection.h"

#include <QCoreApplication>

#include <KLocale>
#include <KDebug>

#include <cups/cups.h>

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<bool>)

KCupsConnection* KCupsConnection::m_instance = 0;
static int password_retries = 0;
const char * password_cb(const char *prompt, http_t *http, const char *method, const char *resource, void *user_data);

KCupsConnection* KCupsConnection::global()
{
    if (!m_instance) {
        m_instance = new KCupsConnection(qApp);
    }

    return m_instance;
}

KCupsConnection::KCupsConnection(QObject *parent) :
    QThread(parent),
    m_inited(false),
    // Creating the dialog before start() will make it run on the gui thread
    m_passwordDialog(new KPasswordDialog(0L, KPasswordDialog::ShowUsernameLine))
{
    m_passwordDialog->setModal(true);
    m_passwordDialog->setPrompt(i18n("Enter an username and a password to complete the task"));

    // Starts this thread
    start();
}

KCupsConnection::~KCupsConnection()
{
    quit();
    wait();
}

void KCupsConnection::run()
{
    // This is dead cool, cups will call the thread_password_cb()
    // function when a password set is needed, as we passed the
    // password dialog pointer the functions just need to call
    // it on a blocking mode.
    cupsSetPasswordCB2(password_cb, m_passwordDialog);

    m_inited = true;
    exec();
}

bool KCupsConnection::readyToStart()
{
    if (QThread::currentThread() == KCupsConnection::global()) {
        password_retries = 0;
        return true;
    }
    return false;
}

ReturnArguments KCupsConnection::request(ipp_op_e       operation,
                                         const QString &resource,
                                         QVariantHash   reqValues,
                                         bool           needResponse)
{
    ReturnArguments ret;

    if (!readyToStart()) {
        return ret; // This is not intended to be used in the gui thread
    }

    ipp_t *response = NULL;
    bool needDestName = false;
    int group_tag = IPP_TAG_PRINTER;
    do {
        ipp_t *request;
        bool isClass = false;
        QString filename;
        QVariantHash values = reqValues;

        ippDelete(response);

        if (values.contains("printer-is-class")) {
            isClass = values.take("printer-is-class").toBool();
        }
        if (values.contains("need-dest-name")) {
            needDestName = values.take("need-dest-name").toBool();
        }
        if (values.contains("group-tag-qt")) {
            group_tag = values.take("group-tag-qt").toInt();
        }

        if (values.contains("filename")) {
            filename = values.take("filename").toString();
        }

        // Lets create the request
        if (values.contains("printer-name")) {
            request = ippNewDefaultRequest(values.take("printer-name").toString(), isClass, operation);
        } else {
            request = ippNewRequest(operation);
        }

        QVariantHash::const_iterator i = values.constBegin();
        while (i != values.constEnd()) {
            switch (i.value().type()) {
            case QVariant::Bool:
                if (i.key() == "printer-is-accepting-jobs") {
                    ippAddBoolean(request, IPP_TAG_PRINTER, "printer-is-accepting-jobs", i.value().toBool());
                } else {
                    ippAddBoolean(request, IPP_TAG_OPERATION,
                                  i.key().toUtf8(), i.value().toBool());
                }
                break;
            case QVariant::Int:
                if (i.key() == "job-id") {
                    ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER,
                                  "job-id", i.value().toInt());
                } else if (i.key() == "printer-state") {
                    ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_ENUM,
                                  "printer-state", IPP_PRINTER_IDLE);
                } else {
                    ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_ENUM,
                                  i.key().toUtf8(), i.value().toInt());
                }
                break;
            case QVariant::String:
                if (i.key() == "device-uri") {
                    // device uri has a different TAG
                    ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI,
                                "device-uri", "utf-8",
                                i.value().toString().toUtf8());
                } else if (i.key() == "job-printer-uri") {
                    const char* dest_name = i.value().toString().toUtf8();
                    char  destUri[HTTP_MAX_URI];
                    httpAssembleURIf(HTTP_URI_CODING_ALL, destUri, sizeof(destUri),
                                     "ipp", "utf-8", "localhost", ippPort(),
                                     "/printers/%s", dest_name);
                    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                                 "job-printer-uri", "utf-8", destUri);
                } else if (i.key() == "printer-uri") {
                    // needed for getJobs
                    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                                 "printer-uri", NULL, "ipp://localhost/");
                } else if (i.key() == "printer-op-policy" ||
                           i.key() == "printer-error-policy" ||
                           i.key() == "ppd-name") {
                    // printer-op-policy has a different TAG
                    ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_NAME,
                                i.key().toUtf8(), "utf-8",
                                i.value().toString().toUtf8());
                } else if (i.key() == "job-name") {
                    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                                 "job-name", "utf-8", i.value().toString().toUtf8());
                } else if (i.key() == "which-jobs") {
                    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                                 "which-jobs", "utf-8", i.value().toString().toUtf8());
                } else {
                    ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT,
                                i.key().toUtf8(), "utf-8",
                                i.value().toString().toUtf8());
                }
                break;
            case QVariant::StringList:
                {
                    ipp_attribute_t *attr;
                    QStringList list = i.value().value<QStringList>();
                    if (i.key() == "member-uris") {
                        attr = ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_URI,
                                            "member-uris", list.size(), "utf-8", NULL);
                    } else if (i.key() == "requested-attributes") {
                        attr = ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                                            "requested-attributes", list.size(), "utf-8", NULL);
                    } else {
                        attr = ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_NAME,
                                            i.key().toUtf8(), list.size(), "utf-8", NULL);
                    }
                    // Dump all the list values
                    for (int i = 0; i < list.size(); i++) {
                        // TODO valgrind says this leak but it does not work calling .data()
                        attr->values[i].string.text = qstrdup(list.at(i).toUtf8());
                    }
                }
                break;
            default:
                kWarning() << "type NOT recognized! This will be ignored:" << i.key() << "values" << i.value();
            }
            ++i;
        }

        // Do the request
        // do the request deleting the response
        if (filename.isEmpty()) {
            response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, resource.toUtf8());
        } else {
            response = cupsDoFileRequest(CUPS_HTTP_DEFAULT, request, resource.toUtf8(), filename.toUtf8());
        }

    } while (retryIfForbidden());

    if (response != NULL && needResponse) {
        ret = parseIPPVars(response, group_tag, needDestName);
    }
    ippDelete(response);

    return ret;
}

ReturnArguments KCupsConnection::parseIPPVars(ipp_t *response, int group_tag, bool needDestName)
{
    ipp_attribute_t *attr;
    ReturnArguments ret;

    for (attr = response->attrs; attr != NULL; attr = attr->next) {
       /*
        * Skip leading attributes until we hit a a group which can be a printer, job...
        */
        while (attr && attr->group_tag != group_tag) {
            attr = attr->next;
        }

        if (attr == NULL) {
            break;
        }

        /*
         * Pull the needed attributes from this printer...
         */
        QVariantHash destAttributes;
        for (; attr && attr->group_tag == group_tag; attr = attr->next) {
            if (attr->value_tag != IPP_TAG_INTEGER &&
                attr->value_tag != IPP_TAG_ENUM &&
                attr->value_tag != IPP_TAG_BOOLEAN &&
                attr->value_tag != IPP_TAG_TEXT &&
                attr->value_tag != IPP_TAG_TEXTLANG &&
                attr->value_tag != IPP_TAG_LANGUAGE &&
                attr->value_tag != IPP_TAG_NAME &&
                attr->value_tag != IPP_TAG_NAMELANG &&
                attr->value_tag != IPP_TAG_KEYWORD &&
                attr->value_tag != IPP_TAG_RANGE &&
                attr->value_tag != IPP_TAG_URI) {
                continue;
            }

            /*
             * Add a printer description attribute...
             */
            destAttributes[QString::fromUtf8(attr->name)] = ippAttrToVariant(attr);
        }

        /*
         * See if we have everything needed...
         */
        if (needDestName && destAttributes["printer-name"].toString().isEmpty()) {
            if (attr == NULL) {
                break;
            } else {
                continue;
            }
        }

        ret << destAttributes;

        if (attr == NULL) {
            break;
        }
    }
    return ret;
}

// Don't forget to delete the request
ipp_t* KCupsConnection::ippNewDefaultRequest(const QString &name, bool isClass, ipp_op_t operation)
{
    char  uri[HTTP_MAX_URI]; // printer URI
    ipp_t *request;

    QString destination;
    if (isClass) {
        destination = QLatin1String("/classes/") + name;
    } else {
        destination = QLatin1String("/printers/") + name;
    }

    // Create a new request
    // where we need:
    // * printer-uri
    // * requesting-user-name
    request = ippNewRequest(operation);
    httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", "utf-8", "localhost",
                     ippPort(), destination.toUtf8());
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
                 "utf-8", uri);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
                 "utf-8", cupsUser());
    return request;
}

QVariant KCupsConnection::ippAttrToVariant(ipp_attribute_t *attr)
{
    if (attr->num_values == 1 &&
        attr->value_tag != IPP_TAG_INTEGER &&
        attr->value_tag != IPP_TAG_ENUM &&
        attr->value_tag != IPP_TAG_BOOLEAN &&
        attr->value_tag != IPP_TAG_RANGE) {
        return QString::fromUtf8(attr->values[0].string.text);
    }

    if (attr->value_tag == IPP_TAG_INTEGER || attr->value_tag == IPP_TAG_ENUM) {
        if (attr->num_values == 1) {
            return attr->values[0].integer;
        } else {
            QList<int> values;
            for (int i = 0; i < attr->num_values; i++) {
                values << attr->values[i].integer;
            }
            return QVariant::fromValue(values);
        }
    } else if (attr->value_tag == IPP_TAG_BOOLEAN ) {
        if (attr->num_values == 1) {
            return static_cast<bool>(attr->values[0].integer);
        } else {
            QList<bool> values;
            for (int i = 0; i < attr->num_values; i++) {
                values << static_cast<bool>(attr->values[i].integer);
            }
            return QVariant::fromValue(values);
        }
    } else if (attr->value_tag == IPP_TAG_RANGE) {
        QVariantList values;
        for (int i = 0; i < attr->num_values; i++) {
            values << attr->values[i].range.lower;
            values << attr->values[i].range.upper;
        }
        return values;
    } else {
        QStringList values;
        for (int i = 0; i < attr->num_values; i++) {
            values << QString::fromUtf8(attr->values[i].string.text);
        }
        return values;
    }
}

bool KCupsConnection::retryIfForbidden()
{
    if (cupsLastError() == IPP_FORBIDDEN ||
        cupsLastError() == IPP_NOT_AUTHORIZED ||
        cupsLastError() == IPP_NOT_AUTHENTICATED) {
        if (password_retries == 0) {
            // Pretend to be the root user
            // Sometime seting this just works
            cupsSetUser("root");
        } else if (password_retries > 3 || password_retries == -1) {
            // the authentication failed 3 times
            // OR the dialog was canceld (-1)
            // reset to 0 and quit the do-while loop
            password_retries = 0;
            return false;
        }

        // force authentication
        kDebug() << "cupsLastErrorString()" << cupsLastErrorString() << cupsLastError();
        kDebug() << "cupsDoAuthentication" << password_retries;
        cupsDoAuthentication(CUPS_HTTP_DEFAULT, "POST", "/");
        // tries to do the action again
        // sometimes just trying to be root works
        return true;
    }

    // the action was not forbidden
    return false;
}

const char * password_cb(const char *prompt, http_t *http, const char *method, const char *resource, void *user_data)
{
    Q_UNUSED(prompt)
    Q_UNUSED(http)
    Q_UNUSED(method)
    Q_UNUSED(resource)

    if (++password_retries > 3) {
        // cancel the authentication
        cupsSetUser(NULL);
        return NULL;
    }

    KPasswordDialog *passwordDialog = static_cast<KPasswordDialog *>(user_data);
    passwordDialog->setUsername(QString::fromUtf8(cupsUser()));
    if (password_retries > 1) {
        passwordDialog->showErrorMessage(QString(), KPasswordDialog::UsernameError);
        passwordDialog->showErrorMessage(i18n("Wrong username or password"), KPasswordDialog::PasswordError);
    }

    // This will block this thread until exec is not finished
    QMetaObject::invokeMethod(passwordDialog,
                              "exec",
                              Qt::BlockingQueuedConnection);

    if (passwordDialog->result() == KDialog::Ok) {
        cupsSetUser(passwordDialog->username().toUtf8());
        return passwordDialog->password().toUtf8();
    } else {
        // the dialog was canceled
        password_retries = -1;
        cupsSetUser(NULL);
        return NULL;
    }
}

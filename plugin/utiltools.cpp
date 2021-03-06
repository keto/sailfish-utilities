#include "utiltools.h"

#include <QDir>
#include <QDebug>
#include <qqmlinfo.h>

static QString SystemTool("sailfish_tools_system_action");

UtilTools::UtilTools(QObject *parent)
    : QObject(parent)
{
}

UtilTools::~UtilTools()
{
}

void UtilTools::removeBackups(QJSValue successCallback, QJSValue errorCallback)
{
    QDir vaultDir(QDir::homePath() + "/.vault");

    if (vaultDir.removeRecursively()) {
        if (successCallback.isCallable()) {
            QJSValue result = successCallback.call();
            if (result.isError()) {
                qmlInfo(this) << "Error executing callback";
            }
        }
    } else {
        if (errorCallback.isCallable()) {
            QJSValue result = errorCallback.call();
            if (result.isError()) {
                qmlInfo(this) << "Error executing error callback";
            }
        }
    }
}

void UtilTools::cleanRpmDb(QJSValue successCallback, QJSValue errorCallback)
{
    execute(SystemTool, QStringList("repair_rpm_db"), successCallback, errorCallback);
}

void UtilTools::restartKeyboard(QJSValue successCallback, QJSValue errorCallback)
{
    QStringList arguments;
    arguments << "--user" << "restart" << "maliit-server.service";
    execute("systemctl", arguments, successCallback, errorCallback);
}

void UtilTools::cleanTrackerDb(QJSValue successCallback, QJSValue errorCallback)
{
    QStringList arguments;
    arguments << "--user" << "start" << "tracker-reindex.service";
    execute("systemctl", arguments, successCallback, errorCallback);
}

void UtilTools::restartNetwork(QJSValue successCallback, QJSValue errorCallback)
{
    execute(SystemTool, QStringList("restart_network"), successCallback, errorCallback);
}

void UtilTools::restartLipstick(QJSValue successCallback, QJSValue errorCallback)
{
    execute(SystemTool, QStringList("restart_lipstick"), successCallback, errorCallback);
}

void UtilTools::restartDevice(QJSValue successCallback, QJSValue errorCallback)
{
    execute(SystemTool, QStringList("restart_device"), successCallback, errorCallback);
}

void UtilTools::handleProcessExit(int exitCode, QProcess::ExitStatus status)
{
    QProcess *process = qobject_cast<QProcess*>(sender());

    if (!m_pendingCalls.contains(process)) {
        qmlInfo(this) << "Exit from unknown process";
        return;
    }

    QPair<QJSValue, QJSValue> callbacks = m_pendingCalls.take(process);

    if (status == QProcess::NormalExit && exitCode == 0) {
        if (callbacks.first.isCallable()) {
            QJSValue result = callbacks.first.call();
            if (result.isError()) {
                qmlInfo(this) << "Error executing callback";
            }
        }
    } else if (callbacks.second.isCallable()) {
        QJSValue result = callbacks.second.call();
        if (result.isError()) {
            qmlInfo(this) << "Error executing error callback";
        }
    }
}

void UtilTools::execute(const QString &command, const QStringList &arguments, QJSValue successCallback, QJSValue errorCallback)
{
    QProcess *process = new QProcess;
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(handleProcessExit(int,QProcess::ExitStatus)));
    m_pendingCalls.insert(process, QPair<QJSValue,QJSValue>(successCallback, errorCallback));
    process->start(command, arguments);
}

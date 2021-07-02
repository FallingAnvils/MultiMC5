#include "LaunchController.h"
#include "MainWindow.h"
#include "MultiServerMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ProgressDialog.h"
#include "InstanceWindow.h"
#include "BuildConfig.h"
#include "JavaCommon.h"
#include <QLineEdit>
#include <QInputDialog>
#include <tasks/Task.h>
#include <launch/steps/TextPrint.h>
#include <QStringList>
#include <QHostInfo>
#include <QList>
#include <QHostAddress>

LaunchController::LaunchController(QObject *parent) : Task(parent)
{
}

void LaunchController::executeTask()
{
    if (!m_instance)
    {
        emitFailed(tr("No instance specified!"));
        return;
    }

    login();
}

// FIXME: minecraft specific
void LaunchController::login()
{
    JavaCommon::checkJVMArgs(m_instance->settings()->get("JvmArgs").toString(), m_parentWidget);
    launchInstance();
    //emitFailed(tr("Failed to launch."));
}

void LaunchController::launchInstance()
{
    Q_ASSERT_X(m_instance != NULL, "launchInstance", "instance is NULL");
    if(!m_instance->reloadSettings())
    {
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't load the instance profile."));
        emitFailed(tr("Couldn't load the instance profile."));
        return;
    }

    m_launcher = m_instance->createLaunchTask(m_serverPort);
    if (!m_launcher)
    {
        emitFailed(tr("Couldn't instantiate a launcher."));
        return;
    }

    auto console = qobject_cast<InstanceWindow *>(m_parentWidget);
    auto showConsole = m_instance->settings()->get("ShowConsole").toBool();
    if(!console && showConsole)
    {
        MSMC->showInstanceWindow(m_instance);
    }
    connect(m_launcher.get(), &LaunchTask::readyForLaunch, this, &LaunchController::readyForLaunch);
    connect(m_launcher.get(), &LaunchTask::succeeded, this, &LaunchController::onSucceeded);
    connect(m_launcher.get(), &LaunchTask::failed, this,  &LaunchController::onFailed);
    connect(m_launcher.get(), &LaunchTask::requestProgress, this, &LaunchController::onProgressRequested);

    // Prepend Version
    m_launcher->prependStep(new TextPrint(m_launcher.get(), "MultiServerMC version: " + BuildConfig.printableVersionString() + "\n\n", MessageLevel::MultiServerMC));
    m_launcher->start();
}

void LaunchController::readyForLaunch()
{
    if (!m_profiler)
    {
        m_launcher->proceed();
        return;
    }

    QString error;
    if (!m_profiler->check(&error))
    {
        m_launcher->abort();
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't start profiler: %1").arg(error));
        emitFailed("Profiler startup failed!");
        return;
    }
    BaseProfiler *profilerInstance = m_profiler->createProfiler(m_launcher->instance(), this);

    connect(profilerInstance, &BaseProfiler::readyToLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("The game launch is delayed until you press the "
                        "button. This is the right time to setup the profiler, as the "
                        "profiler server is running now.\n\n%1").arg(message));
        msg.setWindowTitle(tr("Waiting."));
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("Launch"), QMessageBox::AcceptRole);
        msg.setModal(true);
        msg.exec();
        m_launcher->proceed();
    });
    connect(profilerInstance, &BaseProfiler::abortLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("Couldn't start the profiler: %1").arg(message));
        msg.setWindowTitle(tr("Error"));
        msg.setIcon(QMessageBox::Critical);
        msg.addButton(QMessageBox::Ok);
        msg.setModal(true);
        msg.exec();
        m_launcher->abort();
        emitFailed("Profiler startup failed!");
    });
    profilerInstance->beginProfiling(m_launcher);
}

void LaunchController::onSucceeded()
{
    emitSucceeded();
}

void LaunchController::onFailed(QString reason)
{
    if(m_instance->settings()->get("ShowConsoleOnError").toBool())
    {
        MSMC->showInstanceWindow(m_instance, "console");
    }
    emitFailed(reason);
}

void LaunchController::onProgressRequested(Task* task)
{
    ProgressDialog progDialog(m_parentWidget);
    progDialog.setSkipButton(true, tr("Abort"));
    m_launcher->proceed();
    progDialog.execWithTask(task);
}

bool LaunchController::abort()
{
    if(!m_launcher)
    {
        return true;
    }
    if(!m_launcher->canAbort())
    {
        return false;
    }
    auto response = CustomMessageBox::selectable(
            m_parentWidget, tr("Kill Minecraft?"),
            tr("This can cause the instance to get corrupted and should only be used if Minecraft "
            "is frozen for some reason"),
            QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
    if (response == QMessageBox::Yes)
    {
        return m_launcher->abort();
    }
    return false;
}

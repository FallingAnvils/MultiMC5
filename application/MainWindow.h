/* Copyright 2013-2021 MultiServerMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>

#include <QMainWindow>
#include <QProcess>
#include <QTimer>

#include "BaseInstance.h"
#include "net/NetJob.h"
#include "updater/GoUpdate.h"

class LaunchController;
class QToolButton;
class InstanceProxyModel;
class LabeledToolButton;
class QLabel;
class MinecraftLauncher;
class BaseProfilerFactory;
class GroupView;
class KonamiCode;
class InstanceTask;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    class Ui;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool eventFilter(QObject *obj, QEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent * event) override;

    void checkInstancePathForProblems();

    void updatesAllowedChanged(bool allowed);

    void droppedURLs(QList<QUrl> urls);
signals:
    void isClosing();

protected:
    QMenu * createPopupMenu() override;

private slots:

    void on_actionAbout_triggered();

    void on_actionAddInstance_triggered();

    void on_actionCopyInstance_triggered();

    void on_actionChangeInstGroup_triggered();

    void on_actionChangeInstIcon_triggered();
    void on_changeIconButton_clicked(bool)
    {
        on_actionChangeInstIcon_triggered();
    }

    void on_actionViewInstanceFolder_triggered();

    void on_actionConfig_Folder_triggered();

    void on_actionViewSelectedInstFolder_triggered();

    void on_actionViewSelectedMCFolder_triggered();

    void refreshInstances();

    void on_actionViewCentralModsFolder_triggered();

    void checkForUpdates();

    void on_actionSettings_triggered();

    void on_actionLaunchInstance_triggered();

    void on_actionDeleteInstance_triggered();

    void deleteGroup();

    void on_actionExportInstance_triggered();

    void on_actionRenameInstance_triggered();
    void on_renameButton_clicked(bool)
    {
        on_actionRenameInstance_triggered();
    }

    void on_actionEditInstance_triggered();

    void on_actionEditInstNotes_triggered();

    void on_actionWorlds_triggered();

    void taskEnd();

    /**
     * called when an icon is changed in the icon model.
     */
    void iconUpdated(QString);

    void showInstanceContextMenu(const QPoint &);

    void updateToolsMenu();

    void instanceActivated(QModelIndex);

    void instanceChanged(const QModelIndex &current, const QModelIndex &previous);

    void instanceSelectRequest(QString id);

    void instanceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void selectionBad();

    void startTask(Task *task);

    void updateAvailable(GoUpdate::Status status);

    void updateNotAvailable();

    /*!
     * Runs the DownloadTask and installs updates.
     */
    void downloadUpdates(GoUpdate::Status status);

    void konamiTriggered();

    void globalSettingsClosed();

private:
    void retranslateUi();

    void addInstance(QString url = QString());
    void activateInstance(InstancePtr instance);
    void setCatBackground(bool enabled);
    void updateInstanceToolIcon(QString new_icon);
    void setSelectedInstanceById(const QString &id);

    void runModalTask(Task *task);
    void instanceFromInstanceTask(InstanceTask *task);
    void finalizeInstance(InstancePtr inst);

private:
    std::unique_ptr<Ui> ui;

    // these are managed by Qt's memory management model!
    GroupView *view = nullptr;
    InstanceProxyModel *proxymodel = nullptr;
    QLabel *m_statusLeft = nullptr;
    KonamiCode * secretEventFilter = nullptr;

    unique_qobject_ptr<NetJob> skin_download_job;

    InstancePtr m_selectedInstance;
    QString m_currentInstIcon;

    // managed by the application object
    Task *m_versionLoadTask = nullptr;
};

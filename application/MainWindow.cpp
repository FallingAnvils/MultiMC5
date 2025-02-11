/* Copyright 2013-2021 MultiServerMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
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
#include "MultiServerMC.h"
#include "BuildConfig.h"

#include "MainWindow.h"

#include <QtCore/QVariant>
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QKeyEvent>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QShortcut>

#include <BaseInstance.h>
#include <Env.h>
#include <InstanceList.h>
#include <MSMCZip.h>
#include <icons/IconList.h>
#include <java/JavaUtils.h>
#include <java/JavaInstallList.h>
#include <launch/LaunchTask.h>
#include <BuildConfig.h>
#include <net/NetJob.h>
#include <net/Download.h>
#include <tools/BaseProfiler.h>
#include <updater/DownloadTask.h>
#include <updater/UpdateChecker.h>
#include <DesktopServices.h>
#include "InstanceWindow.h"
#include "InstancePageProvider.h"
#include "InstanceProxyModel.h"
#include "JavaCommon.h"
#include "LaunchController.h"
#include "groupview/GroupView.h"
#include "groupview/InstanceDelegate.h"
#include "widgets/LabeledToolButton.h"
#include "dialogs/NewInstanceDialog.h"
#include "dialogs/ProgressDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/VersionSelectDialog.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/IconPickerDialog.h"
#include "dialogs/CopyInstanceDialog.h"
#include "dialogs/UpdateDialog.h"
#include "dialogs/ExportInstanceDialog.h"
#include <InstanceImportTask.h>
#include "UpdateController.h"
#include "KonamiCode.h"
#include <InstanceCopyTask.h>

// WHY: to hold the pre-translation strings together with the T pointer, so it can be retranslated without a lot of ugly code
template <typename T>
class Translated
{
public:
    Translated(){}
    Translated(QWidget *parent)
    {
        m_contained = new T(parent);
    }
    void setTooltipId(const char * tooltip)
    {
        m_tooltip = tooltip;
    }
    void setTextId(const char * text)
    {
        m_text = text;
    }
    operator T*()
    {
        return m_contained;
    }
    T * operator->()
    {
        return m_contained;
    }
    void retranslate()
    {
        if(m_text)
        {
            m_contained->setText(QApplication::translate("MainWindow", m_text));
        }
        if(m_tooltip)
        {
            m_contained->setToolTip(QApplication::translate("MainWindow", m_tooltip));
        }
    }
private:
    T * m_contained = nullptr;
    const char * m_text = nullptr;
    const char * m_tooltip = nullptr;
};
using TranslatedAction = Translated<QAction>;
using TranslatedToolButton = Translated<QToolButton>;

class TranslatedToolbar
{
public:
    TranslatedToolbar(){}
    TranslatedToolbar(QWidget *parent)
    {
        m_contained = new QToolBar(parent);
    }
    void setWindowTitleId(const char * title)
    {
        m_title = title;
    }
    operator QToolBar*()
    {
        return m_contained;
    }
    QToolBar * operator->()
    {
        return m_contained;
    }
    void retranslate()
    {
        if(m_title)
        {
            m_contained->setWindowTitle(QApplication::translate("MainWindow", m_title));
        }
    }
private:
    QToolBar * m_contained = nullptr;
    const char * m_title = nullptr;
};

class MainWindow::Ui
{
public:
    TranslatedAction actionAddInstance;
    //TranslatedAction actionRefresh;
    TranslatedAction actionCheckUpdate;
    TranslatedAction actionSettings;
    TranslatedAction actionLaunchInstance;
    TranslatedAction actionRenameInstance;
    TranslatedAction actionChangeInstGroup;
    TranslatedAction actionChangeInstIcon;
    TranslatedAction actionEditInstNotes;
    TranslatedAction actionEditInstance;
    TranslatedAction actionWorlds;
    TranslatedAction actionViewSelectedInstFolder;
    TranslatedAction actionViewSelectedMCFolder;
    TranslatedAction actionDeleteInstance;
    TranslatedAction actionConfig_Folder;
    TranslatedAction actionCAT;
    TranslatedAction actionCopyInstance;
    TranslatedAction actionScreenshots;
    TranslatedAction actionExportInstance;
    QVector<TranslatedAction *> all_actions;

    LabeledToolButton *renameButton = nullptr;
    LabeledToolButton *changeIconButton = nullptr;

    QMenu * foldersMenu = nullptr;
    TranslatedToolButton foldersMenuButton;
    TranslatedAction actionViewInstanceFolder;
    TranslatedAction actionViewCentralModsFolder;

    QMenu * helpMenu = nullptr;
    TranslatedToolButton helpMenuButton;
    TranslatedAction actionReportBug;
    TranslatedAction actionDISCORD;
    TranslatedAction actionREDDIT;
    TranslatedAction actionAbout;

    QVector<TranslatedToolButton *> all_toolbuttons;

    QWidget *centralWidget = nullptr;
    QHBoxLayout *horizontalLayout = nullptr;
    QStatusBar *statusBar = nullptr;

    TranslatedToolbar mainToolBar;
    TranslatedToolbar instanceToolBar;
    QVector<TranslatedToolbar *> all_toolbars;
    bool m_kill = false;

    void updateLaunchAction()
    {
        if(m_kill)
        {
            actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Kill"));
            actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Kill the running instance"));
        }
        else
        {
            actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch"));
            actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance."));
        }
        actionLaunchInstance.retranslate();
    }
    void setLaunchAction(bool kill)
    {
        m_kill = kill;
        updateLaunchAction();
    }

    void createMainToolbar(QMainWindow *MainWindow)
    {
        mainToolBar = TranslatedToolbar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        mainToolBar->setMovable(false);
        mainToolBar->setAllowedAreas(Qt::TopToolBarArea);
        mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mainToolBar->setFloatable(false);
        mainToolBar.setWindowTitleId(QT_TRANSLATE_NOOP("MainWindow", "Main Toolbar"));

        actionAddInstance = TranslatedAction(MainWindow);
        actionAddInstance->setObjectName(QStringLiteral("actionAddInstance"));
        actionAddInstance->setIcon(MSMC->getThemedIcon("new"));
        actionAddInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Add Instance"));
        actionAddInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Add a new instance."));
        all_actions.append(&actionAddInstance);
        mainToolBar->addAction(actionAddInstance);

        mainToolBar->addSeparator();

        foldersMenu = new QMenu(MainWindow);
        foldersMenu->setToolTipsVisible(true);

        actionViewInstanceFolder = TranslatedAction(MainWindow);
        actionViewInstanceFolder->setObjectName(QStringLiteral("actionViewInstanceFolder"));
        actionViewInstanceFolder->setIcon(MSMC->getThemedIcon("viewfolder"));
        actionViewInstanceFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Instance Folder"));
        actionViewInstanceFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance folder in a file browser."));
        all_actions.append(&actionViewInstanceFolder);
        foldersMenu->addAction(actionViewInstanceFolder);

        actionViewCentralModsFolder = TranslatedAction(MainWindow);
        actionViewCentralModsFolder->setObjectName(QStringLiteral("actionViewCentralModsFolder"));
        actionViewCentralModsFolder->setIcon(MSMC->getThemedIcon("centralmods"));
        actionViewCentralModsFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Central Mods Folder"));
        actionViewCentralModsFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the central mods folder in a file browser."));
        all_actions.append(&actionViewCentralModsFolder);
        foldersMenu->addAction(actionViewCentralModsFolder);

        foldersMenuButton = TranslatedToolButton(MainWindow);
        foldersMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Folders"));
        foldersMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open one of the folders shared between instances."));
        foldersMenuButton->setMenu(foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);
        foldersMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        foldersMenuButton->setIcon(MSMC->getThemedIcon("viewfolder"));
        foldersMenuButton->setFocusPolicy(Qt::NoFocus);
        all_toolbuttons.append(&foldersMenuButton);
        QWidgetAction* foldersButtonAction = new QWidgetAction(MainWindow);
        foldersButtonAction->setDefaultWidget(foldersMenuButton);
        mainToolBar->addAction(foldersButtonAction);

        actionSettings = TranslatedAction(MainWindow);
        actionSettings->setObjectName(QStringLiteral("actionSettings"));
        actionSettings->setIcon(MSMC->getThemedIcon("settings"));
        actionSettings->setMenuRole(QAction::PreferencesRole);
        actionSettings.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Settings"));
        actionSettings.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change settings."));
        all_actions.append(&actionSettings);
        mainToolBar->addAction(actionSettings);

        helpMenu = new QMenu(MainWindow);
        helpMenu->setToolTipsVisible(true);

        if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
            actionReportBug = TranslatedAction(MainWindow);
            actionReportBug->setObjectName(QStringLiteral("actionReportBug"));
            actionReportBug->setIcon(MSMC->getThemedIcon("bug"));
            actionReportBug.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Report a Bug"));
            actionReportBug.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the bug tracker to report a bug with MultiServerMC."));
            all_actions.append(&actionReportBug);
            helpMenu->addAction(actionReportBug);
        }

        if (!BuildConfig.DISCORD_URL.isEmpty()) {
            actionDISCORD = TranslatedAction(MainWindow);
            actionDISCORD->setObjectName(QStringLiteral("actionDISCORD"));
            actionDISCORD->setIcon(MSMC->getThemedIcon("discord"));
            actionDISCORD.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Discord"));
            actionDISCORD.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open MultiServerMC discord voice chat."));
            all_actions.append(&actionDISCORD);
            helpMenu->addAction(actionDISCORD);
        }

        if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
            actionREDDIT = TranslatedAction(MainWindow);
            actionREDDIT->setObjectName(QStringLiteral("actionREDDIT"));
            actionREDDIT->setIcon(MSMC->getThemedIcon("reddit-alien"));
            actionREDDIT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Reddit"));
            actionREDDIT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open MultiServerMC subreddit."));
            all_actions.append(&actionREDDIT);
            helpMenu->addAction(actionREDDIT);
        }

        actionAbout = TranslatedAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAbout->setIcon(MSMC->getThemedIcon("about"));
        actionAbout->setMenuRole(QAction::AboutRole);
        actionAbout.setTextId(QT_TRANSLATE_NOOP("MainWindow", "About MultiServerMC"));
        actionAbout.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View information about MultiServerMC."));
        all_actions.append(&actionAbout);
        helpMenu->addAction(actionAbout);

        helpMenuButton = TranslatedToolButton(MainWindow);
        helpMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Help"));
        helpMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Get help with MultiServerMC or Minecraft."));
        helpMenuButton->setMenu(helpMenu);
        helpMenuButton->setPopupMode(QToolButton::InstantPopup);
        helpMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        helpMenuButton->setIcon(MSMC->getThemedIcon("help"));
        helpMenuButton->setFocusPolicy(Qt::NoFocus);
        all_toolbuttons.append(&helpMenuButton);
        QWidgetAction* helpButtonAction = new QWidgetAction(MainWindow);
        helpButtonAction->setDefaultWidget(helpMenuButton);
        mainToolBar->addAction(helpButtonAction);

        if(BuildConfig.UPDATER_ENABLED)
        {
            actionCheckUpdate = TranslatedAction(MainWindow);
            actionCheckUpdate->setObjectName(QStringLiteral("actionCheckUpdate"));
            actionCheckUpdate->setIcon(MSMC->getThemedIcon("checkupdate"));
            actionCheckUpdate.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Update"));
            actionCheckUpdate.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Check for new updates for MultiServerMC."));
            all_actions.append(&actionCheckUpdate);
            mainToolBar->addAction(actionCheckUpdate);
        }

        all_toolbars.append(&mainToolBar);
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
    }

    void createStatusBar(QMainWindow *MainWindow)
    {
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);
    }

    void createInstanceToolbar(QMainWindow *MainWindow)
    {
        instanceToolBar = TranslatedToolbar(MainWindow);
        instanceToolBar->setObjectName(QStringLiteral("instanceToolBar"));
        // disabled until we have an instance selected
        instanceToolBar->setEnabled(false);
        instanceToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
        instanceToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
        instanceToolBar->setFloatable(false);
        instanceToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "Instance Toolbar"));

        // NOTE: not added to toolbar, but used for instance context menu (right click)
        actionChangeInstIcon = TranslatedAction(MainWindow);
        actionChangeInstIcon->setObjectName(QStringLiteral("actionChangeInstIcon"));
        actionChangeInstIcon->setIcon(QIcon(":/icons/instances/infinity"));
        actionChangeInstIcon->setIconVisibleInMenu(true);
        actionChangeInstIcon.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Change Icon"));
        actionChangeInstIcon.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's icon."));
        all_actions.append(&actionChangeInstIcon);

        changeIconButton = new LabeledToolButton(MainWindow);
        changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
        changeIconButton->setToolTip(actionChangeInstIcon->toolTip());
        changeIconButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        instanceToolBar->addWidget(changeIconButton);

        // NOTE: not added to toolbar, but used for instance context menu (right click)
        actionRenameInstance = TranslatedAction(MainWindow);
        actionRenameInstance->setObjectName(QStringLiteral("actionRenameInstance"));
        actionRenameInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Rename"));
        actionRenameInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Rename the selected instance."));
        all_actions.append(&actionRenameInstance);

        // the rename label is inside the rename tool button
        renameButton = new LabeledToolButton(MainWindow);
        renameButton->setObjectName(QStringLiteral("renameButton"));
        renameButton->setToolTip(actionRenameInstance->toolTip());
        renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        instanceToolBar->addWidget(renameButton);

        instanceToolBar->addSeparator();

        actionLaunchInstance = TranslatedAction(MainWindow);
        actionLaunchInstance->setObjectName(QStringLiteral("actionLaunchInstance"));
        all_actions.append(&actionLaunchInstance);
        instanceToolBar->addAction(actionLaunchInstance);

        instanceToolBar->addSeparator();

        actionEditInstance = TranslatedAction(MainWindow);
        actionEditInstance->setObjectName(QStringLiteral("actionEditInstance"));
        actionEditInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Edit Instance"));
        actionEditInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the instance settings, mods and versions."));
        all_actions.append(&actionEditInstance);
        instanceToolBar->addAction(actionEditInstance);

        actionEditInstNotes = TranslatedAction(MainWindow);
        actionEditInstNotes->setObjectName(QStringLiteral("actionEditInstNotes"));
        actionEditInstNotes.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Edit Notes"));
        actionEditInstNotes.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Edit the notes for the selected instance."));
        all_actions.append(&actionEditInstNotes);
        instanceToolBar->addAction(actionEditInstNotes);

        actionWorlds = TranslatedAction(MainWindow);
        actionWorlds->setObjectName(QStringLiteral("actionWorlds"));
        actionWorlds.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Worlds"));
        actionWorlds.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View the worlds of this instance."));
        all_actions.append(&actionWorlds);
        instanceToolBar->addAction(actionWorlds);

        actionChangeInstGroup = TranslatedAction(MainWindow);
        actionChangeInstGroup->setObjectName(QStringLiteral("actionChangeInstGroup"));
        actionChangeInstGroup.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Change Group"));
        actionChangeInstGroup.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's group."));
        all_actions.append(&actionChangeInstGroup);
        instanceToolBar->addAction(actionChangeInstGroup);

        instanceToolBar->addSeparator();

        actionViewSelectedMCFolder = TranslatedAction(MainWindow);
        actionViewSelectedMCFolder->setObjectName(QStringLiteral("actionViewSelectedMCFolder"));
        actionViewSelectedMCFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Minecraft Folder"));
        actionViewSelectedMCFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's minecraft folder in a file browser."));
        all_actions.append(&actionViewSelectedMCFolder);
        instanceToolBar->addAction(actionViewSelectedMCFolder);

        actionConfig_Folder = TranslatedAction(MainWindow);
        actionConfig_Folder->setObjectName(QStringLiteral("actionConfig_Folder"));
        actionConfig_Folder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Config Folder"));
        actionConfig_Folder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance's config folder."));
        all_actions.append(&actionConfig_Folder);
        instanceToolBar->addAction(actionConfig_Folder);

        actionViewSelectedInstFolder = TranslatedAction(MainWindow);
        actionViewSelectedInstFolder->setObjectName(QStringLiteral("actionViewSelectedInstFolder"));
        actionViewSelectedInstFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Instance Folder"));
        actionViewSelectedInstFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's root folder in a file browser."));
        all_actions.append(&actionViewSelectedInstFolder);
        instanceToolBar->addAction(actionViewSelectedInstFolder);

        instanceToolBar->addSeparator();

        actionExportInstance = TranslatedAction(MainWindow);
        actionExportInstance->setObjectName(QStringLiteral("actionExportInstance"));
        actionExportInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Export Instance"));
        // FIXME: missing tooltip
        all_actions.append(&actionExportInstance);
        instanceToolBar->addAction(actionExportInstance);

        actionDeleteInstance = TranslatedAction(MainWindow);
        actionDeleteInstance->setObjectName(QStringLiteral("actionDeleteInstance"));
        actionDeleteInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Delete"));
        actionDeleteInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Delete the selected instance."));
        all_actions.append(&actionDeleteInstance);
        instanceToolBar->addAction(actionDeleteInstance);

        actionCopyInstance = TranslatedAction(MainWindow);
        actionCopyInstance->setObjectName(QStringLiteral("actionCopyInstance"));
        actionCopyInstance->setIcon(MSMC->getThemedIcon("copy"));
        actionCopyInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Copy Instance"));
        actionCopyInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Copy the selected instance."));
        all_actions.append(&actionCopyInstance);
        instanceToolBar->addAction(actionCopyInstance);

        all_toolbars.append(&instanceToolBar);
        MainWindow->addToolBar(Qt::RightToolBarArea, instanceToolBar);
    }

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
        {
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        }
        MainWindow->resize(800, 600);
        MainWindow->setWindowIcon(MSMC->getThemedIcon("logo"));
        MainWindow->setWindowTitle("MultiServerMC 5");
#ifndef QT_NO_ACCESSIBILITY
        MainWindow->setAccessibleName("MultiServerMC");
#endif

        createMainToolbar(MainWindow);

        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        MainWindow->setCentralWidget(centralWidget);

        createStatusBar(MainWindow);
        createInstanceToolbar(MainWindow);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        QString winTitle = tr("MultiServerMC 5 - Version %1").arg(BuildConfig.printableVersionString());
        if (!BuildConfig.BUILD_PLATFORM.isEmpty())
        {
            winTitle += tr(" on %1", "on platform, as in operating system").arg(BuildConfig.BUILD_PLATFORM);
        }
        MainWindow->setWindowTitle(winTitle);
        // all the actions
        for(auto * item: all_actions)
        {
            item->retranslate();
        }
        for(auto * item: all_toolbars)
        {
            item->retranslate();
        }
        for(auto * item: all_toolbuttons)
        {
            item->retranslate();
        }
        // submenu buttons
        foldersMenuButton->setText(tr("Folders"));
        helpMenuButton->setText(tr("Help"));
    } // retranslateUi
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new MainWindow::Ui)
{
    ui->setupUi(this);

    // OSX magic.
    setUnifiedTitleAndToolBarOnMac(true);

    // Global shortcuts
    {
        // FIXME: This is kinda weird. and bad. We need some kind of managed shutdown.
        auto q = new QShortcut(QKeySequence::Quit, this);
        connect(q, SIGNAL(activated()), qApp, SLOT(quit()));
    }

    // Konami Code
    {
        secretEventFilter = new KonamiCode(this);
        connect(secretEventFilter, &KonamiCode::triggered, this, &MainWindow::konamiTriggered);
    }

    // Create the instance list widget
    {
        view = new GroupView(ui->centralWidget);

        view->setSelectionMode(QAbstractItemView::SingleSelection);
        // FIXME: leaks ListViewDelegate
        view->setItemDelegate(new ListViewDelegate(this));
        view->setFrameShape(QFrame::NoFrame);
        // do not show ugly blue border on the mac
        view->setAttribute(Qt::WA_MacShowFocusRect, false);

        view->installEventFilter(this);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::showInstanceContextMenu);
        connect(view, &GroupView::droppedURLs, this, &MainWindow::droppedURLs, Qt::QueuedConnection);

        proxymodel = new InstanceProxyModel(this);
        proxymodel->setSourceModel(MSMC->instances().get());
        proxymodel->sort(0);
        connect(proxymodel, &InstanceProxyModel::dataChanged, this, &MainWindow::instanceDataChanged);

        view->setModel(proxymodel);
        view->setSourceOfGroupCollapseStatus([](const QString & groupName)->bool {
            return MSMC->instances()->isGroupCollapsed(groupName);
        });
        connect(view, &GroupView::groupStateChanged, MSMC->instances().get(), &InstanceList::on_GroupStateChanged);
        ui->horizontalLayout->addWidget(view);
    }

    // start instance when double-clicked
    connect(view, &GroupView::activated, this, &MainWindow::instanceActivated);

    // track the selection -- update the instance toolbar
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::instanceChanged);

    // track icon changes and update the toolbar!
    connect(MSMC->icons().get(), &IconList::iconUpdated, this, &MainWindow::iconUpdated);

    // model reset -> selection is invalid. All the instance pointers are wrong.
    connect(MSMC->instances().get(), &InstanceList::dataIsInvalid, this, &MainWindow::selectionBad);

    // handle newly added instances
    connect(MSMC->instances().get(), &InstanceList::instanceSelectRequest, this, &MainWindow::instanceSelectRequest);

    // When the global settings page closes, we want to know about it and update our state
    connect(MSMC, &MultiServerMC::globalSettingsClosed, this, &MainWindow::globalSettingsClosed);

    m_statusLeft = new QLabel(tr("No instance selected"), this);
    statusBar()->addPermanentWidget(m_statusLeft, 1);

    if(BuildConfig.UPDATER_ENABLED)
    {
        bool updatesAllowed = MSMC->updatesAreAllowed();
        updatesAllowedChanged(updatesAllowed);

        // NOTE: calling the operator like that is an ugly hack to appease ancient gcc...
        connect(ui->actionCheckUpdate.operator->(), &QAction::triggered, this, &MainWindow::checkForUpdates);

        // set up the updater object.
        auto updater = MSMC->updateChecker();
        connect(updater.get(), &UpdateChecker::updateAvailable, this, &MainWindow::updateAvailable);
        connect(updater.get(), &UpdateChecker::noUpdateFound, this, &MainWindow::updateNotAvailable);
        // if automatic update checks are allowed, start one.
        if (MSMC->settings()->get("AutoUpdate").toBool() && updatesAllowed)
        {
            updater->checkForUpdate(MSMC->settings()->get("UpdateChannel").toString(), false);
        }
    }

    setSelectedInstanceById(MSMC->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    retranslateUi();
}

void MainWindow::retranslateUi()
{
    if (m_selectedInstance) {
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
    } else {
        m_statusLeft->setText(tr("No instance selected"));
    }

    ui->retranslateUi(this);
}

MainWindow::~MainWindow()
{
}

QMenu * MainWindow::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction( ui->mainToolBar->toggleViewAction() );
    return filteredMenu;
}

void MainWindow::konamiTriggered()
{
    // ENV.enableFeature("NewModsPage");
    qDebug() << "Super Secret Mode ACTIVATED!";
}

void MainWindow::showInstanceContextMenu(const QPoint &pos)
{
    QList<QAction *> actions;

    QAction *actionSep = new QAction("", this);
    actionSep->setSeparator(true);

    bool onInstance = view->indexAt(pos).isValid();
    if (onInstance)
    {
        actions = ui->instanceToolBar->actions();

        // replace the change icon widget with an actual action
        actions.replace(0, ui->actionChangeInstIcon);

        // replace the rename widget with an actual action
        actions.replace(1, ui->actionRenameInstance);

        // add header
        actions.prepend(actionSep);
        QAction *actionVoid = new QAction(m_selectedInstance->name(), this);
        actionVoid->setEnabled(false);
        actions.prepend(actionVoid);
    }
    else
    {
        auto group = view->groupNameAt(pos);

        QAction *actionVoid = new QAction("MultiServerMC", this);
        actionVoid->setEnabled(false);

        QAction *actionCreateInstance = new QAction(tr("Create instance"), this);
        actionCreateInstance->setToolTip(ui->actionAddInstance->toolTip());
        if(!group.isNull())
        {
            QVariantMap data;
            data["group"] = group;
            actionCreateInstance->setData(data);
        }

        connect(actionCreateInstance, SIGNAL(triggered(bool)), SLOT(on_actionAddInstance_triggered()));

        actions.prepend(actionSep);
        actions.prepend(actionVoid);
        actions.append(actionCreateInstance);
        if(!group.isNull())
        {
            QAction *actionDeleteGroup = new QAction(tr("Delete group '%1'").arg(group), this);
            QVariantMap data;
            data["group"] = group;
            actionDeleteGroup->setData(data);
            connect(actionDeleteGroup, SIGNAL(triggered(bool)), SLOT(deleteGroup()));
            actions.append(actionDeleteGroup);
        }
    }
    QMenu myMenu;
    myMenu.addActions(actions);
    /*
    if (onInstance)
        myMenu.setEnabled(m_selectedInstance->canLaunch());
    */
    myMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::updateToolsMenu()
{
    QToolButton *launchButton = dynamic_cast<QToolButton*>(ui->instanceToolBar->widgetForAction(ui->actionLaunchInstance));

    if(!m_selectedInstance || m_selectedInstance->isRunning())
    {
        ui->actionLaunchInstance->setMenu(nullptr);
        launchButton->setPopupMode(QToolButton::InstantPopup);
        return;
    }

    QMenu *launchMenu = ui->actionLaunchInstance->menu();
    launchButton->setPopupMode(QToolButton::MenuButtonPopup);
    if (launchMenu)
    {
        launchMenu->clear();
    }
    else
    {
        launchMenu = new QMenu(this);
    }

    QAction *normalLaunch = launchMenu->addAction(tr("Launch"));
    connect(normalLaunch, &QAction::triggered, [this]()
            {
                MSMC->launch(m_selectedInstance, true);
            });
    QString profilersTitle = tr("Profilers");
    launchMenu->addSeparator()->setText(profilersTitle);
    for (auto profiler : MSMC->profilers().values())
    {
        QAction *profilerAction = launchMenu->addAction(profiler->name());
        QString error;
        if (!profiler->check(&error))
        {
            profilerAction->setDisabled(true);
            QString profilerToolTip = tr("Profiler not setup correctly. Go into settings, \"External Tools\".");
            profilerAction->setToolTip(profilerToolTip);
        }
        else
        {
            connect(profilerAction, &QAction::triggered, [this, profiler]()
                    {
                        MSMC->launch(m_selectedInstance, true, profiler.get());
                    });
        }
    }
    ui->actionLaunchInstance->setMenu(launchMenu);
}

QString profileInUseFilter(const QString & profile, bool used)
{
    if(used)
    {
        return profile + QObject::tr(" (in use)");
    }
    else
    {
        return profile;
    }
}



void MainWindow::updatesAllowedChanged(bool allowed)
{
    if(!BuildConfig.UPDATER_ENABLED)
    {
        return;
    }
    ui->actionCheckUpdate->setEnabled(allowed);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == view)
    {
        if (ev->type() == QEvent::KeyPress)
        {
            secretEventFilter->input(ev);
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
            switch (keyEvent->key())
            {
                /*
            case Qt::Key_Enter:
            case Qt::Key_Return:
                activateInstance(m_selectedInstance);
                return true;
                */
            case Qt::Key_Delete:
                on_actionDeleteInstance_triggered();
                return true;
            case Qt::Key_F5:
                refreshInstances();
                return true;
            case Qt::Key_F2:
                on_actionRenameInstance_triggered();
                return true;
            default:
                break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::updateAvailable(GoUpdate::Status status)
{
    if(!MSMC->updatesAreAllowed())
    {
        updateNotAvailable();
        return;
    }
    UpdateDialog dlg(true, this);
    UpdateAction action = (UpdateAction)dlg.exec();
    switch (action)
    {
    case UPDATE_LATER:
        qDebug() << "Update will be installed later.";
        break;
    case UPDATE_NOW:
        downloadUpdates(status);
        break;
    }
}

void MainWindow::updateNotAvailable()
{
    UpdateDialog dlg(false, this);
    dlg.exec();
}

QList<int> stringToIntList(const QString &string)
{
    QStringList split = string.split(',', QString::SkipEmptyParts);
    QList<int> out;
    for (int i = 0; i < split.size(); ++i)
    {
        out.append(split.at(i).toInt());
    }
    return out;
}
QString intListToString(const QList<int> &list)
{
    QStringList slist;
    for (int i = 0; i < list.size(); ++i)
    {
        slist.append(QString::number(list.at(i)));
    }
    return slist.join(',');
}

void MainWindow::downloadUpdates(GoUpdate::Status status)
{
    if(!MSMC->updatesAreAllowed())
    {
        return;
    }
    qDebug() << "Downloading updates.";
    ProgressDialog updateDlg(this);
    status.rootPath = MSMC->root();

    auto dlPath = FS::PathCombine(MSMC->root(), "update", "XXXXXX");
    if (!FS::ensureFilePathExists(dlPath))
    {
        CustomMessageBox::selectable(this, tr("Error"), tr("Couldn't create folder for update downloads:\n%1").arg(dlPath), QMessageBox::Warning)->show();
    }
    GoUpdate::DownloadTask updateTask(status, dlPath, &updateDlg);
    // If the task succeeds, install the updates.
    if (updateDlg.execWithTask(&updateTask))
    {
        /**
         * NOTE: This disables launching instances until the update either succeeds (and this process exits)
         * or the update fails (and the control leaves this scope).
         */
        MSMC->updateIsRunning(true);
        UpdateController update(this, MSMC->root(), updateTask.updateFilesDir(), updateTask.operations());
        update.installUpdates();
        MSMC->updateIsRunning(false);
    }
    else
    {
        CustomMessageBox::selectable(this, tr("Error"), updateTask.failReason(), QMessageBox::Warning)->show();
    }
}

namespace {
template <typename T>
T non_stupid_abs(T in)
{
    if (in < 0)
        return -in;
    return in;
}
}

void MainWindow::runModalTask(Task *task)
{
    connect(task, &Task::failed, [this](QString reason)
        {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
        });
    connect(task, &Task::succeeded, [this, task]()
        {
            QStringList warnings = task->warnings();
            if(warnings.count())
            {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
        });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);
}

void MainWindow::instanceFromInstanceTask(InstanceTask *rawTask)
{
    unique_qobject_ptr<Task> task(MSMC->instances()->wrapInstanceTask(rawTask));
    runModalTask(task.get());
}

void MainWindow::on_actionCopyInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
    if (!copyInstDlg.exec())
        return;

    auto copyTask = new InstanceCopyTask(m_selectedInstance, copyInstDlg.shouldCopySaves(), copyInstDlg.shouldKeepPlaytime());
    copyTask->setName(copyInstDlg.instName());
    copyTask->setGroup(copyInstDlg.instGroup());
    copyTask->setIcon(copyInstDlg.iconKey());
    unique_qobject_ptr<Task> task(MSMC->instances()->wrapInstanceTask(copyTask));
    runModalTask(task.get());
}

void MainWindow::finalizeInstance(InstancePtr inst)
{
    view->updateGeometries();
    setSelectedInstanceById(inst->id());
    if (true)
    {
        ProgressDialog loadDialog(this);
        auto update = inst->createUpdateTask(Net::Mode::Online);
        connect(update.get(), &Task::failed, [this](QString reason)
                {
                    QString error = QString("Instance load failed: %1").arg(reason);
                    CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)->show();
                });
        if(update)
        {
            loadDialog.setSkipButton(true, tr("Abort"));
            loadDialog.execWithTask(update.get());
        }
    }
    else
    {
        CustomMessageBox::selectable(this, tr("Error"), tr("MultiServerMC cannot download Minecraft or update instances unless you have at least "
                                                           "one account added.\nPlease add your Mojang or Minecraft account."),
                                     QMessageBox::Warning)
            ->show();
    }
}

void MainWindow::addInstance(QString url)
{
    QString groupName;
    do
    {
        QObject* obj = sender();
        if(!obj)
            break;
        QAction *action = qobject_cast<QAction *>(obj);
        if(!action)
            break;
        auto map = action->data().toMap();
        if(!map.contains("group"))
            break;
        groupName = map["group"].toString();
    } while(0);

    if(groupName.isEmpty())
    {
        groupName = MSMC->settings()->get("LastUsedGroupForNewInstance").toString();
    }

    NewInstanceDialog newInstDlg(groupName, url, this);
    if (!newInstDlg.exec())
        return;

    MSMC->settings()->set("LastUsedGroupForNewInstance", newInstDlg.instGroup());

    InstanceTask * creationTask = newInstDlg.extractTask();
    if(creationTask)
    {
        instanceFromInstanceTask(creationTask);
    }
}

void MainWindow::on_actionAddInstance_triggered()
{
    addInstance();
}

void MainWindow::droppedURLs(QList<QUrl> urls)
{
    for(auto & url:urls)
    {
        if(url.isLocalFile())
        {
            addInstance(url.toLocalFile());
        }
        else
        {
            addInstance(url.toString());
        }
        // Only process one dropped file...
        break;
    }
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
    if (!m_selectedInstance)
        return;

    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_selectedInstance->iconKey());
    if (dlg.result() == QDialog::Accepted)
    {
        m_selectedInstance->setIconKey(dlg.selectedIconKey);
        auto icon = MSMC->icons()->getIcon(dlg.selectedIconKey);
        ui->actionChangeInstIcon->setIcon(icon);
        ui->changeIconButton->setIcon(icon);
    }
}

void MainWindow::iconUpdated(QString icon)
{
    if (icon == m_currentInstIcon)
    {
        auto icon = MSMC->icons()->getIcon(m_currentInstIcon);
        ui->actionChangeInstIcon->setIcon(icon);
        ui->changeIconButton->setIcon(icon);
    }
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
    m_currentInstIcon = new_icon;
    auto icon = MSMC->icons()->getIcon(m_currentInstIcon);
    ui->actionChangeInstIcon->setIcon(icon);
    ui->changeIconButton->setIcon(icon);
}

void MainWindow::setSelectedInstanceById(const QString &id)
{
    if (id.isNull())
        return;
    const QModelIndex index = MSMC->instances()->getInstanceIndexById(id);
    if (index.isValid())
    {
        QModelIndex selectionIndex = proxymodel->mapFromSource(index);
        view->selectionModel()->setCurrentIndex(selectionIndex, QItemSelectionModel::ClearAndSelect);
    }
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
    if (!m_selectedInstance)
        return;

    bool ok = false;
    InstanceId instId = m_selectedInstance->id();
    QString name(MSMC->instances()->getInstanceGroup(instId));
    auto groups = MSMC->instances()->getGroups();
    groups.insert(0, "");
    groups.sort(Qt::CaseInsensitive);
    int foo = groups.indexOf(name);

    name = QInputDialog::getItem(this, tr("Group name"), tr("Enter a new group name."), groups, foo, true, &ok);
    name = name.simplified();
    if (ok)
    {
        MSMC->instances()->setInstanceGroup(instId, name);
    }
}

void MainWindow::deleteGroup()
{
    QObject* obj = sender();
    if(!obj)
        return;
    QAction *action = qobject_cast<QAction *>(obj);
    if(!action)
        return;
    auto map = action->data().toMap();
    if(!map.contains("group"))
        return;
    QString groupName = map["group"].toString();
    if(!groupName.isEmpty())
    {
        auto reply = QMessageBox::question(this, tr("Delete group"), tr("Are you sure you want to delete the group %1")
            .arg(groupName), QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            MSMC->instances()->deleteGroup(groupName);
        }
    }
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
    QString str = MSMC->settings()->get("InstanceDir").toString();
    DesktopServices::openDirectory(str);
}

void MainWindow::refreshInstances()
{
    MSMC->instances()->loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
    DesktopServices::openDirectory(MSMC->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionConfig_Folder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->instanceConfigFolder();
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}

void MainWindow::checkForUpdates()
{
    if(BuildConfig.UPDATER_ENABLED)
    {
        auto updater = MSMC->updateChecker();
        updater->checkForUpdate(MSMC->settings()->get("UpdateChannel").toString(), true);
    }
    else
    {
        qWarning() << "Updater not set up. Cannot check for updates.";
    }
}

void MainWindow::on_actionSettings_triggered()
{
    MSMC->ShowGlobalSettings(this, "global-settings");
}

void MainWindow::globalSettingsClosed()
{
    // FIXME: quick HACK to make this work. improve, optimize.
    MSMC->instances()->loadList();
    proxymodel->invalidate();
    proxymodel->sort(0);
    updateToolsMenu();
    update();
}

void MainWindow::on_actionEditInstNotes_triggered()
{
    MSMC->showInstanceWindow(m_selectedInstance, "notes");
}

void MainWindow::on_actionWorlds_triggered()
{
    MSMC->showInstanceWindow(m_selectedInstance, "worlds");
}

void MainWindow::on_actionEditInstance_triggered()
{
    MSMC->showInstanceWindow(m_selectedInstance);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionDeleteInstance_triggered()
{
    if (!m_selectedInstance)
    {
        return;
    }
    auto id = m_selectedInstance->id();
    auto response = CustomMessageBox::selectable(
        this,
        tr("CAREFUL!"),
        tr("About to delete: %1\nThis is permanent and will completely delete the instance.\n\nAre you sure?").arg(m_selectedInstance->name()),
        QMessageBox::Warning,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    )->exec();
    if (response == QMessageBox::Yes)
    {
        MSMC->instances()->deleteInstance(id);
    }
}

void MainWindow::on_actionExportInstance_triggered()
{
    if (m_selectedInstance)
    {
        ExportInstanceDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionRenameInstance_triggered()
{
    if (m_selectedInstance)
    {
        view->edit(view->currentIndex());
    }
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->instanceRoot();
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}

void MainWindow::on_actionViewSelectedMCFolder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->gameRoot();
        if (!FS::ensureFilePathExists(str))
        {
            // TODO: report error
            return;
        }
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save the window state and geometry.
    MSMC->settings()->set("MainWindowState", saveState().toBase64());
    MSMC->settings()->set("MainWindowGeometry", saveGeometry().toBase64());
    event->accept();
    emit isClosing();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::instanceActivated(QModelIndex index)
{
    if (!index.isValid())
        return;
    QString id = index.data(InstanceList::InstanceIDRole).toString();
    InstancePtr inst = MSMC->instances()->getInstanceById(id);
    if (!inst)
        return;

    activateInstance(inst);
}

void MainWindow::on_actionLaunchInstance_triggered()
{
    if (!m_selectedInstance)
    {
        return;
    }
    if(m_selectedInstance->isRunning())
    {
        MSMC->kill(m_selectedInstance);
    }
    else
    {
        MSMC->launch(m_selectedInstance);
    }
}

void MainWindow::activateInstance(InstancePtr instance)
{
    MSMC->launch(instance);
}

void MainWindow::taskEnd()
{
    QObject *sender = QObject::sender();
    if (sender == m_versionLoadTask)
        m_versionLoadTask = NULL;

    sender->deleteLater();
}

void MainWindow::startTask(Task *task)
{
    connect(task, SIGNAL(succeeded()), SLOT(taskEnd()));
    connect(task, SIGNAL(failed(QString)), SLOT(taskEnd()));
    task->start();
}

void MainWindow::instanceChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (!current.isValid())
    {
        MSMC->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
    QString id = current.data(InstanceList::InstanceIDRole).toString();
    m_selectedInstance = MSMC->instances()->getInstanceById(id);
    if (m_selectedInstance)
    {
        ui->instanceToolBar->setEnabled(true);
        if(m_selectedInstance->isRunning())
        {
            ui->actionLaunchInstance->setEnabled(true);
            ui->setLaunchAction(true);
        }
        else
        {
            ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());
            ui->setLaunchAction(false);
        }
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        ui->renameButton->setText(m_selectedInstance->name());
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateToolsMenu();

        MSMC->settings()->set("SelectedInstance", m_selectedInstance->id());
    }
    else
    {
        ui->instanceToolBar->setEnabled(false);
        MSMC->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
}

void MainWindow::instanceSelectRequest(QString id)
{
    setSelectedInstanceById(id);
}

void MainWindow::instanceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    auto current = view->selectionModel()->currentIndex();
    QItemSelection test(topLeft, bottomRight);
    if (test.contains(current))
    {
        instanceChanged(current, current);
    }
}

void MainWindow::selectionBad()
{
    // start by reseting everything...
    m_selectedInstance = nullptr;

    statusBar()->clearMessage();
    ui->instanceToolBar->setEnabled(false);
    ui->renameButton->setText(tr("Rename Instance"));
    updateInstanceToolIcon("infinity");

    // ...and then see if we can enable the previously selected instance
    setSelectedInstanceById(MSMC->settings()->get("SelectedInstance").toString());
}

void MainWindow::checkInstancePathForProblems()
{
    QString instanceFolder = MSMC->settings()->get("InstanceDir").toString();
    if (FS::checkProblemticPathJava(QDir(instanceFolder)))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'!\' and this is known to cause Java problems!"));
        warning.setInformativeText(tr("You have now two options: <br/>"
                                      " - change the instance folder in the settings <br/>"
                                      " - move this installation of MultiServerMC5 to a different folder"));
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    auto tempFolderText = tr("This is a problem: <br/>"
                             " - MultiServerMC will likely be deleted without warning by the operating system <br/>"
                             " - close MultiServerMC now and extract it to a real location, not a temporary folder");
    QString pathfoldername = QDir(instanceFolder).absolutePath();
    if (pathfoldername.contains("Rar$", Qt::CaseInsensitive))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'Rar$\' - that means you haven't extracted the MultiServerMC zip!"));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    else if (pathfoldername.startsWith(QDir::tempPath()) || pathfoldername.contains("/TempState/"))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder is in a temporary folder: \'%1\'!").arg(QDir::tempPath()));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
}

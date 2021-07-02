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

#include "MinecraftPage.h"
#include "ui_MinecraftPage.h"

#include <QMessageBox>
#include <QDir>
#include <QTabBar>

#include "settings/SettingsObject.h"
#include "MultiServerMC.h"

MinecraftPage::MinecraftPage(QWidget *parent) : QWidget(parent), ui(new Ui::MinecraftPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    loadSettings();
}

MinecraftPage::~MinecraftPage()
{
    delete ui;
}

bool MinecraftPage::apply()
{
    applySettings();
    return true;
}

void MinecraftPage::on_maximizedCheckBox_clicked(bool checked)
{
    Q_UNUSED(checked);
}


void MinecraftPage::applySettings()
{
    auto s = MSMC->settings();

    // Game time
    s->set("ShowGameTime", ui->showGameTime->isChecked());
    s->set("RecordGameTime", ui->recordGameTime->isChecked());
}

void MinecraftPage::loadSettings()
{
    auto s = MSMC->settings();

    ui->showGameTime->setChecked(s->get("ShowGameTime").toBool());
    ui->recordGameTime->setChecked(s->get("RecordGameTime").toBool());
}

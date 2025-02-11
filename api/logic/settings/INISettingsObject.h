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

#include <QObject>

#include "settings/INIFile.h"

#include "settings/SettingsObject.h"

#include "multiservermc_logic_export.h"

/*!
 * \brief A settings object that stores its settings in an INIFile.
 */
class MULTISERVERMC_LOGIC_EXPORT INISettingsObject : public SettingsObject
{
    Q_OBJECT
public:
    explicit INISettingsObject(const QString &path, QObject *parent = 0);

    /*!
     * \brief Gets the path to the INI file.
     * \return The path to the INI file.
     */
    virtual QString filePath() const
    {
        return m_filePath;
    }

    /*!
     * \brief Sets the path to the INI file and reloads it.
     * \param filePath The INI file's new path.
     */
    virtual void setFilePath(const QString &filePath);

    bool reload() override;

    void suspendSave() override;
    void resumeSave() override;

protected slots:
    virtual void changeSetting(const Setting &setting, QVariant value) override;
    virtual void resetSetting(const Setting &setting) override;

protected:
    virtual QVariant retrieveValue(const Setting &setting) override;
    void doSave();

protected:
    INIFile m_ini;
    QString m_filePath;
};

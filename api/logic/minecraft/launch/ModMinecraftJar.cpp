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

#include "ModMinecraftJar.h"
#include "launch/LaunchTask.h"
#include "MSMCZip.h"
#include "minecraft/OpSys.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

void ModMinecraftJar::executeTask()
{
    auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());

    if(!m_inst->getJarMods().size())
    {
        emitSucceeded();
        return;
    }
    // nuke obsolete stripped jar(s) if needed
    if(!FS::ensureFolderPathExists(m_inst->binRoot()))
    {
        emitFailed(tr("Couldn't create the bin folder for Minecraft.jar"));
    }

    auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
    if(!removeJar())
    {
        emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
    }

    // create temporary modded jar, if needed
    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto jarMods = m_inst->getJarMods();
    if(jarMods.size())
    {
        auto mainJar = profile->getMainJar();
        QStringList jars, temp1, temp2, temp3, temp4;
        mainJar->getApplicableFiles(currentSystem, jars, temp1, temp2, temp3, m_inst->getLocalLibraryPath());
        auto sourceJarPath = jars[0];
        if(!MSMCZip::createModdedJar(sourceJarPath, finalJarPath, jarMods))
        {
            emitFailed(tr("Failed to create the custom Minecraft jar file."));
            return;
        }
    }
    emitSucceeded();
}

void ModMinecraftJar::finalize()
{
    removeJar();
}

bool ModMinecraftJar::removeJar()
{
    auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());
    auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
    QFile finalJar(finalJarPath);
    if(finalJar.exists())
    {
        if(!finalJar.remove())
        {
            return false;
        }
    }
    return true;
}

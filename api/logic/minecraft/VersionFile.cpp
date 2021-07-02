#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/Library.h"
#include "minecraft/PackProfile.h"
#include "ParseUtils.h"

#include <Version.h>

static bool isMinecraftVersion(const QString &uid)
{
    return uid == "net.minecraft";
}

void VersionFile::applyTo(LaunchProfile *profile)
{
    // Only real Minecraft can set those. Don't let anything override them.
    if (isMinecraftVersion(uid))
    {
        profile->applyMinecraftVersion(minecraftVersion);
        profile->applyMinecraftVersionType(type);
    }

    profile->applyMainJar(mainJar);
    profile->applyMainClass(mainClass);
    profile->applyAppletClass(appletClass);
    profile->applyMinecraftArguments(minecraftArguments);
    profile->applyTweakers(addTweakers);
    profile->applyJarMods(jarMods);
    profile->applyMods(mods);
    profile->applyTraits(traits);

    for (auto library : libraries)
    {
        profile->applyLibrary(library);
    }
    for (auto mavenFile : mavenFiles)
    {
        profile->applyMavenFile(mavenFile);
    }
    profile->applyProblemSeverity(getProblemSeverity());
}

/*
    auto theirVersion = profile->getMinecraftVersion();
    if (!theirVersion.isNull() && !dependsOnMinecraftVersion.isNull())
    {
        if (QRegExp(dependsOnMinecraftVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(theirVersion) == -1)
        {
            throw MinecraftVersionMismatch(uid, dependsOnMinecraftVersion, theirVersion);
        }
    }
*/

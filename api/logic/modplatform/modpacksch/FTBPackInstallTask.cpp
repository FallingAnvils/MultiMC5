#include "FTBPackInstallTask.h"

#include "BuildConfig.h"
#include "Env.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version)
{
    m_pack = pack;
    m_version_name = version;
}

bool PackInstallTask::abort()
{
    return true;
}

void PackInstallTask::executeTask()
{
    // Find pack version
    bool found = false;
    VersionInfo version;

    for(auto vInfo : m_pack.versions) {
        if (vInfo.name == m_version_name) {
            found = true;
            version = vInfo;
            break;
        }
    }

    if(!found) {
        emitFailed(tr("Failed to find pack version %1").arg(m_version_name));
        return;
    }

    auto *netJob = new NetJob("ModpacksCH::VersionFetch");
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1/%2")
            .arg(m_pack.id).arg(version.id);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ModpacksCH::Version version;
    try
    {
        ModpacksCH::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    m_version = version;

    downloadPack();
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::downloadPack()
{
    setStatus(tr("Downloading mods..."));

    jobPtr.reset(new NetJob(tr("Mod download")));
    for(auto file : m_version.files) {
        if(file.serverOnly) continue;

        QFileInfo fileName(file.name);
        auto cacheName = fileName.completeBaseName() + "-" + file.sha1 + "." + fileName.suffix();

        auto entry = ENV.metacache()->resolveEntry("ModpacksCHPacks", cacheName);
        entry->setStale(true);

        auto relpath = FS::PathCombine("minecraft", file.path, file.name);
        auto path = FS::PathCombine(m_stagingPath, relpath);

        qDebug() << "Will download" << file.url << "to" << path;
        filesToCopy[entry->getFullPath()] = path;

        auto dl = Net::Download::makeCached(file.url, entry);
        if (!file.sha1.isEmpty()) {
            auto rawSha1 = QByteArray::fromHex(file.sha1.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
        }
        jobPtr->addNetAction(dl);
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        install();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });

    jobPtr->start();
}

void PackInstallTask::install()
{
    setStatus(tr("Copying modpack files"));

    for (auto iter = filesToCopy.begin(); iter != filesToCopy.end(); iter++) {
        auto &from = iter.key();
        auto &to = iter.value();
        FS::copy fileCopyOperation(from, to);
        if(!fileCopyOperation()) {
            qWarning() << "Failed to copy" << from << "to" << to;
            emitFailed(tr("Failed to copy files"));
            return;
        }
    }

    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    for(auto target : m_version.targets) {
        if(target.type == "game" && target.name == "minecraft") {
            components->setComponentVersion("net.minecraft", target.version, true);
            break;
        }
    }

    for(auto target : m_version.targets) {
        if(target.type != "modloader") continue;

        if(target.name == "forge") {
            components->setComponentVersion("net.minecraftforge", target.version, true);
        }
        else if(target.name == "fabric") {
            components->setComponentVersion("net.fabricmc.fabric-loader", target.version, true);
        }
    }

    // install any jar mods
    QDir jarModsDir(FS::PathCombine(m_stagingPath, "minecraft", "jarmods"));
    if (jarModsDir.exists()) {
        QStringList jarMods;

        for (const auto& info : jarModsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            jarMods.push_back(info.absoluteFilePath());
        }

        components->installJarMods(jarMods);
    }

    components->saveNow();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    emitSucceeded();
}

}

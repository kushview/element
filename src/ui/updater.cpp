// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/version.h>
#include <element/ui/updater.hpp>
#include <element/juce/core.hpp>
#include <element/juce/events.hpp>
#include <element/datapath.hpp>
#include <element/version.hpp>

#ifndef EL_TRACE_UPDATER
#define EL_TRACE_UPDATER 0
#endif

#define EL_PACKAGE_ID "net.kushview.element"

using namespace juce;

namespace element {
namespace ui {

//==============================================================================
struct UpdateRepo
{
    std::string host;
    std::string username;
    std::string password;
    bool enabled { false };
};

static File networkFile() noexcept
{
    return DataPath::applicationDataDir().getChildFile ("installer/network.xml");
}

static std::unique_ptr<XmlElement> readNetworkFile()
{
    auto file = networkFile();
    if (! file.existsAsFile())
        return nullptr;
    return XmlDocument::parse (file);
}

#if 0
static void saveRepos (const std::vector<UpdateRepo>& _repos)
{
    auto xml = readNetworkFile();
    if (xml == nullptr)
        return;

    if (auto repos = xml->getChildByName ("Repositories"))
    {
        for (auto e : repos->getChildIterator())
            repos->removeChildElement (e, true);

        for (const auto& repo : _repos)
        {
            auto xml = repos->createNewChildElement ("Repository");
            if (auto c = xml->createNewChildElement ("Host"))
                c->addTextElement (repo.host);
            if (auto c = xml->createNewChildElement ("Username"))
                c->addTextElement (repo.username);
            if (auto c = xml->createNewChildElement ("Password"))
                c->addTextElement (repo.password);
            if (auto c = xml->createNewChildElement ("Enabled"))
                c->addTextElement (repo.enabled ? "1" : "0");
        }
    }

    XmlElement::TextFormat format;
    format.addDefaultHeader = true;
    format.customEncoding = "UTF-8";
    if (! xml->writeTo (networkFile(), format))
    {
        const auto fn = networkFile().getFileName().toStdString();
        std::clog << "[element] failed to write " << fn << std::endl;
    }
}
#endif

static std::vector<UpdateRepo> updateRepos()
{
    std::vector<UpdateRepo> _repos;
    if (auto xml = readNetworkFile())
    {
        if (auto xml2 = xml->getChildByName ("Repositories"))
        {
            for (const auto* const e : xml2->getChildIterator())
            {
                UpdateRepo repo;

                if (auto c = e->getChildByName ("Host"))
                    repo.host = c->getAllSubText().toStdString();
                if (auto c = e->getChildByName ("Username"))
                    repo.username = c->getAllSubText().toStdString();
                if (auto c = e->getChildByName ("Password"))
                    repo.password = c->getAllSubText().toStdString();

                if (auto c = e->getChildByName ("Enabled"))
                {
                    auto st = c->getAllSubText();
                    repo.enabled = st.getIntValue() != 0;
                }

                if (! repo.host.empty())
                {
#if EL_TRACE_UPDATER
                    std::clog << "[element] found repo: " << repo.host << std::endl;
#endif
                    _repos.push_back (repo);
                }
            }
        }
    }
    return _repos;
}

//==============================================================================
class Updater::Updates : public juce::Thread,
                         public juce::AsyncUpdater
{
public:
    Updates (Updater& ur)
        : juce::Thread ("UpdateCheck"),
          owner (ur)
    {
    }

    std::vector<UpdatePackage> toPackages (const juce::XmlElement& xml)
    {
        std::vector<UpdatePackage> packages;
        for (auto e : xml.getChildWithTagNameIterator ("PackageUpdate"))
        {
            auto eID = e->getChildByName ("Name");
            auto eVersion = e->getChildByName ("Version");
            if (eID != nullptr && eVersion != nullptr)
            {
                UpdatePackage pkg;
                pkg.ID = eID->getAllSubText().trim().toStdString();
                pkg.version = eVersion->getAllSubText().trim().toStdString();
                packages.push_back (pkg);
            }
        }
#if EL_TRACE_UPDATER
        for (const auto& p : packages)
        {
            std::clog << "[updater] read: " << p.ID << " v" << p.version << std::endl;
        }
#endif
        return packages;
    }

    std::vector<UpdatePackage> toPackages (const juce::String& xmlStr)
    {
        if (auto xml = juce::XmlDocument::parse (xmlStr))
        {
            return toPackages (*xml);
        }
        return {};
    }

    std::vector<UpdatePackage> checkNow (std::string& xmlOut, const UpdateRepo& repoToCheck)
    {
        std::vector<UpdatePackage> packages;
        juce::URL url (repoToCheck.host);
        String urlStr = url.toString (true);

        if (updatesFilename != "latest.xml" && ! repoToCheck.username.empty() && ! repoToCheck.password.empty())
        {
            String creds = repoToCheck.username;
            creds << ":" << repoToCheck.password << "@";
            urlStr = urlStr.replace ("://", String ("://") + creds);
        }

        url = URL (urlStr);
        url = url.getChildURL (updatesFilename);
        urlStr = url.toString (true);

        int status = -1;
#if EL_TRACE_UPDATER
        std::clog << "[element] checking: " << urlStr.toStdString() << std::endl;
#endif
        auto options = juce::URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                           .withHttpRequestCmd ("GET")
                           .withStatusCode (&status)
                           .withConnectionTimeoutMs (2000);

        if (auto strm = url.createInputStream (options))
        {
            xmlOut = strm->readEntireStreamAsString().toStdString();
            packages = toPackages (xmlOut);
        }

#if EL_TRACE_UPDATER
        std::clog << "[element] response code: " << status << std::endl;
#endif

        return packages;
    }

    bool checkAsync()
    {
        if (isThreadRunning())
        {
            return true;
        }
        startThread (Thread::Priority::background);
        return isThreadRunning();
    }

    UpdateRepo fallbackRepo()
    {
        UpdateRepo repo;
        repo.enabled = true;
        repo.host = safeRepo();
        return repo;
    }

    void run() override
    {
        std::vector<UpdatePackage> pkgs;
        std::vector<UpdateRepo> repos (updateRepos());
        if (repos.empty())
            repos.push_back (fallbackRepo());

        std::string out;
        clearCached();

        for (const auto& repo : repos)
        {
            if (! repo.enabled)
                continue;
            out.clear();
            for (const auto& p : checkNow (out, repo))
                pkgs.push_back (p);
        }

        {
            juce::ScopedLock sl (lock);
            cachedXml = out;
            cachedPackages = pkgs;
#if EL_TRACE_UPDATER
            std::clog << "[updater] local: " << local.ID << ": " << local.version << std::endl;
            for (const auto& p : cachedPackages)
            {
                std::clog << "[updater] cached: " << p.ID << ": " << p.version << std::endl;
            }
#endif
        }

        triggerAsyncUpdate();
    }

    std::vector<UpdatePackage> safePackages() const noexcept
    {
        juce::ScopedLock sl (lock);
        return cachedPackages;
    }

    std::string safeRepo() const noexcept
    {
        juce::ScopedLock sl (lock);
        return repo;
    }

    std::string safeXml() const noexcept
    {
        juce::ScopedLock sl (lock);
        return cachedXml;
    }

    void handleAsyncUpdate() override
    {
#if EL_TRACE_UPDATER
        std::clog << "[updater] invoke updates available signal\n";
#endif
        owner.sigUpdatesAvailable();
    }

    void clearCached()
    {
        juce::ScopedLock sl (lock);
        cachedXml.clear();
        cachedPackages.clear();
    }

    std::vector<UpdatePackage> filterPackages (const UpdatePackage current,
                                               const std::vector<UpdatePackage>& input) const noexcept
    {
        element::Version ours (current.version);
        std::vector<UpdatePackage> ret;
        for (const auto& p : input)
        {
            if (current.ID != p.ID)
                continue;
            element::Version pv (p.version);
            if (pv > ours)
                ret.push_back (p);
        }
        return ret;
    }

    bool updateCached()
    {
        auto np = toPackages (safeXml());
        juce::ScopedLock sl (lock);
        cachedPackages = np;
        return true;
    }

    std::vector<UpdatePackage> available() const noexcept
    {
        return filterPackages (local, safePackages());
    }

private:
    friend class Updater;
    juce::CriticalSection lock;
    Updater& owner;
    UpdatePackage local;
    std::string exeFile;
    std::string updatesFilename { "latest.xml" };
    std::string repo;
    std::string cachedXml;
    std::vector<UpdatePackage> cachedPackages;
};

Updater::Updater()
{
    updates = std::make_unique<Updates> (*this);
    setExeFile (Updater::findExe ("updater"));
    setInfo (EL_PACKAGE_ID, EL_VERSION_STRING, EL_UPDATE_REPOSITORY_URL);
}

Updater::Updater (const std::string& package, const std::string& version, const std::string& repo)
{
    updates = std::make_unique<Updates> (*this);
    setInfo (package, version, repo);
}

Updater::~Updater()
{
    updates->cancelPendingUpdate();
    if (updates->isThreadRunning())
    {
        updates->stopThread (500);
    }
    updates.reset();
}

void Updater::clear()
{
    updates->clearCached();
}

void Updater::check (bool async)
{
#if EL_TRACE_UPDATER
    std::clog << "[updater] check inititated\n";
#endif
    if (async)
    {
        updates->checkAsync();
        return;
    }

    if (! updates->safeXml().empty())
    {
        updates->updateCached();
        return;
    }

    if (! updates->isThreadRunning())
        updates->run();
}

void Updater::setInfo (const std::string& package, const std::string& version, const std::string& repo)
{
    setInfo (package, version);
    setRepository (repo);
}

void Updater::setInfo (const std::string& package, const std::string& version)
{
    updates->local.ID = package;
    updates->local.version = version;
}

//==============================================================================
std::string Updater::findExe (const std::string& basename)
{
    String fileName (boost::trim_copy (basename));
#if JUCE_WINDOWS
    fileName << ".exe";
#elif JUCE_MAC
    fileName << ".app/Contents/MacOS/" << boost::trim_copy (basename);
#endif
    const auto updaterExe = DataPath::applicationDataDir()
                                .getChildFile ("installer")
                                .getChildFile (fileName);
    return updaterExe.getFullPathName().toStdString();
}

//==============================================================================
bool Updater::exists() const noexcept
{
    return File::isAbsolutePath (exeFile()) && File (exeFile()).exists();
}

void Updater::launch()
{
    Logger::writeToLog ("launching updater");
    if (updates->exeFile.empty())
        updates->exeFile = findExe ("updater");
    if (! exists())
    {
#if EL_TRACE_UPDATER
        std::clog << "[element] updater does not exist: " << std::endl
                  << "[element] " << updates->exeFile << std::endl;
#endif
        return;
    }

#if JUCE_MAC || JUCE_LINUX
    juce::File exeFile = File (updates->exeFile);
    exeFile.startAsProcess ("--su");
#elif JUCE_WINDOWS
    juce::File exeFile = File (updates->exeFile);
    exeFile.startAsProcess ("--su");
#endif
}

//==============================================================================
std::string Updater::exeFile() const noexcept { return updates->exeFile; }
void Updater::setExeFile (const std::string& file) { updates->exeFile = file; }
void Updater::setUpdatesFilename (const std::string& filename) { updates->updatesFilename = filename; }
std::string Updater::repository() const noexcept { return updates->safeRepo(); }
void Updater::setRepository (const std::string& url)
{
    juce::ScopedLock sl (updates->lock);
    updates->repo = url;
}

std::vector<UpdatePackage> Updater::packages() const noexcept { return updates->safePackages(); }
std::vector<UpdatePackage> Updater::available() const noexcept { return updates->available(); }

void Updater::setUpdatesXml (const std::string& xml)
{
    {
        juce::ScopedLock sl (updates->lock);
        updates->cachedXml = xml;
    }
    updates->updateCached();
}

} // namespace ui
} // namespace element

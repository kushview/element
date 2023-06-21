
#include <element/version.h>
#include <element/ui/updater.hpp>
#include <element/juce/core.hpp>
#include <element/juce/events.hpp>
#include <element/version.hpp>

#ifndef EL_TRACE_UPDATER
#define EL_TRACE_UPDATER 0
#endif

#define EL_PACKAGE_ID "net.kushview.element"
#define EL_UPDATES_REPO_URL_BASE "https://cd.kushview.net/element/release"

#if JUCE_MAC
#define EL_UPDATES_REPO_URL EL_UPDATES_REPO_URL_BASE "/osx"
#elif JUCE_WINDOWS
#define EL_UPDATES_REPO_URL EL_UPDATES_REPO_URL_BASE "/windows"
#else
#define EL_UPDATES_REPO_URL EL_UPDATES_REPO_URL_BASE "/linux"
#endif

using namespace juce;

namespace element {
namespace ui {

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
        forEachXmlChildElementWithTagName (xml, e, "PackageUpdate")
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

    std::vector<UpdatePackage> checkNow (std::string& xmlOut, const std::string& repoToCheck)
    {
        std::vector<UpdatePackage> packages;

        juce::URL url (repoToCheck);
        url = url.getChildURL ("Updates.xml");
        int status = 599;
#if EL_TRACE_UPDATER
        std::clog << "[element] repo: " << url.toString (false).toStdString() << std::endl;
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

        return packages;
    }

    bool checkAsync()
    {
        if (isThreadRunning())
        {
            return true;
        }
        startThread (4);
        return isThreadRunning();
    }

    void run() override
    {
        std::string out;
        clearCached();
        auto pkgs = checkNow (out, safeRepo());
        {
            juce::ScopedLock sl (lock);
            cachedXml = out;
            cachedPackages = pkgs;
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
    std::string repo;
    std::string cachedXml;
    std::vector<UpdatePackage> cachedPackages;
};

Updater::Updater()
{
    updates = std::make_unique<Updates> (*this);
    setExeFile (Updater::findExe ("updater"));
    setInfo (EL_PACKAGE_ID, EL_VERSION_STRING, EL_UPDATES_REPO_URL);
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
    const bool haveXml = ! updates->safeXml().empty();
    if (haveXml)
    {
        updates->updateCached();
        return;
    }

    if (async)
    {
        updates->checkAsync();
        return;
    }

    std::string xmlOut;
    updates->checkNow (xmlOut, updates->safeRepo());
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
    String fileName = basename;
#if JUCE_WINDOWS
    fileName << ".exe";
    fileName = String ("AppData/Roaming/Kushview/Element/") + fileName;
    juce::File updaterExe = File::getSpecialLocation (File::userHomeDirectory)
                                .getChildFile (fileName);
#elif JUCE_MAC
    fileName << ".app";
    fileName = String ("Library/Application Support/Kushview/Element/") + fileName;
    fileName << "/Contents/MacOS/" << basename;
    juce::File updaterExe = File::getSpecialLocation (File::userHomeDirectory)
                                .getChildFile (fileName);
#endif

    return updaterExe.getFullPathName().toStdString();
}

//==============================================================================
bool Updater::exists() const noexcept
{
    return File::isAbsolutePath (exeFile()) && File (exeFile()).exists();
}

void Updater::launch()
{
    StringArray cmd;
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
#if JUCE_WINDOWS
    cmd.add (String (updates->exeFile).quoted());

#elif JUCE_MAC
    // cmd.add ("open");
    cmd.add (String (updates->exeFile).quoted());
#endif
    cmd.add ("--su");
#if JUCE_MAC
    cmd.add ("&");
#endif
    std::system (cmd.joinIntoString (" ").toRawUTF8());
}

//==============================================================================
std::string Updater::exeFile() const noexcept { return updates->exeFile; }
void Updater::setExeFile (const std::string& file) { updates->exeFile = file; }
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

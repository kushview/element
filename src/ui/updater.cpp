// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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

    std::vector<UpdatePackage> checkNow (std::string& xmlOut)
    {
        std::vector<UpdatePackage> packages;
        juce::URL url (repo);
        url = url.getChildURL ("latest.xml");
        String urlStr = url.toString (true);

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

    void run() override
    {
        std::vector<UpdatePackage> pkgs;
        std::string out;
        clearCached();

        out.clear();
        for (const auto& p : checkNow (out))
            pkgs.push_back (p);

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
    std::string repo;
    std::string cachedXml;
    std::vector<UpdatePackage> cachedPackages;
};

Updater::Updater()
{
    updates = std::make_unique<Updates> (*this);
    setInfo (EL_PACKAGE_ID, ELEMENT_VERSION_STRING, ELEMENT_UPDATES_URL);
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

} // namespace element

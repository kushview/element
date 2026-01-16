// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <element/signals.hpp>

#ifndef ELEMENT_UPDATES_HOST
    #define ELEMENT_UPDATES_HOST "https://repo.kushview.net"
#endif

#ifndef ELEMENT_UPDATES_PATH
    #define ELEMENT_UPDATES_PATH "/element/1/stable"
#endif

#define ELEMENT_UPDATES_URL_BASE ELEMENT_UPDATES_HOST ELEMENT_UPDATES_PATH

#if JUCE_MAC
    #define ELEMENT_UPDATES_URL ELEMENT_UPDATES_URL_BASE "/osx"
#elif JUCE_WINDOWS
    #define ELEMENT_UPDATES_URL ELEMENT_UPDATES_URL_BASE "/windows"
#else
    #define ELEMENT_UPDATES_URL ELEMENT_UPDATES_URL_BASE "/linux"
#endif

namespace element {

/** An update package. */
struct UpdatePackage {
    /** Package (component) Identifier */
    std::string ID;
    /** Version number of the package
        can be 3 or 4 segements.
     */
    std::string version;
};

/** Updater helper for checking available updates */
class Updater {
public:
    static std::unique_ptr<Updater> create();

    virtual ~Updater();

    //==========================================================================
    /** Triggered when updates are found. Only fired when checking async. */
    Signal<void()> sigUpdatesAvailable;

    /** Check for updates in the background */
    void clear();

    /** Check for updates now or later. */
    virtual void check (bool async);

    /** Returns all package updates listed in the repo. */
    std::vector<UpdatePackage> packages() const noexcept;

    /** Returns available packages matching this updater's ID
        and also is a greater version.
    */
    std::vector<UpdatePackage> available() const noexcept;

    //==========================================================================
    /** Change updater / package / repo information */
    void setInfo (const std::string& package, const std::string& version, const std::string& url);

    /** Change updater / package information */
    void setInfo (const std::string& package, const std::string& version);

    //==========================================================================
    /** Returns the repository URL or file:/// path */
    std::string repository() const noexcept;

    /** Set the base URL to the repository to check for Updates with.
        This can also be a file:///path/to/folder on the system.
    */
    void setRepository (const std::string& url);

    //==========================================================================
    /** Override Online XML with local xml 
        Call clear() to wipe it out.
     */
    void setUpdatesXml (const std::string& xml);

protected:
    Updater();
    Updater (const std::string& package, const std::string& version, const std::string& url);

private:
    class Updates;
    std::unique_ptr<Updates> updates;
};

} // namespace element

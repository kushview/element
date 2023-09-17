// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <memory>
#include <string>

#include <element/signals.hpp>

#ifndef EL_UPDATE_REPOSITORY_HOST
    #define EL_UPDATE_REPOSITORY_HOST "https://repo.kushview.net"
#endif

#define EL_UPDATE_REPOSITORY_URL_BASE EL_UPDATE_REPOSITORY_HOST "/element/1/stable"

#if JUCE_MAC
    #define EL_UPDATE_REPOSITORY_URL EL_UPDATE_REPOSITORY_URL_BASE "/osx"
#elif JUCE_WINDOWS
    #define EL_UPDATE_REPOSITORY_URL EL_UPDATE_REPOSITORY_URL_BASE "/windows"
#else
    #define EL_UPDATE_REPOSITORY_URL EL_UPDATE_REPOSITORY_URL_BASE "/linux"
#endif

namespace element {
namespace ui {

/** An update package. */
struct UpdatePackage {
    /** Package (component) Identifier */
    std::string ID;
    /** Version number of the package
        can be 3 or 4 segements.
     */
    std::string version;
};

/** Updater helper that can deal with Qt Installer Framework installers */
class Updater {
public:
    Updater();
    Updater (const std::string& package, const std::string& version, const std::string& url);
    ~Updater();

    //==========================================================================
    /** Triggered when updates are found. Only fired when checking async. */
    Signal<void()> sigUpdatesAvailable;

    /** Check for updates in the background */
    void clear();

    /** Check for updates now or later. */
    void check (bool async);

    /** Checks if the updater program has been found on disk. */
    bool exists() const noexcept;

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
    /** Returns the EXE file of the updater program. */
    std::string exeFile() const noexcept;

    /** Changes the EXE file of the updater program. */
    void setExeFile (const std::string& file);

    /** Launch the updater GUI if possible. */
    void launch();

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

private:
    class Updates;
    std::unique_ptr<Updates> updates;
    static std::string findExe (const std::string& basename = "updater");
};

} // namespace ui
} // namespace element

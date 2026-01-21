// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>

#include <element/element.hpp>

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

/** Updater helper for checking available updates */
class Updater {
public:
    static std::unique_ptr<Updater> create();

    virtual ~Updater();

    /** Check for updates with dialog or in the background. Note that setting
        background to true still will cause a dialog to show if an update is 
        available. */
    virtual void check (bool background);

    /** Returns the repository URL or file:/// path */
    virtual std::string feedUrl() const noexcept;

    /** Set the base URL to the repository to check for Updates with.
        This can also be a file:///path/to/folder on the system.
    */
    virtual void setFeedUrl (const std::string& url);

protected:
    Updater();

private:
    EL_DISABLE_COPY (Updater)
};

} // namespace element

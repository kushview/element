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

/** Application update checker and installer.
    
    Provides functionality to check for, download, and install application updates.
    Uses platform-specific update mechanisms (Sparkle on macOS, WinSparkle on Windows).
 */
class Updater {
public:
    /** Creates a platform-specific updater instance.
        
        @return A unique pointer to the created updater, or nullptr if updates 
                are not supported on the current platform
     */
    static std::unique_ptr<Updater> create();

    virtual ~Updater();

    /** Checks for available updates.
        
        When background is true, the check is performed silently without showing UI 
        unless an update is found. When background is false, a dialog is presented 
        to the user immediately to show the check progress.
        
        If an update is available, a dialog will be shown to the user regardless of 
        the background parameter, allowing them to review release notes and choose 
        whether to install the update.
        
        This method returns immediately; the actual check happens asynchronously.
        
        @param background If true, checks silently in the background. If false, 
                         shows immediate UI feedback to the user.
     */
    virtual void check (bool background);

    /** Returns the currently configured update feed URL.
        
        @return The repository URL (http/https) or file system path (file:///) 
                where the appcast feed is located
     */
    virtual std::string feedUrl() const noexcept;

    /** Sets the update feed URL.
        
        Configures the base URL or file path to check for updates. This should point 
        to the location of the appcast XML file that describes available updates.
        
        @param url The repository URL (e.g., "https://example.com/appcast.xml") or 
                  local file path (e.g., "file:///path/to/appcast.xml"). Note that
                  local file may or may not work depending on platform.
     */
    virtual void setFeedUrl (const std::string& url);

protected:
    /** Protected constructor. Use create() to instantiate. */
    Updater();

private:
    EL_DISABLE_COPY (Updater)
};

} // namespace element

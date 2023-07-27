// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/script.hpp>

namespace element {

using ScriptArray = juce::Array<ScriptInfo>;

class ScriptManager final
{
public:
    ScriptManager();
    ~ScriptManager();

    void scanDefaultLocation();
    void scanDirectory (const juce::File&);

    int getNumScripts() const;
    ScriptInfo getScript (int) const;

    const ScriptArray& getScriptsDSP() const;

    /** Returns the application's script directory.
        Scripts in the app data dir
    */
    static juce::File getApplicationScriptsDir();

    /** Returns the system scripts directory.
        The location where scripts were installed or packaged
    */
    static juce::File getSystemScriptsDir();

    /** Scripts in the users home dir. e.g. ~/.local/share */
    static juce::File getHomeScriptsDir();

    /** User scripts location. e.g. $HOME/Music/Element/Scripts */
    static juce::File getUserScriptsDir();

private:
    class Registry;
    std::unique_ptr<Registry> registry;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptManager)
};

} // namespace element

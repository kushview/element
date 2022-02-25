/*
    This file is part of Element
    Copyright (C) 2020-2021  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "scripting/ScriptDescription.h"

namespace Element {

using ScriptArray = Array<ScriptDescription>;

class ScriptManager final
{
public:
    ScriptManager();
    ~ScriptManager();

    void scanDefaultLocation();
    void scanDirectory (const File&);

    int getNumScripts() const;
    ScriptDescription getScript (int) const;

    const ScriptArray& getScriptsDSP() const;

    /** Returns the application's script directory.
        Scripts in the app data dir
    */
    static File getApplicationScriptsDir();

    /** Returns the system scripts directory.
        The location where scripts were installed or packaged
    */
    static File getSystemScriptsDir();

    /** Scripts in the users home dir. e.g. ~/.local/share */
    static File getHomeScriptsDir();

    /** User scripts location. e.g. $HOME/Music/Element/Scripts */
    static File getUserScriptsDir();

private:
    class Registry;
    std::unique_ptr<Registry> registry;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptManager);
};

} // namespace Element

/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

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

class ScriptManager final
{
public:
    ScriptManager();
    ~ScriptManager();

    Array<ScriptDescription> scanDirectory (File dir);

private:
    // class Registry;
    // std::unique_ptr<Registry> scripts;
    FileSearchPath locations;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptManager);
};

}

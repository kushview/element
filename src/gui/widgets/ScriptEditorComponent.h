/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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

#include "gui/LuaTokeniser.h"

namespace Element {

/** A juce::CodeEditorComponent that sets some default options and color scheme */
class ScriptEditorComponent : public CodeEditorComponent
{
public:
    /** Create a new script editor.
        
        @see juce::CodeDocument
    */
    ScriptEditorComponent (CodeDocument& document, CodeTokeniser* tokens)
        : CodeEditorComponent (document, tokens)
    {
        setTabSize (4, true);
        setFont (getFont().withHeight (getDefaultFontHeight()));
        setColourScheme (kv::LuaTokeniser().getDefaultColourScheme());
    }

    virtual ~ScriptEditorComponent() = default;

    //==============================================================================
    /** Returns a font height that looks 'good' in most systems */
    static float getDefaultFontHeight() { return defaultFontHeight; }

private:
#if JUCE_MAC
    static constexpr float defaultFontHeight = 14.5f;
#elif JUCE_WINDOWS
    static constexpr float defaultFontHeight = 13.f;
#elif JUCE_LINUX
    static constexpr float defaultFontHeight = 16.f;
#else
    static constexpr float defaultFontHeight = 15.f;
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptEditorComponent)
};

} // namespace Element

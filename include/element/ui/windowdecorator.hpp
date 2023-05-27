/*
    This file is part of Element
    Copyright (C) 2023  Kushview, LLC.  All rights reserved.

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

#include <memory>

#include <element/juce/core.hpp>
#include <element/juce/gui_basics.hpp>

#include <element/element.hpp>

namespace element {

class ContentComponent;

/** Used to customize Element's Main Window.
    Create one of these and pass it to GuiService in your initialization code.
 */
class WindowDecorator {
public:
    virtual ~WindowDecorator() = default;

    /** Create a main content by type name.
        
        Return the content specified.  If type is empty or not supported,
        you should still return a valid Content object.  The object returned
        will be used as the content component in the main window.
    */
    virtual std::unique_ptr<ContentComponent> createMainContent (const juce::String& type) = 0;

    /** Create a menu bar model to use in the Main Window. */
    virtual std::unique_ptr<juce::MenuBarModel> createMainMenuBarModel() { return nullptr; }

    /** Return a function to use when setting the Main Window's title. If this
        returns nullptr, Element will fallback to default titling.
     */
    virtual std::function<juce::String()> getMainWindowTitler() { return nullptr; }

protected:
    WindowDecorator() = default;

private:
    EL_DISABLE_COPY (WindowDecorator)
};

} // namespace element

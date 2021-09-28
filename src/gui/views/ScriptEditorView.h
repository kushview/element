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

#include "gui/ContentComponent.h"
#include "gui/widgets/ScriptEditorComponent.h"

namespace Element {

class ScriptSource;

class ScriptEditorView : public ContentView
{
public:
    ScriptEditorView();
    ~ScriptEditorView();

    /** Returns the code document used by this view */
    CodeDocument& getCodeDocument() { return code; }

    /** Reset the buffer, undo history, and save point */
    void reset();

    /** @internal */
    void resized() override;

   #if 0
    virtual void initializeView (AppController&) { }
    virtual void willBeRemoved() { }
    virtual void willBecomeActive() { }
    virtual void didBecomeActive() { }
    virtual void stabilizeContent() { }

    /** Save state to user settings */
    virtual void saveState (PropertiesFile*) {}

    /** Restore state from user settings */
    virtual void restoreState (PropertiesFile*) {}

    /** Get state attached to session */
    virtual void getState (String&) {}

    /** Apply state attached to session */
    virtual void setState (const String&) {}
   #endif

private:
    std::unique_ptr<ScriptEditorComponent> editor;
    std::unique_ptr<ScriptSource> source;
    kv::LuaTokeniser tokens;
    CodeDocument code;    
};

}

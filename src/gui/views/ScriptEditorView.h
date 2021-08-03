#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/ScriptEditorComponent.h"

namespace Element {

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
    kv::LuaTokeniser tokens;
    CodeDocument code;
};

}

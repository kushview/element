// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/view.hpp>
#include <element/node.hpp>
#include <element/script.hpp>

#include "scripting/scriptsource.hpp"
#include "ui/luatokenizer.hpp"

namespace element {

class Context;
class ScriptNode;

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
        setColourScheme (element::LuaTokeniser().getDefaultColourScheme());
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

//==============================================================================
class BaseScriptEditorView : public View
{
protected:
    BaseScriptEditorView();

public:
    virtual ~BaseScriptEditorView();

    ScriptEditorComponent& getEditor() { return *editor; }

    /** Returns the code document used by this view */
    CodeDocument& getCodeDocument() { return code; }

    /** Returns the code document used by this view */
    const CodeDocument& getCodeDocument() const { return code; }

    /** Reload the buffer. */
    void reload();

    /** Reset the buffer, undo history, and save point */
    void reset();

    /** @internal */
    virtual void resized() override;

    //=========================================================================
    /** @internal */

#if 0
    void initializeView (Services&) override;
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

protected:
    virtual String getScriptContent() const = 0;

private:
    std::unique_ptr<ScriptEditorComponent> editor;
    element::LuaTokeniser tokens;
    juce::CodeDocument code;
};

//==============================================================================
class ScriptEditorView : public BaseScriptEditorView
{
public:
    ScriptEditorView() = delete;
    ScriptEditorView (const Script& s);
    virtual ~ScriptEditorView();

    /** Update the script model with the editor contents. */
    void updateScript();

    void setSaveButtonVisible (bool visible);

    /** @internal */
    void resized() override;

protected:
    String getScriptContent() const override;

private:
    Script script;
    TextButton saveButton;
};

class ObservantScriptEditorView : public ScriptEditorView
{
public:
    ObservantScriptEditorView() = delete;
    ObservantScriptEditorView (Context& ctx, const Script& s);
    ~ObservantScriptEditorView();

private:
    std::vector<boost::signals2::connection> connections;
};

//==============================================================================
class ScriptNodeScriptEditorView : public BaseScriptEditorView
{
public:
    ScriptNodeScriptEditorView (Context& ctx, const Node& n, bool editUI);
    ~ScriptNodeScriptEditorView();

    bool isEditingUI() const { return editingUI; }

    /** @internal */
    void resized() override;

    juce::Result updateScript();

protected:
    String getScriptContent() const override;

private:
    Node node;
    bool editingUI;
    TextButton applyButton;
    std::vector<boost::signals2::connection> connections;
};

} // namespace element

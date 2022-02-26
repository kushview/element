/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    - Author Michael Fisher <mfisher@kushvie.net>

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

#include "../../../libs/element/lua/el/object.hpp"
#include "gui/nodes/ScriptNodeEditor.h"
#include "gui/LookAndFeel.h"
#include "scripting/LuaBindings.h"
#include "scripting/ScriptingEngine.h"
#include "scripting/ScriptManager.h"
#include "scripting/Script.h"

namespace Element {

//==============================================================================
class ControlPort : private ParameterListener
{
public:
    ControlPort (Parameter* parameter)
        : ParameterListener (parameter)
    {
        param = parameter;
        control = dynamic_cast<ControlPortParameter*> (param.get());
    }

    virtual ~ControlPort()
    {
        param = nullptr;
        control = nullptr;
    }

    float getValue() const { return param->getValue(); }
    void setValue (float val) { param->setValue (val); }

    bool isControl() const { return control != nullptr; }

    float getControl() const
    {
        if (control)
            return control->get();
        return param->getValue();
    }

    void setControl (float val)
    {
        if (control)
            control->set (val);
        else
            param->setValueNotifyingHost (val);
    }

    std::function<void()> onValueChange;

private:
    Parameter::Ptr param;
    ControlPortParameter::Ptr control;
    void handleNewParameterValue() override
    {
        if (onValueChange)
            onValueChange();
    }
};

class ScriptNodeControlPort : public ControlPort
{
public:
    ScriptNodeControlPort (Parameter* param)
        : ControlPort (param)
    {
        onValueChange = [this]() {
            if (changed.valid())
                changed();
        };
    }

    ~ScriptNodeControlPort() override {}

    sol::function getChangedFunction() const { return changed; }
    void setChangedFunction (const sol::function& f) { changed = f; }

private:
    sol::function changed;
};

//==============================================================================
class LuaNodeParameterPropertyFloat : public PropertyComponent,
                                      private ParameterListener
{
public:
    LuaNodeParameterPropertyFloat (Parameter::Ptr p)
        : PropertyComponent (p->getName (1024)),
          ParameterListener (p),
          param (p)
    {
        if (param->getLabel().isNotEmpty())
        {
            auto name = getName();
            name << " (" << param->getLabel() << ")";
            setName (name);
        }

        addAndMakeVisible (slider);
        slider.setRange (0.0, 1.0, 0.0);
        slider.setSkewFactor (1.0, false);
        slider.setSliderStyle (Slider::LinearBar);

        slider.onDragStart = [this]() {
            dragging = true;
            param->beginChangeGesture();
        };

        slider.onDragEnd = [this]() {
            dragging = false;
            param->endChangeGesture();
        };

        slider.onValueChange = [this] {
            auto newValue = (float) slider.getValue();
            if (param->getValue() != newValue)
            {
                if (! dragging)
                    param->beginChangeGesture();

                param->setValueNotifyingHost (newValue);

                if (! dragging)
                    param->endChangeGesture();
            }
        };

        slider.valueFromTextFunction = [this] (const String& text) -> double {
            if (auto* cp = dynamic_cast<ControlPortParameter*> (param.get()))
                return (double) cp->convertTo0to1 (text.getFloatValue());
            return text.getDoubleValue();
        };

        slider.textFromValueFunction = [this] (double value) -> String {
            String text;
            if (auto* cp = dynamic_cast<ControlPortParameter*> (param.get()))
                return cp->getText (static_cast<float> (value), 1024);
            return String (value, 6);
        };

        refresh();
        slider.updateText();
    }

    void refresh() override
    {
        if (static_cast<float> (slider.getValue()) != param->getValue())
            slider.setValue (param->getValue(), dontSendNotification);
    }

private:
    Slider slider;
    Parameter::Ptr param;
    bool dragging = false;

    void handleNewParameterValue() override
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
        if (! dragging)
            slider.setValue (param->getValue(), dontSendNotification);
    }
};

//==============================================================================
static ValueTree getUIChild (const Node& node, const String& name)
{
    auto UI = node.getUIValueTree();
    return UI.getOrCreateChildWithName (name, nullptr);
}

static ValueTree getScriptNodeEditorState (const Node& node)
{
    return getUIChild (node, "ScriptNodeEditor");
}

//==============================================================================
ScriptNodeEditor::ScriptNodeEditor (ScriptingEngine& scripts, const Node& node)
    : NodeEditorComponent (node),
      engine (scripts),
      state (engine.getLuaState()),
      env (state, sol::create, state.globals()),
      fileBrowser (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                   ScriptManager::getUserScriptsDir(),
                   nullptr,
                   nullptr)
{
    setOpaque (true);

    auto M = state.create_table();
    M.new_usertype<ScriptNodeControlPort> (
        "ControlPort", sol::no_constructor,
#if 0
        "value",        sol::overload (
            [](ScriptNodeControlPort& self) -> double {
                return self.getValue();
            },
            [](ScriptNodeControlPort& self, bool normal) -> double {
                return normal ? self.getValue() : self.getControl();
            },
            [](ScriptNodeControlPort& self, double value) -> double {
                self.setValue (static_cast<float> (value));
                return self.getValue();
            },
            [](ScriptNodeControlPort& self, double value, bool normal) -> double {
                if (normal) self.setValue (static_cast<float> (value));
                else        self.setControl (static_cast<float> (value));
                return normal ? self.getValue() : self.getControl();
            }
        ),
#endif
        "get",
        [] (ScriptNodeControlPort& self) -> double { return self.getControl(); },
        "set",
        [] (ScriptNodeControlPort& self, double value) -> void { self.setControl (static_cast<float> (value)); },

        "valuechanged",
        sol::property (&ScriptNodeControlPort::getChangedFunction, &ScriptNodeControlPort::setChangedFunction));
    env["ScriptNodeEditor.ControlPort"] = M;

    lua = getNodeObjectOfType<ScriptNode>();
    jassert (lua);

    chooser.reset (new FileChooser ("Script", ScriptManager::getUserScriptsDir(), "*.lua", false, false, this));

    addAndMakeVisible (paramsButton);
    paramsButton.setButtonText ("Params");
    paramsButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    paramsButton.onClick = [this]() {
        paramsButton.setToggleState (! paramsButton.getToggleState(), dontSendNotification);
        props.setVisible (paramsButton.getToggleState());
        updateSize();
    };

    addAndMakeVisible (props);
    props.setVisible (paramsButton.getToggleState());

    lua->addChangeListener (this);
    portsChangedConnection = lua->portsChanged.connect (
        std::bind (&ScriptNodeEditor::onPortsChanged, this));

    setSize (660, 480);

    const auto SNE = getScriptNodeEditorState (getNode());
    if ((bool) SNE.getProperty ("showParams", false))
    {
        paramsButton.setToggleState (true, dontSendNotification);
        props.setVisible (true);
    }

    updateAll();
    updatePreview();
    resized();
}

ScriptNodeEditor::~ScriptNodeEditor()
{
    portsChangedConnection.disconnect();
    lua->removeChangeListener (this);

    auto SNE = getScriptNodeEditorState (getNode());
    SNE.setProperty ("showParams", paramsButton.getToggleState(), nullptr);
}

//==============================================================================
sol::table ScriptNodeEditor::createContext()
{
    using CPP = ControlPortParameter;
    sol::state_view view (state.lua_state());
    sol::table ctx = view.create_table();

    ctx["params"] = view.create_table();
    for (auto* param : lua->getParameters())
    {
        ctx["params"][1 + param->getParameterIndex()] =
            std::make_shared<ScriptNodeControlPort> (param);
    }

    return ctx;
}

//==============================================================================
void ScriptNodeEditor::updateAll()
{
    updateCodeEditor();
    updateProperties();
    updatePreview();
}

void ScriptNodeEditor::updateCodeEditor()
{
    // refresh script editor view(s) if possible
}

void ScriptNodeEditor::updateProperties()
{
    props.clear();
    Array<PropertyComponent*> pcs;
    for (auto* param : lua->getParameters())
    {
        if (! param->isAutomatable())
            continue;
        pcs.add (new LuaNodeParameterPropertyFloat (param));
    }
    props.addProperties (pcs);
}

void ScriptNodeEditor::updateSize()
{
    if (comp != nullptr)
    {
        int w = comp->getWidth(), h = comp->getHeight();
        if (props.isVisible())
        {
            w += propsGap;
            w += propsWidth;
        }

        h += 22; // toolbarsize
        setSize (w, h);
        return;
    }
}

void ScriptNodeEditor::updatePreview()
{
    if (comp != nullptr)
    {
        removeChildComponent (comp);
        widget = sol::table();
        comp = nullptr;
    }

    try
    {
        Script loader (state);
        if (loader.load (lua->getCodeDocument (true).getAllContent()))
        {
            auto f = loader.caller();
            env.set_on (f);
            auto ctx = createContext();
            sol::protected_function_result instance = f (ctx);

            if (! instance.valid())
            {
                sol::error e = instance;
                for (const auto& line : StringArray::fromLines (e.what()))
                    log (line);
                return;
            }

            if (instance.get_type() == sol::type::table)
            {
                sol::table DSPUI = instance;
                sol::table editor;

                switch (DSPUI["editor"].get_type())
                {
                    case sol::type::function: {
                        sol::function instantiate = DSPUI["editor"];
                        auto editorResult = instantiate (ctx);
                        if (! editorResult.valid())
                        {
                            sol::error e = editorResult;
                            for (const auto& line : StringArray::fromLines (e.what()))
                                log (line);
                        }
                        else if (editorResult.get_type() == sol::type::table)
                        {
                            editor = editorResult;
                        }
                        break;
                    }
                    default:
                        break;
                }

                if (auto* const c = kv::lua::object_userdata<Component> (editor))
                {
                    comp = c;
                    widget = editor;
                    addAndMakeVisible (*comp);
                    comp->setAlwaysOnTop (true);
                    updateSize();
                }
                else
                {
                    log ("ScriptNodeEditor: didn't get widget from DSPUI script");
                }
            }
        }
        else
        {
            log (loader.getErrorMessage());
        }
    } catch (const std::exception& e)
    {
        for (const auto& line : StringArray::fromLines (e.what()))
            log (line);
    }

    resized();
}

void ScriptNodeEditor::onPortsChanged()
{
    updateProperties();
}

void ScriptNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    updateAll();
    resized();
}

void ScriptNodeEditor::paint (Graphics& g)
{
    g.fillAll (Element::LookAndFeel::widgetBackgroundColor.darker());
}

void ScriptNodeEditor::resized()
{
    const int toolbarSize = 22;
    auto r1 = getLocalBounds().reduced (4);
    auto r2 = r1.removeFromTop (toolbarSize);

    fileBrowser.setBounds (r1.reduced (8));

    paramsButton.changeWidthToFitText (r2.getHeight());
    paramsButton.setBounds (r2.removeFromRight (paramsButton.getWidth()));

    if (comp != nullptr)
    {
        comp->setBounds (0, r2.getBottom(), comp->getWidth(), comp->getHeight());
    }

    if (props.isVisible())
    {
        props.setBounds (comp != nullptr ? comp->getRight() + propsGap : 0,
                         comp != nullptr ? comp->getY() : 0,
                         propsWidth,
                         getHeight() - (comp != nullptr ? comp->getY() : 0));
    }
}

} // namespace Element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/style.hpp>

#include "el/object.hpp"
#include "nodes/genericeditor.hpp"
#include "nodes/scriptnodeeditor.hpp"
#include "scripting/bindings.hpp"
#include "scripting.hpp"
#include "scripting/scriptmanager.hpp"
#include "scripting/scriptloader.hpp"

namespace element {

//==============================================================================
class ControlPort : private ParameterObserver
{
public:
    ControlPort (Parameter* parameter)
        : ParameterObserver (parameter)
    {
        param = parameter;
        control = dynamic_cast<RangedParameter*> (param.get());
        observeParameter (param);
    }

    virtual ~ControlPort()
    {
        observeParameter (nullptr);
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

    float getMin() const noexcept
    {
        if (control)
            return control->getNormalisableRange().start;
        return 0.f;
    }

    float getMax() const noexcept
    {
        if (control)
            return control->getNormalisableRange().end;
        return 1.f;
    }

    std::function<void()> onValueChange;

private:
    ParameterPtr param;
    RangedParameterPtr control;
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
                                      private ParameterObserver
{
public:
    LuaNodeParameterPropertyFloat (ParameterPtr p)
        : PropertyComponent (p->getName (1024)),
          ParameterObserver (p),
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
            if (auto* cp = dynamic_cast<RangedParameter*> (param.get()))
                return (double) cp->convertTo0to1 (text.getFloatValue());
            return text.getDoubleValue();
        };

        slider.textFromValueFunction = [this] (double value) -> String {
            String text;
            if (auto* cp = dynamic_cast<RangedParameter*> (param.get()))
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
    ParameterPtr param;
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
    : NodeEditor (node),
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
        // clang format-off
        "ControlPort",
        sol::no_constructor,
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
        "min",
        [] (ScriptNodeControlPort& self) -> double { return self.getMin(); },
        "max",
        [] (ScriptNodeControlPort& self) -> double { return self.getMax(); },
        "changed",
        sol::property (&ScriptNodeControlPort::getChangedFunction, &ScriptNodeControlPort::setChangedFunction)
        // clang-format on
    );
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

void ScriptNodeEditor::setToolbarVisible (bool visible)
{
    if (showToolbar == visible)
        return;
    showToolbar = visible;
    props.setVisible (showToolbar);
    paramsButton.setVisible (showToolbar);
    updateSize();
}

//==============================================================================
sol::table ScriptNodeEditor::createContext()
{
    sol::state_view view (state.lua_state());
    sol::table ctx = view.create_table();

    ctx["params"] = view.create_table();
    for (auto* param : lua->getParameters (true))
    {
        auto obj = std::make_shared<ScriptNodeControlPort> (param);
        ctx["params"][1 + param->getParameterIndex()] = obj;
        if (auto rp = dynamic_cast<RangedParameter*> (param))
            ctx.set (rp->getPort().symbol.trim().toStdString(), obj);
    }

    ctx["controls"] = view.create_table();
    for (auto* param : lua->getParameters (false))
    {
        auto obj = std::make_shared<ScriptNodeControlPort> (param);
        ctx["controls"][1 + param->getParameterIndex()] = obj;
        if (auto rp = dynamic_cast<RangedParameter*> (param))
            ctx.set (rp->getPort().symbol.trim().toStdString(), obj);
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
    if (_generic != nullptr)
    {
        setSize (_generic->getWidth(), _generic->getHeight());
    }

    if (comp != nullptr)
    {
        int w = comp->getWidth(), h = comp->getHeight();
        if (showToolbar && props.isVisible())
        {
            w += propsGap;
            w += propsWidth;
        }

        if (showToolbar)
            h += 22; // toolbarsize
        setSize (w, h);
        return;
    }
}

void ScriptNodeEditor::updatePreview()
{
    if (_generic != nullptr)
    {
        _generic.reset();
    }

    if (comp != nullptr)
    {
        removeChildComponent (comp);
        widget = sol::table();
        comp = nullptr;
    }

    bool ok = false;
    auto& codeDoc = lua->getCodeDocument (true);
    const auto code = codeDoc.getAllContent();

    if (code.trim().isEmpty())
    {
        // show generic editor if document is empty...
        _generic = std::make_unique<GenericNodeEditor> (getNode());
        addAndMakeVisible (_generic.get());
        updateSize();
        return;
    }

    try
    {
        ScriptLoader loader (state);
        if (loader.load (code))
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
                const bool canResize = DSPUI.get_or ("resizable", false);

                std::string factoryFn = "instantiate";
                if (DSPUI[factoryFn].get_type() != sol::type::function)
                    factoryFn = "editor";

                switch (DSPUI[factoryFn].get_type())
                {
                    case sol::type::function: {
                        sol::function instantiate = DSPUI[factoryFn];
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

                if (auto* const c = lua::object_userdata<Component> (editor))
                {
                    comp = c;
                    widget = editor;
                    addAndMakeVisible (*comp);
                    comp->setAlwaysOnTop (true);
                    setResizable (canResize);
                    updateSize();
                    ok = true;
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

    if (! ok)
    {
        // noop
    }

    resized();
}

void ScriptNodeEditor::onPortsChanged()
{
    updateAll();
}

void ScriptNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    updateAll();
    resized();
}

void ScriptNodeEditor::paint (Graphics& g)
{
    g.fillAll (element::Colors::widgetBackgroundColor.darker());
}

void ScriptNodeEditor::resized()
{
    if (_generic != nullptr)
    {
        _generic->setBounds (0, 0, _generic->getWidth(), _generic->getHeight());
        return;
    }

    const int toolbarSize = 22;
    auto r1 = getLocalBounds().reduced (4);
    auto r2 = r1.removeFromTop (toolbarSize);

    if (fileBrowser.isVisible())
        fileBrowser.setBounds (r1.reduced (8));

    if (showToolbar)
    {
        paramsButton.changeWidthToFitText (r2.getHeight());
        paramsButton.setBounds (r2.removeFromRight (paramsButton.getWidth()));
    }

    if (comp != nullptr)
    {
        if (resizable())
        {
            comp->setBounds (getLocalBounds());
        }
        else
        {
            auto compY = showToolbar ? r2.getBottom() : 0;
            comp->setBounds (0, compY, comp->getWidth(), comp->getHeight());
        }
    }

    if (showToolbar && props.isVisible())
    {
        props.setBounds (comp != nullptr ? comp->getRight() + propsGap : 0,
                         comp != nullptr ? comp->getY() : 0,
                         propsWidth,
                         getHeight() - (comp != nullptr ? comp->getY() : 0));
    }
}

} // namespace element

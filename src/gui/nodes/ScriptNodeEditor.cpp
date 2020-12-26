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

#include "gui/nodes/ScriptNodeEditor.h"
#include "gui/LookAndFeel.h"

namespace Element {

//==============================================================================
// class ScriptNodeEditorTabs : public TabbedComponent
// {
// public:
//     ScriptNodeEditorTabs() {
//         addTab ("DSP", Colours::black, new Component(), true);
//     }
// };

//==============================================================================
static CodeEditorComponent::ColourScheme luaColors()
{
    static const CodeEditorComponent::ColourScheme::TokenType types[] =
    {
         { "Error",          Colour (0xffcc0000) },
         { "Comment",        Colour (0xff6a9955) },
         { "Keyword",        Colour (0xff569cd6) },
         { "Operator",       Colour (0xffb3b3b3) },
         { "Identifier",     Colour (0xffc5c5c5) },
         { "Integer",        Colour (0xffb5cea8) },
         { "Float",          Colour (0xffb5cea8) },
         { "String",         Colour (0xffce9178) },
         { "Bracket",        Colour (0xffd4d4d4) },
         { "Punctuation",    Colour (0xffb3b3b3) },
         { "Preprocessor Text", Colour (0xffc586c0) } // used for control statements
    };

    CodeEditorComponent::ColourScheme cs;

    for (auto& t : types)
        cs.set (t.name, Colour (t.colour));

    return cs;
}

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

        slider.onDragStart = [this]()
        {
            dragging = true;
            param->beginChangeGesture();
        };

        slider.onDragEnd = [this]()
        {
            dragging = false;
            param->endChangeGesture();
        };

        slider.onValueChange = [this]
        {
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

        slider.valueFromTextFunction = [this](const String& text) -> double
        {
             if (auto* cp = dynamic_cast<ControlPortParameter*> (param.get()))
                return (double) cp->convertTo0to1 (text.getFloatValue());
            return text.getDoubleValue();
        };

        slider.textFromValueFunction = [this](double value) -> String
        {
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

ScriptNodeEditor::ScriptNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    lua = getNodeObjectOfType<ScriptNode>();
    jassert (lua);

    setOpaque (true);

    addAndMakeVisible (compileButton);
    compileButton.setButtonText ("Compile");
    compileButton.onClick = [this]()
    {
        // const auto script = document.getAllContent();
        // auto result = lua->loadScript (script);
        // if (! result.wasOk())
        // {
        //     AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
        //         "Script Error", result.getErrorMessage());
        // }
    };

    addAndMakeVisible (editorButton);
    editorButton.setButtonText ("Params");
    editorButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    editorButton.onClick = [this]()
    {
        editorButton.setToggleState (! editorButton.getToggleState(), dontSendNotification);
        props.setVisible (editorButton.getToggleState());
        resized();
    };

    addAndMakeVisible (modeButton);
    modeButton.setButtonText ("Script");
    modeButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    modeButton.onClick = [this]()
    { 
        modeButton.setToggleState (! modeButton.getToggleState(), dontSendNotification);
        modeButton.setButtonText (modeButton.getToggleState() ? "Editor" : "Script");
        updateCodeEditor();
    };

    addAndMakeVisible (props);
    props.setVisible (editorButton.getToggleState());

    updateAll();

    lua->addChangeListener (this);
    portsChangedConnection = lua->portsChanged.connect (
        std::bind (&ScriptNodeEditor::onPortsChanged, this));

    setSize (660, 480);
}

ScriptNodeEditor::~ScriptNodeEditor()
{
    portsChangedConnection.disconnect();
    lua->removeChangeListener (this);
    editor.reset();
}

void ScriptNodeEditor::updateAll()
{
    updateCodeEditor();
    updateProperties();
}

void ScriptNodeEditor::updateCodeEditor()
{
    editor.reset (new CodeEditorComponent (getActiveDoc(), &tokens));
    addAndMakeVisible (editor.get());
    editor->setTabSize (4, true);
    editor->setFont (editor->getFont().withHeight (18));
    editor->setColourScheme (luaColors());
    resized();
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

void ScriptNodeEditor::onPortsChanged()
{
    updateProperties();
}

CodeDocument& ScriptNodeEditor::getActiveDoc()
{
    return lua->getCodeDocument (modeButton.getToggleState());
}

void ScriptNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    updateAll();
    resized();
}

void ScriptNodeEditor::paint (Graphics& g)
{
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void ScriptNodeEditor::resized()
{
    auto r1 = getLocalBounds().reduced (4);
    auto r2 = r1.removeFromTop (22);
    
    compileButton.changeWidthToFitText (r2.getHeight());
    compileButton.setBounds (r2.removeFromLeft (compileButton.getWidth()));
    editorButton.changeWidthToFitText (r2.getHeight());
    editorButton.setBounds (r2.removeFromRight (editorButton.getWidth()));
    r2.removeFromRight (2);
    modeButton.changeWidthToFitText (r2.getHeight());
    modeButton.setBounds (r2.removeFromRight (modeButton.getWidth()));

    r1.removeFromTop (2);
    if (props.isVisible())
    {
        props.setBounds (r1.removeFromRight (220));
        r1.removeFromRight (2);
    }

    editor->setBounds (r1);
}

}

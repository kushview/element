/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "gui/PluginWindow.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define EL_PLUGIN_MIN_WIDTH     546
#define EL_PLUGIN_MIN_HEIGHT    266

#define EL_PLUGIN_DEFAULT_WIDTH     760
#define EL_PLUGIN_DEFAULT_HEIGHT    480

namespace Element {
using Element::LookAndFeel;

class ElementPluginAudioProcessorEditor::ParamTable : public TableListBox,
                                                      public TableListBoxModel
{
public:
    ParamTable()
    {
        setModel (this);
    }

    ~ParamTable()
    {
        setModel (nullptr);
    }

    int getNumRows() override { return 8; }

    void paintRowBackground (Graphics&, int rowNumber,
                             int width, int height,
                             bool rowIsSelected) override {}

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        g.setColour (Colours::black);
        g.fillRect (0, 0, width, height);
    }

#if 0
    virtual Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate);
    virtual void cellClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);
    virtual int getColumnAutoSizeWidth (int columnId);
    virtual String getCellTooltip (int rowNumber, int columnId);
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows);
#endif
};

void PerformanceParameterSlider::mouseDown (const MouseEvent& ev)
{
    auto* const editor = findParentComponentOfClass<ElementPluginAudioProcessorEditor>();
    if (editor != nullptr && ev.mods.isPopupMenu())
    {
        auto& processor = editor->getProcessor();
        PopupMenu menu = processor.getPerformanceParameterMenu (param.getParameterIndex());
        const int paramIndex = param.getParameterIndex();
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent(this),
                            std::bind (&ElementPluginAudioProcessor::handlePerformanceParameterResult, &processor,
                            std::placeholders::_1, paramIndex));
        return;
    }
    
    return Slider::mouseDown (ev);
}

class ElementPluginAudioProcessorEditor::ParamTableToggle : public Button
{
public:
    ParamTableToggle() : Button ("ParamToggle") {}
    ~ParamTableToggle() { }

protected:
    void paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override
    {
        ignoreUnused (shouldDrawButtonAsDown);
        getLookAndFeel().drawTreeviewPlusMinusBox (g, getLocalBounds().toFloat().reduced (2),
            kv::LookAndFeel_KV1::widgetBackgroundColor.darker(), getToggleState(),
            shouldDrawButtonAsHighlighted);
    }
};

ElementPluginAudioProcessorEditor::ElementPluginAudioProcessorEditor (ElementPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    paramTable.reset (new ParamTable());
    addAndMakeVisible (paramTable.get());
    paramToggle.reset (new ParamTableToggle());
    // addAndMakeVisible (paramToggle.get());
    paramToggle->setClickingTogglesState (true);
    paramToggle->onClick = [this]
    {
        paramTableVisible = paramToggle->getToggleState();
        if (paramTableVisible)
        {
            paramTable->setVisible (true);
            setSize (getWidth(), getHeight() + paramTableSize);
        }
        else
        {
            paramTable->setVisible (false);
            setSize (getWidth(), getHeight() - paramTableSize);
        }
    };

    auto* const app = processor.getAppController();
    auto* const gui = app->findChild<GuiController>();

    {
        setOpaque (true);
       
        addAndMakeVisible (content = gui->getContentComponent());
        jassert (content);
        if (auto* cc = dynamic_cast<ContentComponent*> (content.getComponent()))
            cc->setExtraView (new PerfSliders (processor));

        setResizable (true, true);
        getConstrainer()->setMinimumSize (EL_PLUGIN_MIN_WIDTH, EL_PLUGIN_MIN_HEIGHT);

        const auto& bounds (processor.getEditorBounds());
        if (bounds.getWidth() > 0 && bounds.getHeight() > 0)
        {
            setSize (jmax (EL_PLUGIN_MIN_WIDTH,  bounds.getWidth()), 
                     jmax (EL_PLUGIN_MIN_HEIGHT, bounds.getHeight()));
        }
        else
        {
            setSize (EL_PLUGIN_DEFAULT_WIDTH, EL_PLUGIN_DEFAULT_HEIGHT);
        }

        setWantsPluginKeyboardFocus (processor.getEditorWantsKeyboard());
        
        gui->stabilizeContent();
        
        if (auto session = app->getWorld().getSession())
        {
            const auto graph (session->getActiveGraph());
            if (graph.isGraph())
            {
                gui->showPluginWindowsFor (graph);
                for (int w = 0; w < gui->getNumPluginWindows(); ++w)
                    if (auto* window = gui->getPluginWindow (w))
                        window->toFront (false);
            }
        }
    }

    perfParamChangedConnection = processor.onPerfParamsChanged.connect (std::bind (
        &ElementPluginAudioProcessorEditor::updatePerformanceParamEnablements, this));
}

ElementPluginAudioProcessorEditor::~ElementPluginAudioProcessorEditor()
{
    perfParamChangedConnection.disconnect();
    removeChildComponent (content.getComponent());
    content = nullptr;
    if (auto* const app = processor.getAppController())
    {
        if (auto* gui = app->findChild<GuiController>())
        {
            gui->closeAllPluginWindows();
            gui->clearContentComponent();
        }
    }
}

void ElementPluginAudioProcessorEditor::handleAsyncUpdate()
{
    // noop
}

Element::ContentComponent* ElementPluginAudioProcessorEditor::getContentComponent()
{
    if (nullptr == content)
        if (auto* app = processor.getAppController())
            if (auto* gui = app->findChild<GuiController>())
                content = gui->getContentComponent();
    return nullptr != content ? dynamic_cast<Element::ContentComponent*> (content.getComponent())
                              : 0;
}

bool ElementPluginAudioProcessorEditor::keyPressed (const KeyPress &key, Component *originatingComponent)
{
    ignoreUnused (originatingComponent);
    auto* app = processor.getAppController();
    auto& cmd (app->getWorld().getCommandManager());

    for (int i = 0; i < cmd.getNumCommands(); ++i)
    {
        const auto* info = cmd.getCommandForIndex (i);
        for (const auto& k : info->defaultKeypresses)
            if (k == key)
               return cmd.invokeDirectly (info->commandID, true);
    }
    
    return false;
}

void ElementPluginAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Element::LookAndFeel::widgetBackgroundColor.darker (0.29));
    if (! content)
    {
        g.setColour (Element::LookAndFeel::textColor);
        g.drawFittedText ("Unauthorized: Please activate your license in the application.",
                           0, 0, getWidth(), getHeight(), Justification::centred, 2);
    }
}

void ElementPluginAudioProcessorEditor::resized()
{
    auto bounds (getLocalBounds());
    processor.setEditorBounds (bounds);
    if (content)
        content->setBounds (bounds);
}

}

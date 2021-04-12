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

#include "graph/GraphNode.h"
#include "gui/GuiCommon.h"
#include "gui/NodeIOConfiguration.h"
#include "gui/MainWindow.h"

namespace Element
{

class NumberedBoxes   : public TableListBox,
                        private TableListBoxModel,
                        private Button::Listener
{
public:
    struct Listener
    {
        virtual ~Listener() {}

        virtual void addColumn()    = 0;
        virtual void removeColumn() = 0;
        virtual void columnSelected (int columnId) = 0;
    };

    enum
    {
        plusButtonColumnId  = 128,
        minusButtonColumnId = 129
    };

    NumberedBoxes (Listener& listenerToUse, bool canCurrentlyAddColumn, bool canCurrentlyRemoveColumn)
        : TableListBox ("NumberedBoxes", this),
          listener (listenerToUse),
          canAddColumn (canCurrentlyAddColumn),
          canRemoveColumn (canCurrentlyRemoveColumn)
    {
        auto& tableHeader = getHeader();

        for (int i = 0; i < 16; ++i)
            tableHeader.addColumn (String (i + 1), i + 1, 40);

        setHeaderHeight (0);
        setRowHeight (40);
        getHorizontalScrollBar().setAutoHide (false);
    }

    void setSelected (int columnId)
    {
        if (auto* button = dynamic_cast<TextButton*> (getCellComponent (columnId, 0)))
            button->setToggleState (true, NotificationType::dontSendNotification);
    }

    void setCanAddColumn (bool canCurrentlyAdd)
    {
        if (canCurrentlyAdd != canAddColumn)
        {
            canAddColumn = canCurrentlyAdd;

            if (auto* button = dynamic_cast<TextButton*> (getCellComponent (plusButtonColumnId, 0)))
                button->setEnabled (true);
        }
    }

    void setCanRemoveColumn (bool canCurrentlyRemove)
    {
        if (canCurrentlyRemove != canRemoveColumn)
        {
            canRemoveColumn = canCurrentlyRemove;

            if (auto* button = dynamic_cast<TextButton*> (getCellComponent (minusButtonColumnId, 0)))
                button->setEnabled (true);
        }
    }

private:
    int getNumRows() override                                             { return 1; }
    void paintCell (Graphics&, int, int, int, int, bool) override         {}
    void paintRowBackground (Graphics& g, int, int, int, bool) override   { g.fillAll (Colours::grey); }

    Component* refreshComponentForCell (int, int columnId, bool,
                                        Component* existingComponentToUpdate) override
    {
        auto* textButton = dynamic_cast<TextButton*> (existingComponentToUpdate);

        if (textButton == nullptr)
            textButton = new TextButton();

        textButton->setButtonText (getButtonName (columnId));
        textButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight |
                                       Button::ConnectedOnTop  | Button::ConnectedOnBottom);

        const bool isPlusMinusButton = (columnId == plusButtonColumnId || columnId == minusButtonColumnId);

        if (isPlusMinusButton)
        {
            textButton->setEnabled (columnId == plusButtonColumnId ? canAddColumn : canRemoveColumn);
        }
        else
        {
            textButton->setRadioGroupId (1, NotificationType::dontSendNotification);
            textButton->setClickingTogglesState (true);

            auto busColour = Colours::green.withRotatedHue (static_cast<float> (columnId) / 5.0f);
            textButton->setColour (TextButton::buttonColourId, busColour);
            textButton->setColour (TextButton::buttonOnColourId, busColour.withMultipliedBrightness (2.0f));
        }

        textButton->addListener (this);

        return textButton;
    }

    String getButtonName (int columnId)
    {
        if (columnId == plusButtonColumnId)  return "+";
        if (columnId == minusButtonColumnId) return "-";

        return String (columnId);
    }

    void buttonClicked (Button* btn) override
    {
        auto text = btn->getButtonText();

        if (text == "+") listener.addColumn();
        if (text == "-") listener.removeColumn();
    }

    void buttonStateChanged (Button* btn) override
    {
        auto text = btn->getButtonText();

        if (text == "+" || text == "-")
            return;

        if (btn->getToggleState())
            listener.columnSelected (text.getIntValue());
    }

    Listener& listener;
    bool canAddColumn, canRemoveColumn;
};

class NodeAudioBusesComponent::InputOutputConfig  : public Component,
                                                        private ComboBox::Listener,
                                                        private Button::Listener,
                                                        private NumberedBoxes::Listener
{
public:
    InputOutputConfig (NodeAudioBusesComponent& parent, bool direction)
        : owner (parent),
          ioTitle ("ioLabel", direction ? "Input Configuration" : "Output Configuration"),
          nameLabel ("nameLabel", "Bus Name:"),
          layoutLabel ("layoutLabel", "Channel Layout:"),
          enabledToggle ("Enabled"),
          ioBuses (*this, false, false),
          isInput (direction),
          currentBus (0)
    {
        ioTitle.setFont (ioTitle.getFont().withStyle (Font::bold));
        nameLabel.setFont (nameLabel.getFont().withStyle (Font::bold));
        layoutLabel.setFont (layoutLabel.getFont().withStyle (Font::bold));
        enabledToggle.setClickingTogglesState (true);

        layouts.addListener (this);
        enabledToggle.addListener (this);

        addAndMakeVisible (layoutLabel);
        addAndMakeVisible (layouts);
        addAndMakeVisible (enabledToggle);
        addAndMakeVisible (ioTitle);
        addAndMakeVisible (nameLabel);
        addAndMakeVisible (name);
        addAndMakeVisible (ioBuses);

        updateBusButtons();
        updateBusLayout();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);

        ioTitle.setBounds (r.removeFromTop (14));
        r.reduce (10, 0);
        r.removeFromTop (16);

        ioBuses.setBounds (r.removeFromTop (60));

        {
            auto label = r.removeFromTop (24);

            nameLabel.setBounds (label.removeFromLeft (100));
            enabledToggle.setBounds (label.removeFromRight (80));
            name.setBounds (label);
        }

        {
            auto label = r.removeFromTop (24);

            layoutLabel.setBounds (label.removeFromLeft (100));
            layouts.setBounds (label);
        }
    }

private:
    void updateBusButtons()
    {
        if (auto* filter = owner.getAudioProcessor())
        {
            auto& header = ioBuses.getHeader();
            header.removeAllColumns();

            const int n = filter->getBusCount (isInput);

            for (int i = 0; i < n; ++i)
                header.addColumn ("", i + 1, 40);

            header.addColumn ("+", NumberedBoxes::plusButtonColumnId,  20);
            header.addColumn ("-", NumberedBoxes::minusButtonColumnId, 20);

            ioBuses.setCanAddColumn    (filter->canAddBus    (isInput));
            ioBuses.setCanRemoveColumn (filter->canRemoveBus (isInput));
        }

        ioBuses.setSelected (currentBus + 1);
    }

    void updateBusLayout()
    {
        if (auto* filter = owner.getAudioProcessor())
        {
            if (auto* bus = filter->getBus (isInput, currentBus))
            {
                name.setText (bus->getName(), NotificationType::dontSendNotification);

                int i;
                for (i = 1; i < AudioChannelSet::maxChannelsOfNamedLayout; ++i)
                    if ((layouts.indexOfItemId(i) == -1) != bus->supportedLayoutWithChannels (i).isDisabled())
                        break;

                // supported layouts have changed
                if (i < AudioChannelSet::maxChannelsOfNamedLayout)
                {
                    layouts.clear();

                    for (i = 1; i < AudioChannelSet::maxChannelsOfNamedLayout; ++i)
                    {
                        auto set = bus->supportedLayoutWithChannels (i);

                        if (! set.isDisabled())
                            layouts.addItem (set.getDescription(), i);
                    }
                }

                const auto& channelSet = isInput ? owner.currentLayout.inputBuses.getReference(currentBus)
                                                 : owner.currentLayout.outputBuses.getReference(currentBus);
                
                layouts.setSelectedId (channelSet.size());
                
                const bool canBeDisabled = bus->isNumberOfChannelsSupported (0);

                if (canBeDisabled != enabledToggle.isEnabled())
                    enabledToggle.setEnabled (canBeDisabled);

                enabledToggle.setToggleState (bus->isEnabled(), NotificationType::dontSendNotification);
            }            
        }
    }

    void updateOthers()
    {
        updateBusLayout();

        if (InputOutputConfig* config = owner.getConfig (! isInput))
            config->updateBusLayout();

        owner.update();
    }

    //==============================================================================
    void comboBoxChanged (ComboBox* combo) override
    {
        if (combo == &layouts)
        {
            if (auto* audioProcessor = owner.getAudioProcessor())
            {
                if (auto* bus = audioProcessor->getBus (isInput, currentBus))
                {
                    auto selectedNumChannels = layouts.getSelectedId();

                    if (selectedNumChannels != bus->getLastEnabledLayout().size())
                    {
                        if (isPositiveAndBelow (selectedNumChannels, AudioChannelSet::maxChannelsOfNamedLayout))
                        {
                            auto& currentSet = isInput ? owner.currentLayout.inputBuses 
                                                       : owner.currentLayout.outputBuses;
                            currentSet.getReference (currentBus) = bus->supportedLayoutWithChannels (selectedNumChannels);
                            updateOthers();
                        }
#if 0
                        if (isPositiveAndBelow (selectedNumChannels, AudioChannelSet::maxChannelsOfNamedLayout)
                             && bus->setCurrentLayoutWithoutEnabling (bus->supportedLayoutWithChannels (selectedNumChannels)))
                        {
                            if (auto* config = owner.getConfig (! isInput))
                                config->updateBusLayout();

                            owner.update();
                        }
                        #endif
                    }
                }
            }
        }
    }

    void buttonClicked (Button*) override {}

    void buttonStateChanged (Button* btn) override
    {
        if (btn == &enabledToggle && enabledToggle.isEnabled())
        {
            if (auto* audioProcessor = owner.getAudioProcessor())
            {
                if (auto* bus = audioProcessor->getBus (isInput, currentBus))
                {
                    if (bus->isEnabled() != enabledToggle.getToggleState())
                    {
                        bool success = false;
#if 0
                        if (enabledToggle.getToggleState())
                            success = bus->enable();
                        else
                            success = bus->setCurrentLayout (AudioChannelSet::disabled());
#endif
                        if (success)
                        {
                            updateOthers();
                        }
                        else
                        {
                            enabledToggle.setToggleState (! enabledToggle.getToggleState(),
                                                          NotificationType::dontSendNotification);
                        }
                    }
                }
            }
        }
    }

    //==============================================================================
    void addColumn() override
    {
        if (auto* audioProcessor = owner.getAudioProcessor())
        {
            if (audioProcessor->canAddBus (isInput))
            {
                if (audioProcessor->addBus (isInput))
                {
                    updateBusButtons();
                    updateBusLayout();

                    if (auto* config = owner.getConfig (! isInput))
                    {
                        config->updateBusButtons();
                        config->updateBusLayout();
                    }

                    owner.update();
                }
            }
        }
    }

    void removeColumn() override
    {
        if (auto* audioProcessor = owner.getAudioProcessor())
        {
            if (audioProcessor->getBusCount (isInput) > 1 && audioProcessor->canRemoveBus (isInput))
            {
                if (audioProcessor->removeBus (isInput))
                {
                    currentBus = jmin (audioProcessor->getBusCount (isInput) - 1, currentBus);

                    updateBusButtons();
                    updateBusLayout();

                    if (auto* config = owner.getConfig (! isInput))
                    {
                        config->updateBusButtons();
                        config->updateBusLayout();
                    }

                    owner.update();
                }
            }
        }
    }

    void columnSelected (int columnId) override
    {
        const int newBus = columnId - 1;

        if (currentBus != newBus)
        {
            currentBus = newBus;
            ioBuses.setSelected (currentBus + 1);
            updateBusLayout();
        }
    }

    //==============================================================================
    NodeAudioBusesComponent& owner;
    Label ioTitle, nameLabel, name, layoutLabel;
    ToggleButton enabledToggle;
    ComboBox layouts;
    NumberedBoxes ioBuses;
    bool isInput;
    int currentBus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputOutputConfig)
};


NodeAudioBusesComponent::NodeAudioBusesComponent (const Node& n, AudioProcessor* const p,
                                                  ContentComponent* cc)
    : AudioProcessorEditor (p),
      content (cc),
      node (n),
      title ("title", p->getName())
{
    jassert (p != nullptr);
    jassert (nullptr != n.getObject());
    jassert (p == n.getObject()->getAudioProcessor());
    currentLayout = p->getBusesLayout();
    
    setOpaque (true);

    title.setFont (title.getFont().withStyle (Font::bold));
    addAndMakeVisible (title);

    if (p->getBusCount (true)  > 0)// || p->canAddBus (true))
        addAndMakeVisible (inConfig = new InputOutputConfig (*this, true));

    if (p->getBusCount (false) > 0)// || p->canAddBus (false))
        addAndMakeVisible (outConfig = new InputOutputConfig (*this, false));

    
    addAndMakeVisible(saveButton);
    saveButton.setButtonText ("Save");
    saveButton.addListener (this);

    addAndMakeVisible(cancelButton);
    cancelButton.setButtonText ("Cancel");
    cancelButton.addListener (this);

    setSize (400, (inConfig != nullptr && outConfig != nullptr ? 160 : 0) + 226);
}

NodeAudioBusesComponent::~NodeAudioBusesComponent() { }

void NodeAudioBusesComponent::buttonClicked (Button* b)
{
    if (b == &saveButton)
    {
        bool posted = false;

        if (auto* cc = getContentComponent())
        {
            cc->post (new ChangeBusesLayout (node, currentLayout));
            posted = true;
        }

        if (! posted)
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, node.getName(), "Could request update of audio buses.");
        }
    }

    if (auto* co = findParentComponentOfClass<CallOutBox> ())
        co->dismiss();
}

void NodeAudioBusesComponent::paint (Graphics& g)
{
     g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void NodeAudioBusesComponent::resized()
{
    auto r = getLocalBounds().reduced (10);
    auto r2 = r.removeFromBottom (22);

    title.setBounds (r.removeFromTop (14));
    r.reduce (10, 0);

    if (inConfig != nullptr)
        inConfig->setBounds (r.removeFromTop (160));

    if (outConfig != nullptr)
        outConfig->setBounds (r.removeFromTop (160));
    
    cancelButton.changeWidthToFitText (22);
    cancelButton.setBounds (r2.removeFromRight (cancelButton.getWidth()));

    r2.removeFromRight (4);
    saveButton.changeWidthToFitText (22);
    saveButton.setBounds (r2.removeFromRight (saveButton.getWidth()));
}

void NodeAudioBusesComponent::update() { }

int32 NodeAudioBusesComponent::getNodeId() const
{
    if (auto* graph = getGraph())
    {
        const int n = graph->getNumNodes();

        for (int i = 0; i < n; ++i)
            if (auto* node = graph->getNode (i))
                if (node->getAudioProcessor() == getAudioProcessor())
                    return static_cast<int32> (node->nodeId);
    }

    return -1;
}

ContentComponent* NodeAudioBusesComponent::getContentComponent()
{
   if (! content)
   {
       Component* comp;
       for (int idx = 0; (comp = Desktop::getInstance().getComponent (idx)) != nullptr; ++idx)
       {
           if (auto* window = dynamic_cast<MainWindow*> (comp))
               content = dynamic_cast<ContentComponent*> (window->getContentComponent());
           if (content != nullptr)
               break;
        }
   }
   return content;
}

GraphEditorComponent* NodeAudioBusesComponent::getGraphEditor() const
{
    return nullptr;
}

GraphNode* NodeAudioBusesComponent::getGraph() const
{
    const Node graph (node.getParentGraph());
    if (auto* gn = graph.getObject())
        return dynamic_cast<GraphNode*> (gn);
    return nullptr;
}

}


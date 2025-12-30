/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <element/audioengine.hpp>
#include <element/context.hpp>
#include <element/services.hpp>
#include <element/ui/mainwindow.hpp>

#include "ui/grapheditorcomponent.hpp"
#include "messages.hpp"
#include "ioconfigurationwindow.hpp"
#include "engine/graphnode.hpp"

#if 0
#include <JuceHeader.h>
#include "../UI/GraphEditorPanel.h"
#include "InternalPlugins.h"
#include "../UI/MainWindow.h"
#include "IOConfigurationWindow.h"
#endif

namespace element {

using namespace juce;

//==============================================================================
struct NumberedBoxes final : public TableListBox,
                             private TableListBoxModel,
                             private Button::Listener
{
    struct Listener
    {
        virtual ~Listener() {}

        virtual void addColumn() = 0;
        virtual void removeColumn() = 0;
        virtual void columnSelected (int columnId) = 0;
    };

    enum
    {
        plusButtonColumnId = 128,
        minusButtonColumnId = 129
    };

    //==============================================================================
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
    //==============================================================================
    Listener& listener;
    bool canAddColumn, canRemoveColumn;

    //==============================================================================
    int getNumRows() override { return 1; }
    void paintCell (Graphics&, int, int, int, int, bool) override {}
    void paintRowBackground (Graphics& g, int, int, int, bool) override { g.fillAll (Colours::grey); }

    Component* refreshComponentForCell (int, int columnId, bool, Component* existingComponentToUpdate) override
    {
        auto* textButton = dynamic_cast<TextButton*> (existingComponentToUpdate);

        if (textButton == nullptr)
            textButton = new TextButton();

        textButton->setButtonText (getButtonName (columnId));
        textButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);

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

    //==============================================================================
    String getButtonName (int columnId)
    {
        if (columnId == plusButtonColumnId)
            return "+";
        if (columnId == minusButtonColumnId)
            return "-";

        return String (columnId);
    }

    void buttonClicked (Button* btn) override
    {
        auto text = btn->getButtonText();

        if (text == "+")
            listener.addColumn();
        if (text == "-")
            listener.removeColumn();
    }

    void buttonStateChanged (Button* btn) override
    {
        auto text = btn->getButtonText();

        if (text == "+" || text == "-")
            return;

        if (btn->getToggleState())
            listener.columnSelected (text.getIntValue());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NumberedBoxes)
};

//==============================================================================
class IOConfigurationWindow::InputOutputConfig final : public Component,
                                                       private Button::Listener,
                                                       private NumberedBoxes::Listener
{
public:
    InputOutputConfig (IOConfigurationWindow& parent, bool direction)
        : owner (parent),
          ioTitle ("ioLabel", direction ? "Input Configuration" : "Output Configuration"),
          ioBuses (*this, false, false),
          isInput (direction)
    {
        ioTitle.setFont (ioTitle.getFont().withStyle (Font::bold));
        nameLabel.setFont (nameLabel.getFont().withStyle (Font::bold));
        layoutLabel.setFont (layoutLabel.getFont().withStyle (Font::bold));
        enabledToggle.setClickingTogglesState (true);

        // TODO: Handle toggled state in the GUI. Until then, don't show the toggle.
        enabledToggle.setVisible (false);

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
        if (auto* plugin = owner.getAudioProcessor())
        {
            auto& header = ioBuses.getHeader();
            header.removeAllColumns();

            const int n = plugin->getBusCount (isInput);

            for (int i = 0; i < n; ++i)
                header.addColumn ("", i + 1, 40);

            header.addColumn ("+", NumberedBoxes::plusButtonColumnId, 20);
            header.addColumn ("-", NumberedBoxes::minusButtonColumnId, 20);

            ioBuses.setCanAddColumn (plugin->canAddBus (isInput));
            ioBuses.setCanRemoveColumn (plugin->canRemoveBus (isInput));
        }

        ioBuses.setSelected (currentBus + 1);
    }

    void updateBusLayout()
    {
        if (auto* plugin = owner.getAudioProcessor())
        {
            if (auto* bus = plugin->getBus (isInput, currentBus))
            {
                name.setText (bus->getName(), NotificationType::dontSendNotification);

                // supported layouts have changed
                layouts.clear (dontSendNotification);
                auto* menu = layouts.getRootMenu();

                auto itemId = 1;
                auto selectedId = -1;

                for (auto i = 1; i <= AudioChannelSet::maxChannelsOfNamedLayout; ++i)
                {
                    for (const auto& set : AudioChannelSet::channelSetsWithNumberOfChannels (i))
                    {
                        if (bus->isLayoutSupported (set))
                        {
                            menu->addItem (PopupMenu::Item { set.getDescription() }
                                               .setAction ([this, set] { applyBusLayout (set); })
                                               .setID (itemId));
                        }

                        if (bus->getCurrentLayout() == set)
                            selectedId = itemId;

                        ++itemId;
                    }
                }

                layouts.setSelectedId (selectedId);

                const bool canBeDisabled = bus->isNumberOfChannelsSupported (0);

                if (canBeDisabled != enabledToggle.isEnabled())
                    enabledToggle.setEnabled (canBeDisabled);

                enabledToggle.setToggleState (bus->isEnabled(), NotificationType::dontSendNotification);
            }
        }
    }

    //==============================================================================
    void applyBusLayout (const AudioChannelSet& set)
    {
        using Layout = AudioProcessor::BusesLayout;
        auto findMatchingAudioSet = [&] (AudioProcessor& proc) -> AudioChannelSet {
            AudioChannelSet set;

            return set;
        };

        auto* p = owner.getAudioProcessor();
        auto* bus = p != nullptr ? p->getBus (isInput, currentBus) : nullptr;
        if (p == nullptr || bus == nullptr)
            return;

        auto newLayout = owner.currentLayout;
        auto& buses = isInput ? newLayout.inputBuses : newLayout.outputBuses;
        auto& otherBuses = ! isInput ? newLayout.inputBuses : newLayout.outputBuses;
        buses.set (currentBus, set);

        owner.currentLayout = newLayout;
        owner.stabilize();
#if 0
        if (auto* p = owner.getAudioProcessor())
        {
            if (auto* bus = p->getBus (isInput, currentBus))
            {
                if (bus->setCurrentLayoutWithoutEnabling (set))
                {
                    if (auto* config = owner.getConfig (! isInput))
                        config->updateBusLayout();

                    owner.update();
                }
            }
        }
#endif
    }

    void buttonClicked (Button*) override {}

    void buttonStateChanged (Button* btn) override
    {
        if (btn == &enabledToggle && enabledToggle.isEnabled())
        {
            if (auto* p = owner.getAudioProcessor())
            {
                if (auto* bus = p->getBus (isInput, currentBus))
                {
                    if (bus->isEnabled() != enabledToggle.getToggleState())
                    {
                        bool success = enabledToggle.getToggleState() ? bus->enable()
                                                                      : bus->setCurrentLayout (AudioChannelSet::disabled());

                        if (success)
                        {
                            updateBusLayout();

                            if (auto* config = owner.getConfig (! isInput))
                                config->updateBusLayout();

                            owner.update();
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
        if (auto* p = owner.getAudioProcessor())
        {
            if (p->canAddBus (isInput))
            {
                if (p->addBus (isInput))
                {
                    updateBusButtons();
                    updateBusLayout();

                    if (auto* config = owner.getConfig (! isInput))
                    {
                        config->updateBusButtons();
                        config->updateBusLayout();
                    }
                }

                owner.update();
            }
        }
    }

    void removeColumn() override
    {
        if (auto* p = owner.getAudioProcessor())
        {
            if (p->getBusCount (isInput) > 1 && p->canRemoveBus (isInput))
            {
                if (p->removeBus (isInput))
                {
                    currentBus = jmin (p->getBusCount (isInput) - 1, currentBus);

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
    IOConfigurationWindow& owner;
    Label ioTitle, name;
    Label nameLabel { "nameLabel", "Bus Name:" };
    Label layoutLabel { "layoutLabel", "Channel Layout:" };
    ToggleButton enabledToggle { "Enabled" };
    ComboBox layouts;
    NumberedBoxes ioBuses;
    bool isInput;
    int currentBus = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputOutputConfig)
};

IOConfigurationWindow::IOConfigurationWindow (Context& ctx, const Node& n, AudioProcessor& p)
    : AudioProcessorEditor (&p),
      title ("title", p.getName()),
      _context (ctx),
      _node (n)
{
    setOpaque (true);

    title.setFont (title.getFont().withStyle (Font::bold));
    addAndMakeVisible (title);

    {
        // ScopedLock renderLock (p.getCallbackLock());
        // p.suspendProcessing (true);
        // p.releaseResources();
    }

    if (p.getBusCount (true) > 0 || p.canAddBus (true))
    {
        inConfig.reset (new InputOutputConfig (*this, true));
        addAndMakeVisible (inConfig.get());
    }

    if (p.getBusCount (false) > 0 || p.canAddBus (false))
    {
        outConfig.reset (new InputOutputConfig (*this, false));
        addAndMakeVisible (outConfig.get());
    }

    applyButton.onClick = [this] { applyButtonClicked(); };
    addAndMakeVisible (applyButton);

    currentLayout = p.getBusesLayout();
    setSize (400, (inConfig != nullptr && outConfig != nullptr ? 160 : 0) + 240);
}

IOConfigurationWindow::~IOConfigurationWindow()
{
    if (auto* graph = getGraph())
    {
        if (auto* p = getAudioProcessor())
        {
            // ScopedLock renderLock (graph->getPropertyLock());

            // graph->suspendProcessing (true);
            // graph->releaseResources();

            // p->prepareToPlay (graph->getSampleRate(), graph->getBlockSize());
            // p->suspendProcessing (false);

            // graph->prepareToRender (graph->getSampleRate(), graph->getBlockSize());
            // graph->suspendProcessing (false);
        }
    }
}

void IOConfigurationWindow::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void IOConfigurationWindow::resized()
{
    auto r = getLocalBounds().reduced (10);

    title.setBounds (r.removeFromTop (14));
    r.reduce (10, 0);

    if (inConfig != nullptr)
        inConfig->setBounds (r.removeFromTop (160));

    if (outConfig != nullptr)
        outConfig->setBounds (r.removeFromTop (160));

    r.removeFromTop (10);
    applyButton.setBounds (r.removeFromTop (24).removeFromRight (100));
}

void IOConfigurationWindow::update()
{
#if 0
    auto nodeID = getNodeID();

    if (auto* graph = getGraph())
        if (nodeID != AudioProcessorGraph::NodeID())
            graph->disconnectNode (nodeID);


    // FIXME:
    if (auto* graphEditor = getGraphEditor())
        if (auto* panel = graphEditor->graphPanel.get())
            panel->updateComponents();
#endif
}

void IOConfigurationWindow::stabilize()
{
    if (auto* p = getAudioProcessor())
    {
        applyButton.setEnabled (p->checkBusesLayoutSupported (currentLayout));
    }
}

uint32_t IOConfigurationWindow::getNodeID() const
{
    if (auto* graph = getGraph())
    {
        for (int i = 0; i < graph->getNumNodes(); ++i)
        {
            auto* n = graph->getNode (i);
            if (n->getAudioProcessor() == getAudioProcessor())
                return n->nodeId;
        }
    }

    return {};
}

MainWindow* IOConfigurationWindow::getMainWindow() const
{
    auto& desktop = Desktop::getInstance();

    for (int i = desktop.getNumComponents(); --i >= 0;)
        if (auto* mainWindow = dynamic_cast<MainWindow*> (desktop.getComponent (i)))
            return mainWindow;

    return nullptr;
}

GraphEditorComponent* IOConfigurationWindow::getGraphEditor() const
{
    // FIXME:
    return nullptr;
}

GraphNode* IOConfigurationWindow::getGraph() const
{
    const auto graph = _node.getParentGraph();
    return dynamic_cast<GraphNode*> (graph.getObject());
}

void IOConfigurationWindow::applyButtonClicked()
{
    auto msg = new ChangeBusesLayout (_node, currentLayout);
    Component::SafePointer<IOConfigurationWindow> safeThis (this);
    msg->onFinished = [safeThis]() {
        if (auto* c = safeThis.getComponent())
        {
            if (auto* callOut = c->findParentComponentOfClass<CallOutBox>())
                callOut->dismiss();
            else
                c->stabilize();
        }
    };
    _context.services().postMessage (msg);
}

} // namespace element

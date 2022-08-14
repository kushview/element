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

#include "controllers/GuiController.h"
#include "gui/Buttons.h"
#include "gui/views/ControllerDevicesView.h"
#include "gui/ViewHelpers.h"
#include "session/controllerdevice.hpp"
#include "messages.hpp"
#include "globals.hpp"

namespace Element {

class ControllerMapsTable : public TableListBox,
                            public TableListBoxModel
{
public:
    enum Columns
    {
        Device = 1,
        Control,
        Node,
        Parameter
    };

    ControllerMapsTable()
    {
        setModel (this);
        const int flags = TableHeaderComponent::notSortable;
        // getHeader().addColumn ("Device", Device, 100, 30, -1, flags);
        getHeader().addColumn ("Node", Node, 100, 30, -1, flags);
        getHeader().addColumn ("Control", Control, 100, 30, -1, flags);
        getHeader().addColumn ("Parameter", Parameter, 100, 30, -1, flags);
        setHeaderHeight (22);
        setRowHeight (20);
    }

    ~ControllerMapsTable()
    {
        setModel (nullptr);
    }

    void clear()
    {
        maps.clearQuick (true);
        updateContent();
    }

    void refreshContent (const ControllerDevice& device = ControllerDevice(),
                         const ControllerDevice::Control& control = ControllerDevice::Control())
    {
        maps.clear (true);

        if (session)
        {
            for (int i = 0; i < session->getNumControllerMaps(); ++i)
            {
                std::unique_ptr<ControllerMapObjects> objects;
                objects.reset (new ControllerMapObjects (session, session->getControllerMap (i)));
                if (! device.isValid() || (device.isValid() && device.getUuidString() == objects->device.getUuidString()))
                {
                    maps.add (objects.release());
                }
            }
        }

        updateContent();
        repaint();
    }

    void updateWith (SessionPtr sess,
                     const ControllerDevice& device = ControllerDevice(),
                     const ControllerDevice::Control& control = ControllerDevice::Control())
    {
        session = sess;
        refreshContent (device, control);
    }

    int getNumRows() override { return maps.size(); }

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        ViewHelpers::drawBasicTextRow ("", g, width, height, rowIsSelected);
    }

    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        auto* const objects = maps[rowNumber];
        if (! objects)
            return;

        const auto mapp (objects->controllerMap);
        const auto device (objects->device);
        const auto control (objects->control);
        const auto node (objects->node);
        g.setColour (objects->isValid() ? LookAndFeel::textColor : Colours::red);

        String text = "N/A";
        switch (columnId)
        {
            case Device: {
                text = device.getName().toString();
            }
            break;

            case Control: {
                text = control.getName().toString();
            }
            break;

            case Node: {
                text = node.getName();
            }
            break;

            case Parameter: {
                text = "Parameter ";
                text << mapp.getParameterIndex();

                if (NodeObject::isSpecialParameter (mapp.getParameterIndex()))
                {
                    text = NodeObject::getSpecialParameterName (mapp.getParameterIndex());
                }
                else if (auto* obj = node.getObject())
                {
                    if (auto* proc = obj->getAudioProcessor())
                        if (auto* param = proc->getParameters()[mapp.getParameterIndex()])
                            text = param->getName (64);
                }
            }
            break;
        }

        g.drawText (text, 0, 0, width, height, Justification::centredLeft);
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
        if (auto* objects = maps[lastRowSelected])
        {
            ViewHelpers::postMessageFor (this, new RemoveControllerMapMessage (objects->controllerMap));
        }
    }

    void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
        if (auto* const objects = maps[rowNumber])
        {
            switch (columnId)
            {
                case Node:
                    ViewHelpers::presentPluginWindow (this, objects->node);
                    break;
                default:
                    break;
            }
        }
    }

    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected, Component* existing) override
    {
        return nullptr;
#if 0
        CellContent* const content = nullptr == existing 
            ? new CellContent() : dynamic_cast<CellContent*> (existing);
        jassert (content);
        
        content->columnId = columnId;
        content->rowNumber = rowNumber;
        if (auto* const objects = maps [rowNumber])
            content->mapp = *objects;
        else
            content->mapp = ControllerMapObjects();
        content->stabilize();
        return content;
#endif
    }

#if 0
    
    virtual void cellClicked (int rowNumber, int columnId, const MouseEvent&);
    
    virtual void backgroundClicked (const MouseEvent&);
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);
    virtual int getColumnAutoSizeWidth (int columnId);
    virtual String getCellTooltip (int rowNumber, int columnId);
    virtual void selectedRowsChanged (int lastRowSelected);
    
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows);
#endif
private:
    SessionPtr session;
    OwnedArray<ControllerMapObjects> maps;

    class CellContent : public Component
    {
    public:
        CellContent()
        {
            setInterceptsMouseClicks (false, true);
            addAndMakeVisible (comboBox);
        }

        void resized() override
        {
            comboBox.setBounds (getLocalBounds().reduced (2));
        }

        void stabilize()
        {
            comboBox.clear();
            comboBox.addItem ("Item 1", 1);
            comboBox.addItem ("Item 2", 2);
            comboBox.addItem ("Item 3", 3);
        }

    private:
        friend class ControllerMapsTable;
        ComboBox comboBox;
        int columnId = 0;
        int rowNumber = -1;
        ControllerMapObjects mapp;
    };
};

class MidiLearnButton : public SettingButton,
                        public MidiInputCallback,
                        public AsyncUpdater,
                        public Button::Listener
{
public:
    Signal<void()> messageReceived;

    explicit MidiLearnButton (const String& deviceName = String())
    {
        addListener (this);
    }

    ~MidiLearnButton()
    {
        stopListening();
    }

    void buttonClicked (Button*) override {}

    bool isListening() const { return listening; }

    void updateToggleState()
    {
        setToggleState (isListening(), dontSendNotification);
    }

    void setInputDevice (const String& deviceName)
    {
        if (inputName == deviceName)
            return;
        const bool wasListening = isListening();
        stopListening();
        inputName = deviceName;
        if (wasListening)
            startListening();
    }

    void stopListening()
    {
#if 0
        if (input)
        {
            input->stop();
            input.reset (nullptr);
        }
#else
        jassert (ViewHelpers::getGlobals (this));
        if (auto* world = ViewHelpers::getGlobals (this))
            world->getMidiEngine().removeMidiInputCallback (this);
#endif

        listening = false;
        updateToggleState();
    }

    void startListening()
    {
        if (inputName.isEmpty())
            return;

        stopListening();
        clearMessage();

        jassert (ViewHelpers::getGlobals (this));
        if (auto* world = ViewHelpers::getGlobals (this))
            world->getMidiEngine().addMidiInputCallback (inputName, this, true);

        listening = true;
        updateToggleState();
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& msg) override
    {
        if (gotFirstMessage.get() && stopOnFirstMessage.get())
            return;
        gotFirstMessage.set (true);
        ScopedLock sl (lock);
        message = msg;
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        if (gotFirstMessage.get() && stopOnFirstMessage.get())
            stopListening();
        messageReceived();
    }

    MidiMessage getMidiMessage() const
    {
        ScopedLock sl (lock);
        return message;
    }

private:
    void clearMessage()
    {
        gotFirstMessage.set (false);
        ScopedLock sl (lock);
        message = MidiMessage();
    }

    CriticalSection lock;
    bool listening = false;
    Atomic<bool> gotFirstMessage = false;
    Atomic<bool> stopOnFirstMessage = false;
    MidiMessage message;
    String inputName;
};

class ControlListBox : public ListBox,
                       public ListBoxModel
{
public:
    std::function<void()> selectionChanged;

    ControlListBox()
    {
        setModel (this);
        setRowHeight (32);
    }

    ~ControlListBox()
    {
        setModel (nullptr);
    }

    void setControllerDevice (const ControllerDevice& dev)
    {
        editedDevice = dev;
        updateContent();
        repaint();
    }

    ControllerDevice::Control getSelectedControl() const
    {
        if (getNumSelectedRows() > 0 && getSelectedRow() < editedDevice.getNumControls())
            return editedDevice.getControl (getSelectedRow());
        return ControllerDevice::Control();
    }

    int getNumRows() override
    {
        return editedDevice.getNumChildren();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        ignoreUnused (rowNumber, g, width, height, rowIsSelected);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        auto* row = dynamic_cast<ControllerRow*> (existingComponentToUpdate);
        if (row == nullptr)
            row = new ControllerRow (*this);

        row->refresh (editedDevice.getControl (rowNumber),
                      rowNumber,
                      isRowSelected);

        return row;
    }

    void listBoxItemClicked (int row, const MouseEvent&) override { DBG ("clicked"); }
    void selectedRowsChanged (int lastRowSelected) override
    {
        if (selectionChanged)
            selectionChanged();
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
        const auto selected (ControllerDevice::Control (
            editedDevice.getControl (lastRowSelected)));
        ViewHelpers::postMessageFor (this, new RemoveControlMessage (editedDevice, selected));
    }

#if 0
    void listBoxItemClicked (int row, const MouseEvent&) override { }
    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { }
    void backgroundClicked (const MouseEvent&) override { }
    void selectedRowsChanged (int lastRowSelected) override { }
    
    void returnKeyPressed (int lastRowSelected) override { }
    void listWasScrolled() override { }
    var getDragSourceDescription (const SparseSet<int>& rowsToDescribe) override { }
    String getTooltipForRow (int row) override { }
    MouseCursor getMouseCursorForRow (int row) override { }
#endif

private:
    ControllerDevice editedDevice;

    class ControllerRow : public Component,
                          public Button::Listener
    {
    public:
        ControllerRow (ControlListBox& l) : list (l)
        {
            status.setJustificationType (Justification::centredRight);
            status.setColour (Label::textColourId, Element::LookAndFeel::textColor.darker());
            status.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (status);
        }

        void buttonClicked (Button*) override
        {
            list.selectRow (rowNumber, true, true);
            setSelected (list.getSelectedRow() == rowNumber);
        }

        void mouseDown (const MouseEvent&) override
        {
            list.selectRow (rowNumber, true);
            setSelected (list.getSelectedRow() == rowNumber);
        }

        void paint (Graphics& g) override
        {
            ViewHelpers::drawBasicTextRow (control.getName().toString(),
                                           g,
                                           getWidth(),
                                           getHeight(),
                                           selected);
        }

        void resized() override
        {
            auto r (getLocalBounds().reduced (8));
            status.setBounds (r.removeFromRight (getWidth() / 2));
        }

        void refresh (const ControllerDevice::Control& ctl, int row, bool isNowSelected)
        {
            control = ctl;
            rowNumber = row;
            setSelected (isNowSelected);

            String text = "N/A";
            if (control.isNoteEvent())
            {
                text = MidiMessage::getMidiNoteName (control.getEventId(), true, true, 4);
            }
            else if (control.isControllerEvent())
            {
                text = "CC ";
                text << control.getEventId();
            }

            status.setText (text, dontSendNotification);
            list.repaintRow (rowNumber);
        }

        void setSelected (const bool isNowSelected)
        {
            if (selected == isNowSelected)
                return;
            selected = isNowSelected;
        }

        ControllerDevice::Control control;
        int rowNumber = -1;
        bool selected = false;
        Label status;

    private:
        ControlListBox& list;
    };
};

class ControllerDevicesView::Content : public Component,
                                       public Button::Listener,
                                       public ComboBox::Listener,
                                       public Value::Listener,
                                       public AsyncUpdater
{
public:
    Content()
    {
        controllersBox.setTextWhenNoChoicesAvailable ("No Controllers");
        controllersBox.setTextWhenNothingSelected ("(Controllers)");
        controllersBox.addListener (this);
        addAndMakeVisible (controllersBox);

        createButton.setButtonText ("+");
        createButton.setTooltip ("Add a new controller device");
        createButton.addListener (this);
        addAndMakeVisible (createButton);

        deleteButton.setButtonText ("-");
        deleteButton.setTooltip ("Remove the current controller device");
        deleteButton.addListener (this);
        addAndMakeVisible (deleteButton);

        controls.setControllerDevice (editedDevice);
        controls.selectionChanged = std::bind (&Content::triggerAsyncUpdate, this);
        addAndMakeVisible (controls);

        addControlButton.setButtonText ("+");
        addControlButton.setTooltip ("Add a new control");
        addControlButton.addListener (this);
        addAndMakeVisible (addControlButton);

        learnButton.setButtonText ("Learn");
        learnButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);
        learnButton.setTooltip ("Learn MIDI");
        learnButton.messageReceived.connect (
            std::bind (&Content::onLearnMidi, this));
        learnButton.addListener (this);
        addAndMakeVisible (learnButton);

        removeControlButton.setButtonText ("-");
        removeControlButton.setTooltip ("Remove the selected control");
        addAndMakeVisible (removeControlButton);
        removeControlButton.addListener (this);
        addAndMakeVisible (properties);

        addAndMakeVisible (saveControllerButton);
        saveControllerButton.setTooltip ("Save a controller to disk");
        saveControllerButton.setPath (getIcons().fasSave, 4.2);
        saveControllerButton.addListener (this);

        addAndMakeVisible (openControllerButton);
        openControllerButton.setTooltip ("Open a controller from disk");
        openControllerButton.setPath (getIcons().fasFolderOpen, 3.0);
        openControllerButton.addListener (this);

        addAndMakeVisible (maps);

        deviceName.addListener (this);
        inputDevice.addListener (this);
        controlName.addListener (this);
        eventId.addListener (this);
        eventType.addListener (this);
        toggleMode.addListener (this);
        momentary.addListener (this);

        triggerAsyncUpdate();
    }

    ~Content() noexcept
    {
        disconnectHandlers();
        session = nullptr;
        controls.selectionChanged = nullptr;
        learnButton.messageReceived.disconnect_all_slots();
        deviceName.removeListener (this);
    }

    void handleAsyncUpdate() override { stabilizeContent(); }

    static bool supportedForMapping (const MidiMessage& message, const ControllerDevice::Control& control)
    {
        ignoreUnused (control);
        return (message.isController() || message.isNoteOn())
               && message.getRawDataSize() > 0;
    }

    void onLearnMidi()
    {
        const auto message (learnButton.getMidiMessage());
        const auto control (controls.getSelectedControl());
        if (supportedForMapping (message, control))
        {
            const var mappingData ((void*) message.getRawData(),
                                   (size_t) message.getRawDataSize());
            ValueTree data = control.getValueTree();
            data.setProperty (Tags::mappingData, mappingData, nullptr);
        }
        controls.updateContent();
        ViewHelpers::postMessageFor (this, new RefreshControllerDeviceMessage (editedDevice));
    }

    bool haveControllers() const
    {
        if (auto sess = (const_cast<Content*> (this))->getSession())
            return sess->getNumControllerDevices() > 0;
        return false;
    }

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (deviceName))
        {
            updateComboBoxes();
            ensureCorrectDeviceChosen();
        }
        else if (value.refersToSameSourceAs (inputDevice))
        {
            ViewHelpers::postMessageFor (this,
                                         new RefreshControllerDeviceMessage (editedDevice));
        }
        else if (value.refersToSameSourceAs (controlName))
        {
            triggerAsyncUpdate();
        }
        else if (value.refersToSameSourceAs (eventId))
        {
            controls.updateContent();
        }
        else if (value.refersToSameSourceAs (eventType))
        {
            triggerAsyncUpdate();
        }
        else if (value.refersToSameSourceAs (toggleMode))
        {
            triggerAsyncUpdate();
        }
        else if (value.refersToSameSourceAs (momentary))
        {
            triggerAsyncUpdate();
        }
    }

    void buttonClicked (Button* button) override
    {
        if (button == &createButton)
        {
            createNewController();
        }
        else if (button == &deleteButton)
        {
            deleteEditedController();
        }
        else if (button == &addControlButton)
        {
            createNewControl();
        }
        else if (button == &removeControlButton)
        {
            deleteSelectedControl();
        }
        else if (button == &learnButton)
        {
            if (learnButton.isListening())
            {
                learnButton.stopListening();
            }
            else
            {
                learnButton.setInputDevice (inputDevice.toString().trim());
                learnButton.startListening();
            }
        }
        else if (button == &saveControllerButton)
        {
            auto name = editedDevice.getName().toString();
            if (name.isEmpty())
                name << "Controller";
            name << ".xml";

            FileChooser chooser ("Save Controller Device",
                                 DataPath::defaultControllersDir().getChildFile (name).getNonexistentSibling(),
                                 "*.xml",
                                 true,
                                 false);
            if (chooser.browseForFileToSave (true))
            {
                DBG ("[EL] save device");
                if (auto xml = editedDevice.getValueTree().createXml())
                    xml->writeToFile (chooser.getResult(), String());
            }
        }
        else if (button == &openControllerButton)
        {
            FileChooser chooser ("Open Controller Device",
                                 DataPath::defaultControllersDir(),
                                 "*.xml",
                                 true,
                                 false);
            if (chooser.browseForFileToOpen())
            {
                ViewHelpers::postMessageFor (this,
                                             new AddControllerDeviceMessage (chooser.getResult()));
            }
        }
    }

    void comboBoxChanged (ComboBox* box) override
    {
        editedDevice = ControllerDevice (getSession()->getControllerDeviceValueTree (box->getSelectedItemIndex()));
        stabilizeContent();
    }

    void resized() override
    {
        const int controlsWidth = 280;
        auto r1 (getLocalBounds());

        auto mb = r1.removeFromBottom (jmax (10, mappingsSize));
        mb.removeFromTop (4);
        maps.setBounds (mb);

        auto r2 (r1.removeFromTop (22));
        controllersBox.setBounds (r2.removeFromLeft (controlsWidth).withHeight (22));
        r2.removeFromRight (2);
        createButton.setBounds (r2.removeFromRight (22).reduced (1));
        r2.removeFromRight (2);
        deleteButton.setBounds (r2.removeFromRight (22).reduced (1));
        r2.removeFromRight (2);
        openControllerButton.setBounds (r2.removeFromRight (22).reduced (1));
        r2.removeFromRight (2);
        saveControllerButton.setBounds (r2.removeFromRight (22).reduced (1));

        r1.removeFromTop (4);

        auto r3 = r1.removeFromLeft (controlsWidth);
        auto r4 = r3.removeFromBottom (24).reduced (2);

        learnButton.setBounds (r4.removeFromRight (48));
        r4.removeFromRight (8);

        removeControlButton.setBounds (r4.removeFromLeft (24));
        r4.removeFromLeft (4);
        addControlButton.setBounds (r4.removeFromLeft (24));

        controls.setBounds (r3);

        r1.removeFromLeft (4);
        properties.setBounds (r1);
    }

    void clear()
    {
        editedDevice = ControllerDevice();
        controllersBox.clear();
        properties.clear();
        maps.clear();
    }

    void setChildVisibility (const bool visible)
    {
        Array<Component*> comps ({ &properties, &controls, &addControlButton, &removeControlButton, &learnButton, &saveControllerButton });
        for (auto* comp : comps)
            comp->setVisible (visible);
    }

    void stabilizeContent()
    {
        auto sess = getSession (false);

        if (haveControllers())
        {
            setChildVisibility (true);

            updateProperties();
            updateComboBoxes();
            ensureCorrectDeviceChosen();

            if (controllersBox.getSelectedId() <= 0)
            {
                controllersBox.setSelectedItemIndex (0, dontSendNotification);
                comboBoxChanged (&controllersBox);
            }

            controls.setControllerDevice (editedDevice);
            maps.updateWith (sess, editedDevice, controls.getSelectedControl());
        }
        else
        {
            setChildVisibility (false);
            clear();
        }
    }

    void getControllerDeviceProperties (Array<PropertyComponent*>& props)
    {
        deviceName.removeListener (this);
        inputDevice.removeListener (this);
        controlName.removeListener (this);
        eventType.removeListener (this);
        eventId.removeListener (this);
        toggleMode.removeListener (this);
        momentary.removeListener (this);

        deviceName = editedDevice.getPropertyAsValue (Tags::name);
        props.add (new TextPropertyComponent (deviceName, "Controller Name", 120, false, true));

        jassert (ViewHelpers::findContentComponent (this) != nullptr);
        auto& app = ViewHelpers::findContentComponent (this)->getAppController();

        StringArray keys;
        Array<var> values;
        if (app.getRunMode() == RunMode::Standalone)
        {
            keys.addArray (MidiInput::getDevices());
            for (const auto& d : keys)
                values.add (d);

            const auto& inputDeviceVar = editedDevice.getInputDevice();
            if (inputDeviceVar.toString().isNotEmpty() && ! keys.contains (inputDeviceVar.toString()))
            {
                keys.add (String());
                values.add (String());
                keys.add (inputDeviceVar.toString());
                values.add (inputDeviceVar);
            }
            inputDevice = editedDevice.getPropertyAsValue ("inputDevice");
            if (inputDevice.toString().trim().isEmpty())
                inputDevice.setValue (values.getFirst());
        }
        else
        {
            keys.add ("Host MIDI");
            values.add (String ("hostMidi"));
            inputDevice = Value();
            inputDevice.setValue ("hostMidi");
        }

        props.add (new ChoicePropertyComponent (inputDevice, "Input Device", keys, values));

        if (app.getRunMode() == RunMode::Plugin)
        {
            if (auto* inputDeviceProp = dynamic_cast<ChoicePropertyComponent*> (props.getLast()))
            {
                inputDeviceProp->refresh();
                inputDeviceProp->setEnabled (false);
            }
        }

        auto control = controls.getSelectedControl();

        if (control.isValid())
        {
            controlName = control.getPropertyAsValue (Tags::name);
            props.add (new TextPropertyComponent (controlName,
                                                  "Control Name",
                                                  120,
                                                  false,
                                                  true));

            eventType = control.getPropertyAsValue ("eventType");
            props.add (new ChoicePropertyComponent (eventType, "Event Type", { "Controller", "Note" }, { var ("controller"), var ("note") }));

            String eventName = "Event ID";
            if (control.isNoteEvent())
                eventName = "Note Number";
            else if (control.isControllerEvent())
                eventName = "CC Number";

            props.add (new ChoicePropertyComponent (control.getPropertyAsValue (Tags::midiChannel),
                                                    "Channel",
                                                    { "Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" },
                                                    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }));

            eventId = control.getPropertyAsValue ("eventId");
            props.add (new SliderPropertyComponent (eventId, eventName, 0.0, 127.0, 1.0));

            if (control.isControllerEvent())
            {
                toggleMode = control.getToggleModeObject();
                props.add (new ChoicePropertyComponent (toggleMode, "Toggle Mode", { "Equal or Higher", "Same Value" }, { "eqorhi", "eq" }));

                Value toggleValue = control.getPropertyAsValue ("toggleValue");
                props.add (new SliderPropertyComponent (toggleValue, "Toggle Value", 0.0, 127.0, 1.0));

                if (toggleMode.getValue() != "eq")
                {
                    Value inverseToggle = control.getPropertyAsValue ("inverseToggle");
                    props.add (new BooleanPropertyComponent (inverseToggle, "Toggle Inversely", "Perform the inverse toggle action"));
                }
            }
            else if (control.isNoteEvent())
            {
                momentary = control.getMomentaryValue();
                props.add (new BooleanPropertyComponent (momentary, "Momentary", "Hold the toggle until note off received?"));

                if ((bool) momentary.getValue())
                {
                    Value inverseToggle = control.getPropertyAsValue ("inverseToggle");
                    props.add (new BooleanPropertyComponent (inverseToggle, "Toggle Inversely", "Perform the inverse toggle action"));
                }
            }
        }

        controlName.addListener (this);
        inputDevice.addListener (this);
        deviceName.addListener (this);
        eventType.addListener (this);
        eventId.addListener (this);
        toggleMode.addListener (this);
        momentary.addListener (this);
    }

    void createNewController()
    {
        auto newDevice = ControllerDevice ("New Device");
        ViewHelpers::postMessageFor (this, new AddControllerDeviceMessage (newDevice));
    }

    void controllerAdded (const ControllerDevice& device)
    {
        editedDevice = device;
        stabilizeContent();
    }

    void deleteEditedController()
    {
        ViewHelpers::postMessageFor (this, new RemoveControllerDeviceMessage (editedDevice));
    }

    void controllerRemoved (const ControllerDevice&)
    {
        int index = controllersBox.getSelectedItemIndex();
        index = jmin (index, session->getNumControllerDevices() - 1);
        if (index >= 0 && index < session->getNumControllerDevices())
            editedDevice = session->getControllerDevice (index);
        else
            editedDevice = ControllerDevice();
        stabilizeContent();
    }

    void createNewControl()
    {
        String controlName = "Control ";
        controlName << (controls.getNumRows() + 1);
        const ControllerDevice::Control newControl (controlName);
        ViewHelpers::postMessageFor (this, new AddControlMessage (editedDevice, newControl));
    }

    void onControlAdded (const ControllerDevice::Control& control)
    {
        controls.updateContent();
        const auto index = editedDevice.indexOf (control);
        if (index >= 0 && index < controls.getNumRows())
        {
            controls.selectRow (index);
            controls.repaintRow (index);
        }
    }

    void deleteSelectedControl()
    {
        const auto selected (ControllerDevice::Control (
            editedDevice.getControl (controls.getSelectedRow())));
        ViewHelpers::postMessageFor (this, new RemoveControlMessage (editedDevice, selected));
    }

    void onControlRemoved (const ControllerDevice::Control& control)
    {
        auto selected = controls.getSelectedRow();
        controls.updateContent();
        if (controls.getNumRows() > 0)
        {
            selected = (selected < 0) ? 0 : jmax (0, jmin (selected, controls.getNumRows() - 1));
            controls.selectRow (selected);
            controls.repaintRow (selected);
        }
        else
        {
            controls.deselectAllRows();
        }
    }

    void setSession (SessionPtr s, const bool stabilize = false)
    {
        if (s == session)
            return;
        disconnectHandlers();
        session = s;
        connectHandlers();

        if (stabilize)
            triggerAsyncUpdate();
    }

    SessionPtr getSession (const bool force = false)
    {
        if (session == nullptr || force)
        {
            if (session)
                disconnectHandlers();
            session = ViewHelpers::getSession (this);
            connectHandlers();
        }

        return session;
    }

    void setMappingSize (const int newSize)
    {
        mappingsSize = newSize;
        resized();
    }

private:
    ControllerDevice editedDevice;
    SettingButton testButton;
    SettingButton createButton;
    SettingButton deleteButton;
    SettingButton addControlButton;
    SettingButton removeControlButton;
    SettingButton saveControllerButton;
    SettingButton openControllerButton;
    MidiLearnButton learnButton;
    ComboBox controllersBox;
    ControlListBox controls;
    PropertyPanel properties;
    ControllerMapsTable maps;
    SessionPtr session;
    Value deviceName, inputDevice, controlName;
    Value eventType, eventId, toggleMode;
    Value momentary;

    int mappingsSize = 150;
    void updateComboBoxes()
    {
        const auto controllers = getSession()->getValueTree().getChildWithName (Tags::controllers);
        controllersBox.clear (dontSendNotification);
        for (int i = 0; i < controllers.getNumChildren(); ++i)
        {
            const auto controller = controllers.getChild (i);
            const auto name = controller.getProperty (Tags::name).toString();
            const int itemId = i + 1;
            controllersBox.addItem (name, itemId);
        }
    }

    void ensureCorrectDeviceChosen()
    {
        int index = 0;
        const auto controllerName (editedDevice.getName().toString());
        const auto controllerIndex (editedDevice.getValueTree().getParent().indexOf (editedDevice.getValueTree()));
        if (controllerIndex < 0)
            return;

        for (index = 0; index < controllersBox.getNumItems(); ++index)
            if (controllerIndex == index && controllerName.equalsIgnoreCase (controllersBox.getItemText (index)))
                break;
        controllersBox.setSelectedItemIndex (index, dontSendNotification);
    }

    void updateProperties()
    {
        properties.clear();
        Array<PropertyComponent*> props;
        getControllerDeviceProperties (props);
        properties.addProperties (props);
        props.clearQuick();

        // getControllerDeviceProperties (props);
        // properties.addSection ("Section", props);
    }

    void connectHandlers()
    {
        disconnectHandlers();
        if (session != nullptr)
        {
            connections.add (session->controllerDeviceAdded.connect (
                std::bind (&Content::controllerAdded, this, std::placeholders::_1)));
            connections.add (session->controllerDeviceRemoved.connect (
                std::bind (&Content::controllerRemoved, this, std::placeholders::_1)));
            connections.add (session->controlAdded.connect (
                std::bind (&Content::onControlAdded, this, std::placeholders::_1)));
            connections.add (session->controlRemoved.connect (
                std::bind (&Content::onControlRemoved, this, std::placeholders::_1)));
        }
    }

    void disconnectHandlers()
    {
        for (auto& connection : connections)
            connection.disconnect();
        connections.clear();
    }

    Array<boost::signals2::connection> connections;
};

ControllerDevicesView::ControllerDevicesView()
{
    setName ("ControllerDevicesView");
    content.reset (new Content());
    addAndMakeVisible (content.get());
}

void ControllerDevicesView::initializeView (AppController& app)
{
    if (content)
        content->setSession (app.getWorld().getSession(), false);
}

ControllerDevicesView::~ControllerDevicesView()
{
    content.reset (nullptr);
}

void ControllerDevicesView::resized()
{
    content->setBounds (getLocalBounds().reduced (2));
}

void ControllerDevicesView::stabilizeContent()
{
    content->stabilizeContent();
}

} // namespace Element

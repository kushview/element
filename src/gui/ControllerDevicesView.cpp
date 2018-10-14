/*
    ControllerDevice.cpp - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#include "gui/Buttons.h"
#include "gui/ControllerDevicesView.h"
#include "gui/ViewHelpers.h"
#include "session/ControllerDevice.h"
#include "Messages.h"

namespace Element {

class ControllerMapsTable : public TableListBox,
                      public TableListBoxModel
{
public:
    enum Columns { Device = 1, Control, Node, Parameter };

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

    void updateWith (SessionPtr sess, 
                     const ControllerDevice& device = ControllerDevice(),
                     const ControllerDevice::Control& control = ControllerDevice::Control())
    {
        session = sess;
        maps.clear (true);
        
        if (session)
        {
            for (int i = 0; i < session->getNumControllerMaps(); ++i)
            {
                std::unique_ptr<ControllerMapObjects> objects;
                objects.reset (new ControllerMapObjects (session, session->getControllerMap (i)));
                if (!device.isValid() || 
                        (device.isValid() && device.getUuidString() == objects->device.getUuidString()))
                {
                    maps.add (objects.release());
                }
            }
        }

        updateContent();
        repaint();
    }

    int getNumRows() override { return maps.size(); }

    void paintRowBackground (Graphics& g, int rowNumber,
                             int width, int height,
                             bool rowIsSelected) override
    {
        ViewHelpers::drawBasicTextRow ("", g, width, height, rowIsSelected);
    }

    void paintCell (Graphics& g, int rowNumber,
                    int columnId, int width, int height,
                    bool rowIsSelected) override
    {
        auto* const objects = maps [rowNumber];
        if (! objects) return;

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
            } break;

            case Control: {
                text = control.getName().toString();
            } break;

            case Node: {
                text = node.getName();
            } break;

            case Parameter: {
                if (auto* obj = node.getGraphNode())
                    if (auto* proc = obj->getAudioProcessor())
                        if (auto* param = proc->getParameters()[mapp.getParameterIndex()])
                            text = param->getName (64);
            } break;
        }

        g.drawText (text, 0,0, width, height, Justification::centredLeft);
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
        const auto mapp (session->getControllerMap (lastRowSelected));
        ViewHelpers::postMessageFor (this, new RemoveControllerMapMessage (mapp));
    }

    void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
        auto* const objects = maps [rowNumber];
        if (! objects) return;

        if (columnId == Node && objects->node.isValid())
            ViewHelpers::presentPluginWindow (this, objects->node);
    }
#if 0
    virtual Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate);
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
};

class MidiLearnButton : public SettingButton,
                         public MidiInputCallback,
                         public AsyncUpdater,
                         public Button::Listener
{
public:
    boost::signals2::signal<void()> messageReceived;

    explicit MidiLearnButton (const String& deviceName = String())
    { 
        addListener (this);
    }

    void buttonClicked (Button*) override
    {
        // if (! isEnabled())
        //     return;
        // if (isListening()) stopListening();
        // else startListening();
    }

    bool isListening() const { return input != nullptr; }

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
        if (input)
        {
            input->stop();
            input.reset (nullptr);
        }

        updateToggleState();
    }

    void startListening()
    {
        stopListening();
        clearMessage();
        
        const auto deviceIdx = MidiInput::getDevices().indexOf (inputName);
        if (deviceIdx >= 0)
            input.reset (MidiInput::openDevice (deviceIdx, this));
        
        if (input)
        {
            DBG("[EL] started learning midi");
            input->start();
        }
        else
        {
            DBG("[EL] could not start MIDI device: " << inputName);
        }

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
    Atomic<bool> gotFirstMessage = false;
    Atomic<bool> stopOnFirstMessage = false;
    MidiMessage message;
    String inputName;
    std::unique_ptr<MidiInput> input;
};

class ControlListBox : public ListBox,
                       public ListBoxModel
{
public:
    std::function<void()> selectionChanged;

    ControlListBox ()
    {
        setModel (this);
        setRowHeight (48);
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

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                           bool rowIsSelected) override 
    { 
        ignoreUnused (rowNumber, g, width, height, rowIsSelected);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                       Component* existingComponentToUpdate) override
    {
        auto* row = dynamic_cast<ControllerRow*> (existingComponentToUpdate);
        if (row == nullptr)
            row = new ControllerRow (*this);

        row->refresh (editedDevice.getControl (rowNumber), 
                      rowNumber, isRowSelected);
        
        return row;
    }

    void listBoxItemClicked (int row, const MouseEvent&) override { DBG("clicked");}
    void selectedRowsChanged (int lastRowSelected) override
    { 
        if (selectionChanged)
            selectionChanged();
    }

   #if 0
    void listBoxItemClicked (int row, const MouseEvent&) override { }
    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { }
    void backgroundClicked (const MouseEvent&) override { }
    void selectedRowsChanged (int lastRowSelected) override { }
    void deleteKeyPressed (int lastRowSelected) override { }
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
                g, getWidth(), getHeight(), selected);
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

            const auto midi = control.getMappingData();
            String text = "N/A";
            if (midi.isNoteOn())
                text = midi.getMidiNoteName (midi.getNoteNumber(), true, true, 4);
            if (midi.isController()) {
                text = "CC "; text << midi.getControllerNumber();
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

        addAndMakeVisible (maps);

        deviceName.addListener (this);
        inputDevice.addListener (this);
        controlName.addListener (this);

        triggerAsyncUpdate();
    }
    
    ~Content()
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
        return message.isController() && message.getRawDataSize() > 0;
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
        if (auto sess = (const_cast<Content*>(this))->getSession())
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
    }

    void comboBoxChanged (ComboBox* box) override
    {
        editedDevice = ControllerDevice (getSession()->getControllerDeviceValueTree (box->getSelectedItemIndex()));
        stabilizeContent();
    }

    void resized() override
    {
        const int controlsWidth     = 280;
        auto r1 (getLocalBounds());

        auto mb = r1.removeFromBottom (jmax (10, mappingsSize));
        mb.removeFromTop (4);
        maps.setBounds (mb);

        auto r2 (r1.removeFromTop (22));
        controllersBox.setBounds (r2.removeFromLeft (controlsWidth).withHeight (22));
        r2.removeFromRight (2);
        createButton.setBounds (r2.removeFromRight (22).reduced(1));
        r2.removeFromRight (2);
        deleteButton.setBounds (r2.removeFromRight (22).reduced(1));
        
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
    }

    void setChildVisibility (const bool visible)
    {
        properties.setVisible (visible);
        controls.setVisible (visible);
        addControlButton.setVisible (visible);
        removeControlButton.setVisible (visible);
        learnButton.setVisible (visible);
    }

    void stabilizeContent()
    {
        auto sess = getSession (true);

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

        deviceName = editedDevice.getPropertyAsValue (Tags::name);
        props.add (new TextPropertyComponent (deviceName, "Controller Name", 120, false, true));

        StringArray keys = MidiInput::getDevices();
        Array<var> values;
        for (const auto& d : keys)
            values.add (d);
        inputDevice = editedDevice.getPropertyAsValue ("inputDevice");
        props.add (new ChoicePropertyComponent (editedDevice.getPropertyAsValue ("inputDevice"),
            "Input Device", keys, values));

        auto control = controls.getSelectedControl();

        if (control.isValid())
        {
            controlName = control.getPropertyAsValue (Tags::name);
            props.add (new TextPropertyComponent (controlName, 
                "Control Name", 120, false, true));
        }

        controlName.addListener (this);
        inputDevice.addListener (this);
        deviceName.addListener (this);
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
    MidiLearnButton learnButton;
    ComboBox controllersBox;
    ControlListBox controls;
    PropertyPanel properties;
    ControllerMapsTable maps;
    SessionPtr session;
    Value deviceName, inputDevice, controlName;

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
        const auto controllerIndex (editedDevice.getValueTree().getParent()
                                    .indexOf (editedDevice.getValueTree()));
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
    disableIfNotUnlocked();
    content->stabilizeContent();
}

}

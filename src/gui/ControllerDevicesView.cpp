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

class ControlListBox : public ListBox,
                       public ListBoxModel
{
public:
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

        row->rowNumber = rowNumber;
        row->selected = isRowSelected;
        row->refresh();
        
        return row;
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
            addAndMakeVisible (learnButton);
            learnButton.setButtonText ("Learn");
            learnButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);
            learnButton.addListener (this);

            status.setJustificationType (Justification::centredRight);
            status.setColour (Label::textColourId, Element::LookAndFeel::textColor.darker());
            status.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (status);
        }

        void buttonClicked (Button*) override
        {
            list.selectRow (rowNumber, true, true);
            setSelected (list.getSelectedRow() == rowNumber);
            learnButton.setToggleState (!learnButton.getToggleState(), dontSendNotification);
        }

        void mouseDown (const MouseEvent&) override
        {
            list.selectRow (rowNumber, true);
            setSelected (list.getSelectedRow() == rowNumber);
        }

        void paint (Graphics& g) override
        {
            String text = "Knob #"; text << (1 + rowNumber);
            ViewHelpers::drawBasicTextRow (text, g, getWidth(), getHeight(), selected);
        }

        void resized() override
        {
            auto r (getLocalBounds().reduced (8));
            learnButton.setBounds (r.removeFromRight (4 + r.getHeight()));
            status.setBounds (r.removeFromRight (getWidth() / 2));
        }

        void refresh()
        {
            status.setText ("Unassigned", dontSendNotification);
            setSelected (rowNumber == list.getSelectedRow());
        }

        void setSelected (const bool isNowSelected)
        {
            if (selected == isNowSelected)
                return;
            selected = isNowSelected;
        }

        int rowNumber = -1;
        bool selected = false;
        SettingButton learnButton;
        Label status;

    private:
        ControlListBox& list;
    };
};

class ControllerDevicesView::Content : public Component,
                                       public Button::Listener,
                                       public ComboBox::Listener,
                                       public Value::Listener
{
public:
    Content()
    { 
        controllersBox.setTextWhenNoChoicesAvailable ("No Controllers");
        controllersBox.setTextWhenNothingSelected ("(Controllers)");
        controllersBox.addListener (this);
        addAndMakeVisible (controllersBox);

        createButton.setButtonText ("+");
        createButton.addListener (this);
        addAndMakeVisible (createButton);

        deleteButton.setButtonText ("-");
        deleteButton.addListener (this);
        addAndMakeVisible (deleteButton);

        controls.setControllerDevice (editedDevice);
        addAndMakeVisible (controls);
        
        addControlButton.setButtonText ("+");
        addControlButton.addListener (this);
        addAndMakeVisible (addControlButton);

        removeControlButton.setButtonText ("-");
        addAndMakeVisible (removeControlButton);
        removeControlButton.addListener (this);
        addAndMakeVisible (properties);

        deviceName.addListener (this);

        stabilizeContent();
    }
    
    ~Content()
    {
        deviceName.removeListener (this);
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
        auto r2 (r1.removeFromTop (18));
        controllersBox.setBounds (r2.removeFromLeft (200).withHeight (24));
        createButton.setBounds (r2.removeFromRight (18));
        deleteButton.setBounds (r2.removeFromRight (18));

        r1.removeFromTop (8);

        auto r3 = r1.removeFromLeft (controlsWidth);
        auto r4 = r3.removeFromBottom (32).reduced (2);

        addControlButton.setBounds (r4.removeFromRight (48));
        r4.removeFromRight (4);
        removeControlButton.setBounds (r4.removeFromRight (48));
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
    }

    void stabilizeContent()
    {
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
        deviceName = editedDevice.getPropertyAsValue (Tags::name);
        props.add (new TextPropertyComponent (deviceName, "Name", 120, false, true));

        StringArray keys = MidiInput::getDevices();
        Array<var> values;
        for (const auto& d : keys)
            values.add (d);
        props.add (new ChoicePropertyComponent (editedDevice.getPropertyAsValue ("inputDevice"),
            "Input Device", keys, values));
        deviceName.addListener (this);
    }

    void createNewController()
    {
        auto newDevice = ControllerDevice();
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
        ControllerDevice::Control newControl;
        DBG(editedDevice.getValueTree().toXmlString());
        ViewHelpers::postMessageFor (this, new AddControlMessage (editedDevice, newControl));
    }

    void controlAdded (const ControllerDevice::Control& control)
    {
        controls.updateContent();
    }

    void deleteSelectedControl()
    {
        const auto selected (ControllerDevice::Control (
            editedDevice.getControl (controls.getSelectedRow())));
        ViewHelpers::postMessageFor (this, new RemoveControlMessage (editedDevice, selected));
    }

    void controlRemoved (const ControllerDevice::Control& control)
    {
        const auto selected = controls.getSelectedRow();
        if (isPositiveAndBelow (selected, editedDevice.getNumChildren()))
        {
            editedDevice.getValueTree().removeChild (selected, nullptr);
            controls.updateContent();
            if (controls.getNumRows() > 0)
                controls.selectRow (jmax (0, jmin (selected, controls.getNumRows() - 1)));
            else
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

private:
    ControllerDevice editedDevice;
    SettingButton testButton;
    SettingButton createButton;
    SettingButton deleteButton;
    SettingButton addControlButton;
    SettingButton removeControlButton;
    ComboBox controllersBox;
    ControlListBox controls;
    PropertyPanel properties;
    SessionPtr session;
    Value deviceName;

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
        properties.addSection ("Device", props);
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
        }
    }

    void disconnectHandlers()
    {
        for (auto connection : connections)
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
    content->stabilizeContent();
}

ControllerMapsView::ControllerMapsView() { }
ControllerMapsView::~ControllerMapsView() { }

}

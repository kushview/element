
#include "gui/Buttons.h"
#include "gui/ControllerDevicesView.h"
#include "gui/ViewHelpers.h"
#include "session/ControllerDevice.h"

namespace Element {

class ControlListBox : public ListBox,
                       public ListBoxModel
{
public:
    ControlListBox()
    {
        setModel (this);
        setRowHeight (24);
    }

    ~ControlListBox()
    {
        setModel (nullptr);
    }

    int getNumRows() override
    {
        return 0;
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
            addAndMakeVisible (status);
        }

        void buttonClicked (Button*) override
        {
            list.selectRow (rowNumber, true, true);
            setSelected (list.getSelectedRow() == rowNumber);
            learnButton.setToggleState (!learnButton.getToggleState(), dontSendNotification);
        }

        void paint (Graphics& g) override
        {
            String text = "Knob #"; text << (1 + rowNumber);
            ViewHelpers::drawBasicTextRow (text, g, getWidth(), getHeight(), selected);
        }

        void resized() override
        {
            auto r (getLocalBounds().reduced (4));
            learnButton.setBounds (r.removeFromRight (48));
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
        // testButton.setButtonText ("Test Button");
        // addAndMakeVisible (testButton);

        controllersBox.setTextWhenNoChoicesAvailable ("No Controllers");
        controllersBox.setTextWhenNothingSelected ("(Controllers)");
        addAndMakeVisible (controllersBox);

        createButton.setButtonText ("+");
        createButton.addListener (this);
        addAndMakeVisible (createButton);

        deleteButton.setButtonText ("-");
        deleteButton.addListener (this);
        addAndMakeVisible (deleteButton);

        addAndMakeVisible (controls);
        
        addControlButton.setButtonText ("Add control");
        addControlButton.addListener (this);
        addAndMakeVisible (addControlButton);

        addAndMakeVisible (properties);

        deviceName.addListener(this);

        stabilizeContent();
    }
    
    ~Content()
    {
        deviceName.removeListener(this);
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

        }
    }

    void comboBoxChanged (ComboBox* box) override
    {
        editedDevice = ControllerDevice (getSession()->getControllerDeviceValueTree (box->getSelectedItemIndex()));
        updateProperties();
    }

    void resized() override 
    {
        auto r1 (getLocalBounds());
        auto r2 (r1.removeFromTop (18));
        controllersBox.setBounds (r2.removeFromLeft (200).withHeight (24));
        createButton.setBounds (r2.removeFromRight (18));
        deleteButton.setBounds (r2.removeFromRight (18));

        r1.removeFromTop (2);
        controls.setBounds (r1.removeFromLeft (240));
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
    }

    void stabilizeContent()
    {
        if (haveControllers())
        {
            setChildVisibility (true);
            controls.updateContent();
            updateProperties();
            updateComboBoxes();
        }
        else
        {
            setChildVisibility (false);
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
        editedDevice = ControllerDevice();
        getSession()->getValueTree().getOrCreateChildWithName ("controllers", nullptr)
            .addChild (editedDevice.getValueTree(), -1, nullptr);
        stabilizeContent();
    }

    void deleteEditedController()
    {
        getSession()->getValueTree().getChildWithName ("controllers")
            .removeChild (editedDevice.getValueTree(), nullptr);
        stabilizeContent();
    }

    SessionPtr getSession (const bool force = false)
    {
        if (session == nullptr || force)
            session = ViewHelpers::getSession (this);
        return session;
    }

private:
    ControllerDevice editedDevice;
    SettingButton testButton;
    SettingButton createButton;
    SettingButton deleteButton;
    ComboBox controllersBox;
    ControlListBox controls;
    SettingButton addControlButton;
    PropertyPanel properties;
    SessionPtr session;
    Value deviceName;

    void updateComboBoxes()
    {
        const auto controllerName = controllersBox.getItemText (controllersBox.getSelectedItemIndex());
        const auto controllers = getSession()->getValueTree().getChildWithName ("controllers");
        int selectedId = 0;

        controllersBox.clear (dontSendNotification);

        for (int i = 0; i < controllers.getNumChildren(); ++i)
        {
            const auto controller = controllers.getChild (i);
            const auto name = controller.getProperty (Tags::name).toString();
            const int itemId = i + 1;
            controllersBox.addItem (name, itemId);
            if (name.equalsIgnoreCase (controllerName) && selectedId <= 0)
                selectedId = itemId;
        }
        if (selectedId <= 0 && controllersBox.getNumItems() > 0)
            selectedId = 1;
        if (selectedId > 0)
        {
            editedDevice = ControllerDevice (controllers.getChild (selectedId - 1));
            controllersBox.setSelectedId (selectedId, dontSendNotification);
        }
    }

    void updateProperties()
    {
        properties.clear();
        Array<PropertyComponent*> props;
        getControllerDeviceProperties (props);
        properties.addSection ("Device", props);
        props.clearQuick();
    }
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

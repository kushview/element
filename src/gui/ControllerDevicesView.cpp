
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

    int getNumRows() override { return 16; }

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
    void listBoxItemClicked (int row, const MouseEvent&) override {  }
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
            learnButton.setBounds (r.removeFromRight (64));
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
                                       public ComboBox::Listener
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
        addAndMakeVisible (properties);
    }
    
    ~Content() { }

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
    }

    void comboBoxChanged (ComboBox* box) override
    {
        const auto selectedId = box->getSelectedId();
    }

    void resized() override 
    {
        auto r1 (getLocalBounds());
        auto r2 (r1.removeFromTop (18));
        controllersBox.setBounds (r2.removeFromLeft (200));
        createButton.setBounds (r2.removeFromRight (18));
        deleteButton.setBounds (r2.removeFromRight (18));

        r1.removeFromTop (2);
        controls.setBounds (r1.removeFromLeft (240));
        r1.removeFromLeft (4);
        properties.setBounds (r1);
    }

    void clear()
    {
        controllersBox.clear();
        properties.clear();
    }

    void stabilizeContent()
    {
        clear();

        Array<PropertyComponent*> props;
        props.add (new TextPropertyComponent (editedDevice.getPropertyAsValue (Tags::name),
             "Name", 120, false, true));

        StringArray keys = MidiInput::getDevices();
        Array<var> values;
        for (const auto& d : keys)
            values.add (d);
        props.add (new ChoicePropertyComponent (editedDevice.getPropertyAsValue ("inputDevice"),
            "Input Device", keys, values));
        properties.addSection ("Device", props);
        props.clearQuick(); values.clearQuick();

        auto controls = editedDevice.getValueTree().getOrCreateChildWithName ("controls", 0);

        props.add (new TextPropertyComponent (controls.getPropertyAsValue (Tags::name, 0),
             "Name", 120, false, true));

        keys = { "Knob", "Button", "Fader" };
        values = { "knob", "button", "fader" };
        props.add (new ChoicePropertyComponent (controls.getPropertyAsValue (Tags::type, 0),
            "Type", keys, values));
        properties.addSection ("Control", props);
    }

    void createNewController()
    {
        
    }

    void deleteEditedController()
    {

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
    PropertyPanel properties;
    SessionPtr session;
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

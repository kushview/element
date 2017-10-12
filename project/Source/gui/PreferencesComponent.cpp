/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.1.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "session/DeviceManager.h"
#include "Globals.h"
//[/Headers]

#include "PreferencesComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace Element {


    class PreferencesComponent::PageList :  public ListBox,
                                            public ListBoxModel
    {
    public:

        PageList (PreferencesComponent& prefs)
            : owner (prefs)
        {
            font.setHeight (16);
            setModel (this);
        }

        ~PageList()
        {
            setModel (nullptr);
        }

        int getNumRows()
        {
            return pageNames.size();
        }

        virtual void paintListBoxItem (int rowNumber,
                                       Graphics& g,
                                       int width, int height,
                                       bool rowIsSelected)
        {
            if (rowNumber < pageNames.size())
            {
                if (rowIsSelected)
                {
                    g.setColour (Colour (0xff444444));
                    g.fillRect (0, 0, width, height);
                }

                g.setFont (font);
                g.setColour (Colours::whitesmoke);
                g.drawText (pageNames[rowNumber], 10, 0, width - 10, height, Justification::left, true);
            }
        }

        void listBoxItemClicked (int row, const MouseEvent& e)
        {
            if (uri != pageURIs[row]) {
                uri = pageURIs [row];
                owner.setPage (uri);
            }
        }

        virtual String getTooltipForRow (int row)
        {
            String tool (pageNames[row]);
            tool << String(" ") << "settings";
            return tool;
        }


    private:
        friend class PreferencesComponent;

        void addItem (const String& name, const String& identifier)
        {
            pageNames.addIfNotAlreadyThere (name);
            pageURIs.addIfNotAlreadyThere (identifier);
            updateContent();
        }

        Font font;
        PreferencesComponent& owner;

        StringArray pageNames;
        StringArray pageURIs;
        String      uri;

    };

    typedef AudioDeviceSelectorComponent DevicesComponent;
    class AudioSettingsComponent : public DevicesComponent
    {
    public:
        AudioSettingsComponent (DeviceManager& devices)
            : DevicesComponent (devices, 0, 16, 0, 16,
                                true, true, true, false)
        {
            setSize (300, 400);
        }
    };

//[/MiscUserDefs]

//==============================================================================
PreferencesComponent::PreferencesComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (pageList = new PageList (*this));
    pageList->setName ("Page List");

    addAndMakeVisible (groupComponent = new GroupComponent ("new group",
                                                            TRANS("group")));
    groupComponent->setColour (GroupComponent::outlineColourId, Colour (0xff888888));
    groupComponent->setColour (GroupComponent::textColourId, Colours::white);

    addAndMakeVisible (pageComponent = new Component());
    pageComponent->setName ("new component");


    //[UserPreSize]
    groupComponent->setVisible (false);
    //[/UserPreSize]

    setSize (600, 500);


    //[Constructor] You can add your own custom stuff here..
    // pageList->addItem ("Application",  "element://gui/application");
    pageList->addItem ("Audio Engine", "element://gui/audioEngine");
    // pageList->addItem ("MIDI Devices", "element://gui/midiDevices");
    setPage ("element://gui/audioEngine");
    //[/Constructor]
}

PreferencesComponent::~PreferencesComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    pageList = nullptr;
    groupComponent = nullptr;
    pageComponent = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PreferencesComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff3b3b3b));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PreferencesComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    pageList->setBounds (8, 8, 184, 480);
    groupComponent->setBounds (200, 8, 392, 480);
    pageComponent->setBounds (208, 32, 376, 448);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void PreferencesComponent::setPage (const String& uri)
{
    if (nullptr != pageComponent && uri == pageComponent->getName())
        return;

    Rectangle<int> b = pageComponent->getBounds();
    if (uri == "element://gui/audioEngine")
    {
        groupComponent->setText ("Audio");
        pageComponent = new Component(); // FIXME: new AudioSettingsComponent (gui);
        pageComponent->setName (uri);
    }
    else if (uri == "element://gui/application")
    {
        groupComponent->setText ("Application");
        pageComponent = new Component(); //new AppSettingsComponent (gui);
        pageComponent->setName (uri);
    }
    else
    {
        groupComponent->setText ("None");
        pageComponent = new Component();
        pageComponent->setName ("nil");
    }

    pageList->selectRow (pageList->pageURIs.indexOf (uri));
    pageComponent->setBounds (b);
    addAndMakeVisible (pageComponent);
}

} /* namespace Element */
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PreferencesComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="4" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="600" initialHeight="500">
  <BACKGROUND backgroundColour="ff3b3b3b"/>
  <GENERICCOMPONENT name="Page List" id="c2205f1e30617b7c" memberName="pageList"
                    virtualName="" explicitFocusOrder="0" pos="8 8 184 480" class="PageList"
                    params="*this"/>
  <GROUPCOMPONENT name="new group" id="8e138086820b2998" memberName="groupComponent"
                  virtualName="" explicitFocusOrder="0" pos="200 8 392 480" outlinecol="ff888888"
                  textcol="ffffffff" title="group"/>
  <GENERICCOMPONENT name="new component" id="8b11ff6707734770" memberName="pageComponent"
                    virtualName="" explicitFocusOrder="0" pos="208 32 376 448" class="Component"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

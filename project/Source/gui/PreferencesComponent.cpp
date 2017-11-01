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
#include "session/PluginManager.h"
#include "gui/GuiCommon.h"
#include "Globals.h"
#include "Settings.h"

#define EL_GENERAL_SETTINGS_NAME "General"
#define EL_AUDIO_ENGINE_PREFERENCE_NAME "Audio Engine"
#define EL_PLUGINS_PREFERENCE_NAME  "Plugins"

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

        void paint (Graphics& g) {
            g.fillAll (LookAndFeel::widgetBackgroundColor.darker (0.45));
        }
        virtual void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                       bool rowIsSelected)
        {
            if (! isPositiveAndBelow (rowNumber, pageNames.size()))
                return;
            ViewHelpers::drawBasicTextRow(pageNames[rowNumber], g, width, height, rowIsSelected);
        }

        void listBoxItemClicked (int row, const MouseEvent& e)
        {
            if (isPositiveAndBelow (row, pageNames.size()) && page != pageNames [row])
            {
                page = pageNames [row];
                owner.setPage (page);
            }
        }

        virtual String getTooltipForRow (int row)
        {
            String tool (pageNames[row]);
            tool << String(" ") << "settings";
            return tool;
        }

        int indexOfPage (const String& name) const {
            return pageNames.indexOf (name);
        }

    private:
        friend class PreferencesComponent;

        void addItem (const String& name, const String& identifier)
        {
            pageNames.addIfNotAlreadyThere (name);
            updateContent();
        }

        Font font;
        PreferencesComponent& owner;
        StringArray pageNames;
        String page;
    };

    class SettingsPage : public Component
    {
    public:
    };
    
    class GeneralSettingsPage : public SettingsPage,
                                public ButtonListener,
                                public ValueListener
    {
    public:
        enum ComboBoxIDs
        {
            ClockSourceInternal  = 1,
            ClockSourceMidiClock = 2
        };
        
        GeneralSettingsPage (Globals& world)
            : settings (world.getSettings()),
              engine (world.getAudioEngine())
        {
            addAndMakeVisible (clockSourceLabel);
            clockSourceLabel.setText ("Clock Source", dontSendNotification);
            clockSourceLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (clockSourceBox);
            clockSourceBox.addItem ("Internal / Session", ClockSourceInternal);
            clockSourceBox.addItem ("MIDI Clock", ClockSourceMidiClock);
            clockSource.referTo (clockSourceBox.getSelectedIdAsValue());
            
            const int source = String("internal") == settings.getUserSettings()->getValue("clockSource")
                ? ClockSourceInternal : ClockSourceMidiClock;
            clockSource.setValue (source);
            clockSource.addListener (this);
        }
        
        virtual ~GeneralSettingsPage()
        {
            clockSource.removeListener (this);
        }
        
        void resized() override
        {
            const int spacingBetweenSections = 6;
            Rectangle<int> r (getLocalBounds());
            clockSourceLabel.setBounds (r.removeFromTop (22));
            r.removeFromTop (spacingBetweenSections);
            clockSourceBox.setBounds (r.removeFromTop (22));
        }
        
        void buttonClicked (Button*) override { }
        
        void valueChanged (Value& value) override
        {
            if (! value.refersToSameSourceAs (clockSource))
                return;
            
            const var val = ClockSourceInternal == (int)clockSource.getValue() ? "internal" : "midiClock";
            settings.getUserSettings()->setValue ("clockSource", val);
            engine->applySettings (settings);
        }
        
    private:
        Label clockSourceLabel;
        ComboBox clockSourceBox;
        Value clockSource;
        Settings& settings;
        AudioEnginePtr engine;
    };
    
    class PluginSettingsComponent : public Component,
                                        public ButtonListener
    {
    public:
        PluginSettingsComponent (Globals& w)
            : plugins (w.getPluginManager()),
              settings (w.getSettings())
        {
            addAndMakeVisible (activeFormats);
            activeFormats.setText ("Enabled Plugin Formats", dontSendNotification);
            activeFormats.setFont (Font (18.0, Font::bold));
            addAndMakeVisible (formatNotice);
            formatNotice.setText ("Note: enabled format changes take effect upon restart", dontSendNotification);
            formatNotice.setFont (Font (12.0, Font::italic));
           #if JUCE_MAC
            availableFormats.addArray ({ "AudioUnit", "VST", "VST3" });
           #else
            availableFormats.addArray ({ "VST", "VST3" });
           #endif
            for (const auto& f : availableFormats)
            {
                auto* toggle = formatToggles.add (new ToggleButton (f));
                addAndMakeVisible (toggle);
                toggle->setButtonText (nameForFormat (f));
                toggle->setColour (ToggleButton::textColourId, LookAndFeel::textColor);
                toggle->setColour (ToggleButton::tickColourId, Colours::black);
                toggle->addListener (this);
            }
            
            updateToggleStates();
        }

        void resized() override
        {
            const int spacingBetweenSections = 6;
            const int toggleInset = 4;
            
            Rectangle<int> r (getLocalBounds());
            activeFormats.setBounds (r.removeFromTop (24));
            formatNotice.setBounds (r.removeFromTop (14));
            
            r.removeFromTop (spacingBetweenSections);
            
            for (auto* c : formatToggles)
            {
                auto r2 = r.removeFromTop (18);
                c->setBounds (r2.removeFromRight (getWidth() - toggleInset));
                r.removeFromTop (4);
            }
        }
        
        void paint (Graphics&) override { }
        
        void buttonClicked (Button*) override
        {
            writeSetting();
        }
        
    private:
        PluginManager&  plugins;
        Settings&       settings;
        
        Label activeFormats;
        
        OwnedArray<ToggleButton> formatToggles;
        StringArray availableFormats;
        
        Label formatNotice;
        
        const String key = "enabledPluginFormats";
        bool hasChanged = false;
        
        String nameForFormat (const String& name)
        {
            if (name == "AudioUnit")
                return "Audio Unit";
            return name;
        }
        
        void updateToggleStates()
        {
            auto& formats = plugins.formats();
            for (auto* c : formatToggles)
            {
                c->setToggleState (false, dontSendNotification);
                for (int i = 0; i < formats.getNumFormats(); ++i)
                {
                    if (formats.getFormat(i)->getName() == c->getName())
                        { c->setToggleState(true, dontSendNotification); break; }
                }
            }
        }
        
        void writeSetting()
        {
            StringArray toks;
            for (auto* c : formatToggles)
            {
                if (c->getToggleState())
                    toks.add (c->getName());
            }
            
            const auto value = toks.joinIntoString(",");
            settings.getUserSettings()->setValue (key, value);
        }
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
PreferencesComponent::PreferencesComponent (Globals& g)
    : world (g)
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
    addPage (EL_GENERAL_SETTINGS_NAME);
    addPage (EL_AUDIO_ENGINE_PREFERENCE_NAME);
    setPage (EL_GENERAL_SETTINGS_NAME);
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
    g.fillAll (LookAndFeel::widgetBackgroundColor);
    //[/UserPrePaint]

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
void PreferencesComponent::addPage (const String& name)
{
    if (! pageList->pageNames.contains (name))
        pageList->addItem (name, name);
}

Component* PreferencesComponent::createPageForName (const String& name)
{
    if (name == EL_GENERAL_SETTINGS_NAME) {
        return new GeneralSettingsPage (world);
    } else if (name == EL_AUDIO_ENGINE_PREFERENCE_NAME) {
        return new AudioSettingsComponent (world.getDeviceManager());
    } else if (name == EL_PLUGINS_PREFERENCE_NAME) {
        return new PluginSettingsComponent (world);
    }

    return nullptr;
}

void PreferencesComponent::setPage (const String& name)
{
    if (nullptr != pageComponent && name == pageComponent->getName())
        return;

    if (pageComponent)
    {
        removeChildComponent (pageComponent);
    }

    pageComponent = createPageForName (name);
    
    if (pageComponent)
    {
        pageComponent->setName (name);
        addAndMakeVisible (pageComponent);
        pageList->selectRow (pageList->indexOfPage (name));
    }
    else
    {
        pageComponent = new Component (name);
    }
    resized();
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
                 parentClasses="public Component" constructorParams="Globals&amp; g"
                 variableInitialisers="world (g)" snapPixels="4" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="600"
                 initialHeight="500">
  <BACKGROUND backgroundColour="3b3b3b"/>
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

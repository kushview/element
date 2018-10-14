/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/UnlockStatus.h"
#include "gui/GuiCommon.h"
#include "gui/UnlockForm.h"
#include "Globals.h"
#include "Settings.h"

#define EL_GENERAL_SETTINGS_NAME "General"
#define EL_AUDIO_SETTINGS_NAME "Audio"
#define EL_MIDI_SETTINGS_NAME "MIDI"
#define EL_PLUGINS_PREFERENCE_NAME  "Plugins"
#define EL_LICENSE_SETTINGS_NAME "License"
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
        SettingsPage() = default;
        virtual ~SettingsPage() { }
    };

    // MARK: Plugin Settings (included in general)

    class PluginSettingsComponent : public SettingsPage,
                                    public Button::Listener
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
                toggle->setName (f);
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
            activeFormats.setFont (Font (15, Font::bold));
            activeFormats.setBounds (r.removeFromTop (18));
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
            restoreSetting();
        }

    private:
        PluginManager&  plugins;
        Settings&       settings;

        Label activeFormats;

        OwnedArray<ToggleButton> formatToggles;
        StringArray availableFormats;

        Label formatNotice;

        const String key = Settings::pluginFormatsKey;
        bool hasChanged = false;

        String nameForFormat (const String& name)
        {
            if (name == "AudioUnit")
                return "Audio Unit";
            return name;
        }

        void updateToggleStates()
        {
            restoreSetting();
        }

        void restoreSetting()
        {
            StringArray toks;
            toks.addTokens (settings.getUserSettings()->getValue(key), ",", "'");
            for (auto* c : formatToggles)
                c->setToggleState (toks.contains(c->getName()), dontSendNotification);
        }

        void writeSetting()
        {
            StringArray toks;
            for (auto* c : formatToggles)
                if (c->getToggleState())
                    toks.add (c->getName());

            toks.trim();
            const auto value = toks.joinIntoString(",");
            settings.getUserSettings()->setValue (key, value);
            settings.saveIfNeeded();
        }
    };

    // MARK: General Settings

    class GeneralSettingsPage : public SettingsPage,
                                public Value::Listener
    {
    public:
        enum ComboBoxIDs
        {
            ClockSourceInternal  = 1,
            ClockSourceMidiClock = 2
        };

        GeneralSettingsPage (Globals& world, GuiController& g)
            : pluginSettings (world),
              settings (world.getSettings()),
              engine (world.getAudioEngine()),
              status (world.getUnlockStatus()),
              gui (g)
        {
            addAndMakeVisible (clockSourceLabel);
            clockSourceLabel.setText ("Clock Source", dontSendNotification);
            clockSourceLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (clockSourceBox);
            clockSourceBox.addItem ("Internal", ClockSourceInternal);
            clockSourceBox.addItem ("MIDI Clock", ClockSourceMidiClock);
            clockSource.referTo (clockSourceBox.getSelectedIdAsValue());

            addAndMakeVisible (checkForUpdatesLabel);
            checkForUpdatesLabel.setText ("Check for updates on startup", dontSendNotification);
            checkForUpdatesLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible(checkForUpdates);
            checkForUpdates.setClickingTogglesState (true);
            checkForUpdates.setToggleState (settings.checkForUpdates(), dontSendNotification);
            checkForUpdates.getToggleStateValue().addListener (this);

            addAndMakeVisible (scanForPlugsLabel);
            scanForPlugsLabel.setText ("Scan plugins on startup", dontSendNotification);
            scanForPlugsLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (scanForPlugins);
            scanForPlugins.setClickingTogglesState (true);
            scanForPlugins.setToggleState (settings.scanForPluginsOnStartup(), dontSendNotification);
            scanForPlugins.getToggleStateValue().addListener (this);

            addAndMakeVisible (showPluginWindowsLabel);
            showPluginWindowsLabel.setText ("Automatically show plugin windows", dontSendNotification);
            showPluginWindowsLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (showPluginWindows);
            showPluginWindows.setClickingTogglesState (true);
            showPluginWindows.setToggleState (settings.showPluginWindowsWhenAdded(), dontSendNotification);
            showPluginWindows.getToggleStateValue().addListener (this);

            addAndMakeVisible (pluginWindowsOnTopLabel);
            pluginWindowsOnTopLabel.setText ("Plugin windows on top by default", dontSendNotification);
            pluginWindowsOnTopLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (pluginWindowsOnTop);
            pluginWindowsOnTop.setClickingTogglesState (true);
            pluginWindowsOnTop.setToggleState (settings.pluginWindowsOnTop(), dontSendNotification);
            pluginWindowsOnTop.getToggleStateValue().addListener (this);

            addAndMakeVisible (openLastSessionLabel);
            openLastSessionLabel.setText ("Open last used Session", dontSendNotification);
            openLastSessionLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (openLastSession);
            openLastSession.setClickingTogglesState (true);
            openLastSession.setToggleState (settings.openLastUsedSession(), dontSendNotification);
            openLastSession.getToggleStateValue().addListener (this);

            if (status.isFullVersion())
            {
                const int source = String("internal") == settings.getUserSettings()->getValue("clockSource")
                    ? ClockSourceInternal : ClockSourceMidiClock;
                clockSource.setValue (source);
                clockSource.addListener (this);
            }
            else
            {
                clockSource.setValue ((int) ClockSourceInternal);
                clockSourceBox.setEnabled (false);
            }

            // addAndMakeVisible (pluginSettings);
        }

        virtual ~GeneralSettingsPage() noexcept
        {
            clockSource.removeListener (this);
        }

        void layoutSetting (Rectangle<int>& r, Label& label, SettingButton& setting)
        {
            const int spacingBetweenSections = 6;
            const int settingHeight = 22;
            const int toggleWidth = 40;
            const int toggleHeight = 18;

            r.removeFromTop (spacingBetweenSections);
            auto r2 = r.removeFromTop (settingHeight);
            label.setBounds (r2.removeFromLeft (getWidth() / 2));
            setting.setBounds (r2.removeFromLeft (toggleWidth)
                                 .withSizeKeepingCentre (toggleWidth, toggleHeight));
        }

        void resized() override
        {
            const int spacingBetweenSections = 6;
            const int settingHeight = 22;
            const int toggleWidth = 40;
            const int toggleHeight = 18;

            Rectangle<int> r (getLocalBounds());
            auto r2 = r.removeFromTop (settingHeight);
            clockSourceLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
            clockSourceBox.setBounds (r2.withSizeKeepingCentre (r2.getWidth(), settingHeight));

            r.removeFromTop (spacingBetweenSections);
            r2 = r.removeFromTop (settingHeight);
            checkForUpdatesLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
            checkForUpdates.setBounds (r2.removeFromLeft (toggleWidth)
                                         .withSizeKeepingCentre (toggleWidth, toggleHeight));

            r.removeFromTop (spacingBetweenSections);
            r2 = r.removeFromTop (settingHeight);
            scanForPlugsLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
            scanForPlugins.setBounds (r2.removeFromLeft (toggleWidth)
                                        .withSizeKeepingCentre (toggleWidth, toggleHeight));

            layoutSetting (r, showPluginWindowsLabel, showPluginWindows);
            layoutSetting (r, pluginWindowsOnTopLabel, pluginWindowsOnTop);
            layoutSetting (r, openLastSessionLabel, openLastSession);

            if (pluginSettings.isVisible())
            {
                r.removeFromTop (spacingBetweenSections * 2);
                pluginSettings.setBounds (r);
            }
        }

        void valueChanged (Value& value) override
        {
            if (value.refersToSameSourceAs (checkForUpdates.getToggleStateValue()))
            {
                settings.setCheckForUpdates (checkForUpdates.getToggleState());
                jassert (settings.checkForUpdates() == checkForUpdates.getToggleState());
            }

            // clock source
            else if (value.refersToSameSourceAs (clockSource) && status.isFullVersion())
            {
                if (! status.isFullVersion())
                    return;

                const var val = ClockSourceInternal == (int)clockSource.getValue() ? "internal" : "midiClock";
                settings.getUserSettings()->setValue ("clockSource", val);
                engine->applySettings (settings);
            }

            else if (value.refersToSameSourceAs (scanForPlugins.getToggleStateValue()))
            {
                settings.setScanForPluginsOnStartup (scanForPlugins.getToggleState());
            }
            else if (value.refersToSameSourceAs (showPluginWindows.getToggleStateValue()))
            {
                settings.setShowPluginWindowsWhenAdded (showPluginWindows.getToggleState());
            }
            else if (value.refersToSameSourceAs (openLastSession.getToggleStateValue()))
            {
                settings.setOpenLastUsedSession (openLastSession.getToggleState());
            }
            else if (value.refersToSameSourceAs (pluginWindowsOnTop.getToggleStateValue()))
            {
                settings.setPluginWindowsOnTop (pluginWindowsOnTop.getToggleState());
            }

            settings.saveIfNeeded();
            gui.stabilizeContent();
        }

    private:
        Label clockSourceLabel;
        ComboBox clockSourceBox;
        Value clockSource;

        Label checkForUpdatesLabel;
        SettingButton checkForUpdates;

        Label scanForPlugsLabel;
        SettingButton scanForPlugins;

        PluginSettingsComponent pluginSettings;

        Label showPluginWindowsLabel;
        SettingButton showPluginWindows;
        
        Label pluginWindowsOnTopLabel;
        SettingButton pluginWindowsOnTop;

        Label openLastSessionLabel;
        SettingButton openLastSession;

        Settings& settings;
        AudioEnginePtr engine;
        UnlockStatus& status;
        GuiController& gui;
    };

    // MARK: Audio Settings

    class AudioSettingsComponent : public SettingsPage
    {
    public:
        AudioSettingsComponent (DeviceManager& d)
            : devs (d, 0, DeviceManager::maxAudioChannels, 0, 
                          DeviceManager::maxAudioChannels, 
                          false, false, true, false),
              devices (d)
        {
            addAndMakeVisible (devs);
            devs.setItemHeight (22);
            setSize (300, 400);
        }

        ~AudioSettingsComponent()
        {
        }

        void resized() override { devs.setBounds (getLocalBounds()); }

    private:
        AudioDeviceSelectorComponent devs;
        DeviceManager& devices;
    };

    // MARK: MIDI Settings

    class MidiSettingsPage : public SettingsPage,
                             public ComboBox::Listener,
                             public Button::Listener,
                             public ChangeListener
    {
    public:
        MidiSettingsPage (Globals& world)
            : devices (world.getDeviceManager()),
              settings (world.getSettings())
        {
            addAndMakeVisible (midiOutputLabel);
            midiOutputLabel.setFont (Font (12.0, Font::bold));
            midiOutputLabel.setText ("MIDI Output Device", dontSendNotification);

            addAndMakeVisible (midiOutput);
            midiOutput.addListener (this);

            addAndMakeVisible(midiInputHeader);
            midiInputHeader.setText ("Active MIDI Inputs", dontSendNotification);
            midiInputHeader.setFont (Font (12, Font::bold));

            midiInputs = new MidiInputs (*this);
            midiInputView.setViewedComponent (midiInputs.get(), false);
            addAndMakeVisible (midiInputView);

            setSize (300, 400);

            devices.addChangeListener (this);
            updateDevices();
        }

        ~MidiSettingsPage()
        {
            devices.removeChangeListener (this);
            midiInputs = nullptr;
            midiOutput.removeListener (this);
        }

        void resized() override
        {
            const int spacingBetweenSections = 6;
            const int settingHeight = 22;

            Rectangle<int> r (getLocalBounds());
            auto r2 = r.removeFromTop (settingHeight);
            midiOutputLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
            midiOutput.setBounds (r2.withSizeKeepingCentre (r2.getWidth(), settingHeight));

            r.removeFromTop (roundToInt ((double) spacingBetweenSections * 1.5));
            midiInputHeader.setBounds (r.removeFromTop (24));

            midiInputView.setBounds (r);
            midiInputs->updateSize();
        }

        void buttonClicked (Button*) override
        {

        }

        void comboBoxChanged (ComboBox* box) override
        {
            const auto name = outputs [midiOutput.getSelectedId() - 10];
            if (box == &midiOutput)
                devices.setDefaultMidiOutput (name);
        }

        void changeListenerCallback (ChangeBroadcaster*) override
        {
            updateDevices();
        }

    private:
        DeviceManager& devices;
        Settings& settings;

        Label midiOutputLabel;
        ComboBox midiOutput;
        Label midiInputHeader;
        StringArray outputs;

        class MidiInputs : public Component,
                           public Button::Listener
        {
        public:
            MidiInputs (MidiSettingsPage& o)
                : owner (o) { }

            void updateDevices()
            {
                midiInputLabels.clearQuick (true);
                midiInputs.clearQuick (true);
                inputs  = MidiInput::getDevices();

                for (const auto& name : inputs)
                {
                    auto* label = midiInputLabels.add (new Label());
                    label->setFont (Font (12));
                    label->setText (name, dontSendNotification);
                    addAndMakeVisible (label);

                    auto* btn = midiInputs.add (new SettingButton());
                    btn->setName (name);
                    btn->setClickingTogglesState (true);
                    btn->setYesNoText ("On", "Off");
                    btn->addListener (this);
                    addAndMakeVisible (btn);
                }

                updateSize();
            }

            void updateSize()
            {
                const int widthOfView = owner.midiInputView.getWidth() - owner.midiInputView.getScrollBarThickness();
                setSize (jmax (200, widthOfView), computeHeight());
            }

            int computeHeight()
            {
                static int tick = 0;

                const int spacingBetweenSections = 6;
                const int settingHeight = 22;

                int h = 1;
                for (int i = 0; i < midiInputs.size(); ++i)
                {
                    h += spacingBetweenSections;
                    h += settingHeight;
                }

                // this makes sure the height is always
                // different and the viewport will refresh
                if (tick == 0)
                    tick = 1;
                else
                    tick = 0;

                return h + tick;
            }

            void resized() override
            {
                const int spacingBetweenSections = 6;
                const int settingHeight = 22;
                const int toggleWidth = 40;
                const int toggleHeight = 18;

                jassert (midiInputLabels.size() == midiInputs.size());
                auto r = getLocalBounds();
                for (int i = 0; i < midiInputs.size(); ++i)
                {
                    r.removeFromTop (spacingBetweenSections);
                    auto r2 = r.removeFromTop (settingHeight);
                    midiInputLabels.getUnchecked(i)->setBounds (r2.removeFromLeft (getWidth() / 2));
                    midiInputs.getUnchecked(i)->setBounds (
                        r2.removeFromLeft(toggleWidth).withSizeKeepingCentre (toggleWidth, toggleHeight));
                }
            }

            void buttonClicked (Button* btn) override
            {
                 if (midiInputs.contains (dynamic_cast<SettingButton*> (btn)))
                 {
                     owner.devices.setMidiInputEnabled (btn->getName(), btn->getToggleState());
                 }
            }

            void updateSelection()
            {
                for (auto* input : midiInputs)
                    input->setToggleState (owner.devices.isMidiInputEnabled(input->getName()), dontSendNotification);
            }

        private:
            friend class MidiSettingsPage;
            MidiSettingsPage& owner;
            StringArray inputs;
            OwnedArray<Label> midiInputLabels;
            OwnedArray<SettingButton> midiInputs;
        };

        friend class MidiInputs;
        ScopedPointer<MidiInputs> midiInputs;
        Viewport midiInputView;

        void updateDevices()
        {
            outputs = MidiOutput::getDevices();
            midiOutput.clear (dontSendNotification);
            midiOutput.setTextWhenNoChoicesAvailable ("<none>");

            int i = 0;
            midiOutput.addItem ("<< none >>", 1);
            midiOutput.addSeparator();
            for (const auto& name : outputs)
            {
                midiOutput.addItem (name, 10 + i);
                ++i;
            }

            midiInputs->updateDevices();

            updateInputSelection();
            updateOutputSelection();

            resized();
        }

        void updateOutputSelection()
        {
            if (auto* out = devices.getDefaultMidiOutput())
                midiOutput.setSelectedId (10 + outputs.indexOf (out->getName()));
            else
                midiOutput.setSelectedId (1);
        }

        void updateInputSelection()
        {
            if (midiInputs)
                midiInputs->updateSelection();
        }
    };

    class LicenseSettingsPage : public SettingsPage
    {
    public:
        LicenseSettingsPage (Globals& world, GuiController& gui)
        {
            setName (EL_LICENSE_SETTINGS_NAME);
            addAndMakeVisible (form = new UnlockForm (world, gui,
                "Enter your license key.", false, false, true, true));
        }

        void resized() override
        {
            form->setBounds (getLocalBounds().reduced (2));
        }

        ~LicenseSettingsPage()
        {
            form = nullptr;
        }

    private:
        ScopedPointer<UnlockForm> form;
    };
//[/MiscUserDefs]

//==============================================================================
PreferencesComponent::PreferencesComponent (Globals& g, GuiController& _gui)
    : world (g), gui (_gui)
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
    addPage (EL_AUDIO_SETTINGS_NAME);
    addPage (EL_MIDI_SETTINGS_NAME);
    addPage (EL_LICENSE_SETTINGS_NAME);
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
        return new GeneralSettingsPage (world, gui);
    } else if (name == EL_AUDIO_SETTINGS_NAME) {
        return new AudioSettingsComponent (world.getDeviceManager());
    } else if (name == EL_PLUGINS_PREFERENCE_NAME) {
        return new PluginSettingsComponent (world);
    } else if (name == EL_MIDI_SETTINGS_NAME) {
        return new MidiSettingsPage (world);
    } else if (name == EL_LICENSE_SETTINGS_NAME) {
        return new LicenseSettingsPage (world, gui);
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
                 parentClasses="public Component" constructorParams="Globals&amp; g, GuiController&amp; _gui"
                 variableInitialisers="world (g), gui(_gui)" snapPixels="4" snapActive="1"
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

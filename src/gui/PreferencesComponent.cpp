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
#include "gui/widgets/AudioDeviceSelectorComponent.h"
#include "gui/ContentComponent.h"
#include "gui/GuiCommon.h"
#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
#include "controllers/OSCController.h"
#include "Globals.h"
#include "Settings.h"

#define EL_GENERAL_SETTINGS_NAME "General"
#define EL_AUDIO_SETTINGS_NAME "Audio"
#define EL_MIDI_SETTINGS_NAME "MIDI"
#define EL_OSC_SETTINGS_NAME "OSC"
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
        SettingsPage() = default;
        virtual ~SettingsPage() { }

    protected:
        virtual void layoutSetting (Rectangle<int>& r, Label& label, Component& setting,
                                    const int valueWidth = -1)
        {
            const int spacingBetweenSections = 6;
            const int settingHeight = 22;
            const int toggleWidth = valueWidth > 0 ? valueWidth : 40;
            const int toggleHeight = 18;

            r.removeFromTop (spacingBetweenSections);
            auto r2 = r.removeFromTop (settingHeight);
            label.setBounds (r2.removeFromLeft (getWidth() / 2));
            setting.setBounds (r2.removeFromLeft (toggleWidth)
                                 .withSizeKeepingCentre (toggleWidth, toggleHeight));
        }
    };

    class OSCSettingsPage : public SettingsPage,
                            private AsyncUpdater
    {
    public:
        OSCSettingsPage (Globals& w, GuiController& g)
            : world (w), gui (g)
        {
            auto& settings = world.getSettings();
            addAndMakeVisible (enabledLabel);
            enabledLabel.setFont (Font (12.0, Font::bold));
            enabledLabel.setText ("OSC Host Enabled?", dontSendNotification);
            addAndMakeVisible (enabledButton);
            enabledButton.setYesNoText ("Yes", "No");
            enabledButton.setClickingTogglesState (true);
            enabledButton.setToggleState (settings.isOscHostEnabled(), dontSendNotification);
            enabledButton.onClick = [this]()
            {
                updateEnablement();
                triggerAsyncUpdate();
            };

            addAndMakeVisible (hostLabel);
            hostLabel.setFont (Font (12.0, Font::bold));
            hostLabel.setText ("OSC Host", dontSendNotification);
            addAndMakeVisible (hostField);
            hostField.setReadOnly (true);
            hostField.setText (IPAddress::getLocalAddress().toString());

            addAndMakeVisible (portLabel);
            portLabel.setFont (Font (12.0, Font::bold));
            portLabel.setText ("OSC Host Port", dontSendNotification);
            addAndMakeVisible (portSlider);
            portSlider.textFromValueFunction = [this](double value) -> String {
                return String (roundToInt (value));
            };
            portSlider.setRange (1.0, 65535.0, 1.0);
            portSlider.setValue ((double) settings.getOscHostPort());
            portSlider.setSliderStyle (Slider::IncDecButtons);
            portSlider.setTextBoxStyle (Slider::TextBoxLeft, false, 82, 22);
            portSlider.onValueChange = [this]()
            {
                world.getSettings().setOscHostPort (roundToInt (portSlider.getValue()));
                triggerAsyncUpdate();
            };
        }

        ~OSCSettingsPage() { }

        void resized() override
        {
            auto r = getLocalBounds();
            layoutSetting (r, enabledLabel, enabledButton);
            layoutSetting (r, hostLabel, hostField, getWidth() / 2);
            layoutSetting (r, portLabel, portSlider, getWidth() / 4);
        }

    private:
        Globals& world;
        GuiController& gui;
        Label enabledLabel;
        SettingButton enabledButton;
        Label hostLabel;
        TextEditor hostField;
        Label portLabel;
        Slider portSlider;

        void handleAsyncUpdate() override
        {
            requestServerUpdate();
        }

        void requestServerUpdate()
        {
            if (auto* const osc = gui.findSibling<OSCController>())
                osc->refreshWithSettings (true);
        }

        void updateEnablement()
        {
            world.getSettings().setOscHostEnabled (enabledButton.getToggleState());
            hostField.setEnabled (enabledButton.getToggleState());
            portSlider.setEnabled (enabledButton.getToggleState());
        }
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
                                public Value::Listener,
                                public FilenameComponentListener,
                                public Button::Listener
    {
    public:
        enum ComboBoxIDs
        {
            ClockSourceInternal  = 1,
            ClockSourceMidiClock = 2
        };

        GeneralSettingsPage (Globals& world, GuiController& g)
            : pluginSettings (world),
             #ifdef EL_PRO
              defaultSessionFile ("Default Session", File(), true, false,
                  false,        // bool isForSaving,
                  "*.els",      //const String& fileBrowserWildcard,
                  "",           //const String& enforcedSuffix,
                  "None"),      //const String& textWhenNothingSelected)
             #else
              defaultSessionFile ("Default Graph", File(), true, false,
                  false,         // bool isForSaving,
                  "*.elg",       //const String& fileBrowserWildcard,
                  "",            //const String& enforcedSuffix,
                  "None"),       //const String& textWhenNothingSelected)
             #endif
              settings (world.getSettings()),
              engine (world.getAudioEngine()),
              gui (g)
        {
            addAndMakeVisible (clockSourceLabel);
            clockSourceLabel.setText ("Clock Source", dontSendNotification);
            clockSourceLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (clockSourceBox);
            clockSourceBox.addItem ("Internal", ClockSourceInternal);
           #if defined (EL_PRO)
            clockSourceBox.addItem ("MIDI Clock", ClockSourceMidiClock);
           #endif
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

            addAndMakeVisible (hidePluginWindowsLabel);
            hidePluginWindowsLabel.setText ("Hide plugin windows when app inactive", dontSendNotification);
            hidePluginWindowsLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (hidePluginWindows);
            hidePluginWindows.setClickingTogglesState (true);
            hidePluginWindows.setToggleState (settings.hidePluginWindowsWhenFocusLost(), dontSendNotification);
            hidePluginWindows.getToggleStateValue().addListener (this);

            addAndMakeVisible (openLastSessionLabel);
           #ifdef EL_PRO
            openLastSessionLabel.setText ("Open last used Session", dontSendNotification);
           #else
            openLastSessionLabel.setText ("Open last used Graph", dontSendNotification);
           #endif
            openLastSessionLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (openLastSession);
            openLastSession.setClickingTogglesState (true);
            openLastSession.setToggleState (settings.openLastUsedSession(), dontSendNotification);
            openLastSession.getToggleStateValue().addListener (this);

            addAndMakeVisible (askToSaveSessionLabel);
           #ifdef EL_PRO
            askToSaveSessionLabel.setText ("Ask to save sessions on exit", dontSendNotification);
           #else
            askToSaveSessionLabel.setText ("Ask to save graphs on exit", dontSendNotification);
           #endif
            askToSaveSessionLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (askToSaveSession);
            askToSaveSession.setClickingTogglesState (true);
            askToSaveSession.setToggleState (settings.askToSaveSession(), dontSendNotification);
            askToSaveSession.getToggleStateValue().addListener (this);

            addAndMakeVisible (systrayLabel);
            systrayLabel.setText ("Show system tray", dontSendNotification);
            systrayLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (systray);
            systray.setClickingTogglesState (true);
            systray.setToggleState (settings.isSystrayEnabled(), dontSendNotification);
            systray.getToggleStateValue().addListener (this);

           #ifdef EL_PRO
            addAndMakeVisible (defaultSessionFileLabel);
            defaultSessionFileLabel.setText ("Default new Session", dontSendNotification);
            defaultSessionFileLabel.setFont (Font (12.0, Font::bold));
            addAndMakeVisible (defaultSessionFile);
            defaultSessionFile.setCurrentFile (settings.getDefaultNewSessionFile(), dontSendNotification);
            defaultSessionFile.addListener (this);
            addAndMakeVisible (defaultSessionClearButton);
            defaultSessionClearButton.setButtonText ("X");
            defaultSessionClearButton.addListener (this);
           #endif

           #if defined (EL_PRO)
            const int source = String("internal") == settings.getUserSettings()->getValue("clockSource")
                ? ClockSourceInternal : ClockSourceMidiClock;
            clockSource.setValue (source);
            clockSource.addListener (this);
           #else
            clockSource.setValue ((int) ClockSourceInternal);
            clockSourceBox.setEnabled (false);
           #endif
        }

        virtual ~GeneralSettingsPage() noexcept
        {
            clockSource.removeListener (this);
        }

        void filenameComponentChanged (FilenameComponent* f) override
        {
            if (f == &defaultSessionFile)
            {
                if (f->getCurrentFile().existsAsFile())
                    settings.setDefaultNewSessionFile (f->getCurrentFile());
                else 
                    settings.setDefaultNewSessionFile (File());
            }

            settings.saveIfNeeded();
        }

        void buttonClicked (Button* b) override
        {
            if (b == &defaultSessionClearButton)
                defaultSessionFile.setCurrentFile (File(), false, sendNotificationAsync);
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
            layoutSetting (r, hidePluginWindowsLabel, hidePluginWindows);
            layoutSetting (r, openLastSessionLabel, openLastSession);
            layoutSetting (r, askToSaveSessionLabel, askToSaveSession);
            layoutSetting (r, systrayLabel, systray);

           #ifdef EL_PRO
            layoutSetting (r, defaultSessionFileLabel, defaultSessionFile, 190 - settingHeight);
            defaultSessionClearButton.setBounds (defaultSessionFile.getRight(),
                                                 defaultSessionFile.getY(),
                                                 settingHeight - 2, defaultSessionFile.getHeight());
           #endif
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
            else if (value.refersToSameSourceAs (clockSource) && true)
            {
                if (! true)
                    return;

                const var val = ClockSourceInternal == (int)clockSource.getValue() ? "internal" : "midiClock";
                settings.getUserSettings()->setValue ("clockSource", val);
                engine->applySettings (settings);
                if (auto* cc = ViewHelpers::findContentComponent())
                    cc->refreshToolbar();
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
            else if (value.refersToSameSourceAs (askToSaveSession.getToggleStateValue()))
            {
                settings.setAskToSaveSession (askToSaveSession.getToggleState());
            }
            else if (value.refersToSameSourceAs (hidePluginWindows.getToggleStateValue()))
            {
                settings.setHidePluginWindowsWhenFocusLost (hidePluginWindows.getToggleState());
            }
            else if (value.refersToSameSourceAs (systray.getToggleStateValue()))
            {
                settings.setSystrayEnabled (systray.getToggleState());
                gui.refreshSystemTray();
            }

            settings.saveIfNeeded();
            gui.stabilizeViews();
            gui.refreshMainMenu();
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

        Label hidePluginWindowsLabel;
        SettingButton hidePluginWindows;

        Label openLastSessionLabel;
        SettingButton openLastSession;

        Label askToSaveSessionLabel;
        SettingButton askToSaveSession;

        Label defaultSessionFileLabel;
        FilenameComponent defaultSessionFile;
        TextButton defaultSessionClearButton;

        Label systrayLabel;
        SettingButton systray;

        Settings& settings;
        AudioEnginePtr engine;
        GuiController& gui;
    };

    // MARK: Audio Settings

    class AudioSettingsComponent : public SettingsPage
    {
    public:
        AudioSettingsComponent (DeviceManager& d)
            : devs (d, 1, DeviceManager::maxAudioChannels,
                       1, DeviceManager::maxAudioChannels, 
                       false, false, false, false),
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
        Element::AudioDeviceSelectorComponent devs;
        DeviceManager& devices;
    };

    // MARK: MIDI Settings

    class MidiSettingsPage : public SettingsPage,
                             public ComboBox::Listener,
                             public Button::Listener,
                             public ChangeListener,
                             public Timer
    {
    public:
        MidiSettingsPage (Globals& g)
            : devices (g.getDeviceManager()),
              settings (g.getSettings()),
              midi (g.getMidiEngine()),
              world (g)
        {
            addAndMakeVisible (midiOutputLabel);
            midiOutputLabel.setFont (Font (12.0, Font::bold));
            midiOutputLabel.setText ("MIDI Output Device", dontSendNotification);

            addAndMakeVisible (midiOutput);
            midiOutput.addListener (this);

           #if defined (EL_PRO)
            addAndMakeVisible (generateClockLabel);
            generateClockLabel.setFont (Font (12.0, Font::bold));
            generateClockLabel.setText ("Generate MIDI Clock", dontSendNotification);
            addAndMakeVisible (generateClock);
            generateClock.setYesNoText ("Yes", "No");
            generateClock.setClickingTogglesState (true);
            generateClock.setToggleState (settings.generateMidiClock(), dontSendNotification);
            generateClock.addListener (this);

            addAndMakeVisible (sendClockToInputLabel);
            sendClockToInputLabel.setFont (Font (12.0, Font::bold));
            sendClockToInputLabel.setText ("Send Clock to MIDI Input?", dontSendNotification);
            addAndMakeVisible (sendClockToInput);
            sendClockToInput.setYesNoText ("Yes", "No");
            sendClockToInput.setClickingTogglesState (true);
            sendClockToInput.setToggleState (settings.sendMidiClockToInput(), dontSendNotification);
            sendClockToInput.addListener (this);
           #endif
            
            addAndMakeVisible(midiInputHeader);
            midiInputHeader.setText ("Active MIDI Inputs", dontSendNotification);
            midiInputHeader.setFont (Font (12, Font::bold));

            midiInputs = new MidiInputs (*this);
            midiInputView.setViewedComponent (midiInputs.get(), false);
            addAndMakeVisible (midiInputView);

            setSize (300, 400);

            devices.addChangeListener (this);
            updateDevices();
            startTimer (1 * 1000); // refresh if needed every 1 second
        }

        ~MidiSettingsPage()
        {
            devices.removeChangeListener (this);
            midiInputs = nullptr;
            midiOutput.removeListener (this);
        }

        void timerCallback() override
        {
            if ((midiInputs && midiInputs->getNumDevices() != MidiInput::getDevices().size()) ||
                midiOutput.getNumItems() - 1 != MidiOutput::getDevices().size())
            {
                updateDevices();
            }
        }

        void resized() override
        {
            const int spacingBetweenSections = 6;
            const int settingHeight = 22;

            Rectangle<int> r (getLocalBounds());
            auto r2 = r.removeFromTop (settingHeight);
            midiOutputLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
            midiOutput.setBounds (r2.withSizeKeepingCentre (r2.getWidth(), settingHeight));
           #if defined (EL_PRO)
            layoutSetting (r, generateClockLabel, generateClock);
            layoutSetting (r, sendClockToInputLabel, sendClockToInput);
           #endif
            r.removeFromTop (roundToInt ((double) spacingBetweenSections * 1.5));
            midiInputHeader.setBounds (r.removeFromTop (24));

            midiInputView.setBounds (r);
            midiInputs->updateSize();
        }

        void buttonClicked (Button* button) override
        {
            if (button == &generateClock)
            {
                settings.setGenerateMidiClock (generateClock.getToggleState());
                generateClock.setToggleState (settings.generateMidiClock(), dontSendNotification);
                if (auto engine = world.getAudioEngine())
                    engine->applySettings (settings);
            }
            else if (button == &sendClockToInput)
            {
                settings.setSendMidiClockToInput (sendClockToInput.getToggleState());
                sendClockToInput.setToggleState (settings.sendMidiClockToInput(), dontSendNotification);
                if (auto engine = world.getAudioEngine())
                    engine->applySettings (settings);
            }
        }

        void comboBoxChanged (ComboBox* box) override
        {
            const auto name = outputs [midiOutput.getSelectedId() - 10];
            if (box == &midiOutput)
                midi.setDefaultMidiOutput (name);
        }

        void changeListenerCallback (ChangeBroadcaster*) override
        {
            updateDevices();
            for (int i = 0; i < DocumentWindow::getNumTopLevelWindows(); ++i)
                if (auto* main = dynamic_cast<MainWindow*> (DocumentWindow::getTopLevelWindow (i)))
                    main->refreshMenu();
        }

    private:
        DeviceManager& devices;
        Settings& settings;
        MidiEngine& midi;
        Globals& world;

        Label midiOutputLabel;
        ComboBox midiOutput;
        Label generateClockLabel;
        SettingButton generateClock;
        Label sendClockToInputLabel;
        SettingButton sendClockToInput;
        Label midiInputHeader;
        StringArray outputs;

        class MidiInputs : public Component,
                           public Button::Listener
        {
        public:
            MidiInputs (MidiSettingsPage& o)
                : owner (o) { }

            int getNumDevices() const { return midiInputs.size(); }

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
                    owner.midi.setMidiInputEnabled (btn->getName(), btn->getToggleState());
                }
            }

            void updateSelection()
            {
                for (auto* input : midiInputs)
                    input->setToggleState (owner.midi.isMidiInputEnabled(input->getName()), dontSendNotification);
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
            if (auto* out = midi.getDefaultMidiOutput())
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
    addPage (EL_OSC_SETTINGS_NAME);
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
    gui.refreshMainMenu();
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
    } else if (name == EL_OSC_SETTINGS_NAME) {
        return new OSCSettingsPage (world, gui);
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

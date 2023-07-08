// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

//[Headers] You can add your own extra header files here...
#include <element/devices.hpp>
#include <element/plugins.hpp>
#include "gui/widgets/AudioDeviceSelectorComponent.h"
#include <element/ui/content.hpp>
#include "gui/GuiCommon.h"
#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
#include "services/oscservice.hpp"
#include "engine/midiengine.hpp"

#include <element/context.hpp>
#include <element/settings.hpp>

#define EL_GENERAL_SETTINGS_NAME "General"
#define EL_AUDIO_SETTINGS_NAME "Audio"
#define EL_MIDI_SETTINGS_NAME "MIDI"
#define EL_OSC_SETTINGS_NAME "OSC"
#define EL_PLUGINS_PREFERENCE_NAME "Plugins"
#define EL_REPOSITORY_PREFERENCE_NAME "Update Mirrors"

//[/Headers]

#include "PreferencesComponent.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace element {

class PreferencesComponent::PageList : public ListBox,
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

    void paint (Graphics& g)
    {
        g.fillAll (Colors::widgetBackgroundColor.darker (0.45f));
    }

    virtual void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
    {
        if (! isPositiveAndBelow (rowNumber, pageNames.size()))
            return;
        ViewHelpers::drawBasicTextRow (pageNames[rowNumber], g, width, height, rowIsSelected);
    }

    void listBoxItemClicked (int row, const MouseEvent& e)
    {
        if (isPositiveAndBelow (row, pageNames.size()) && page != pageNames[row])
        {
            page = pageNames[row];
            owner.setPage (page);
        }
    }

    virtual String getTooltipForRow (int row)
    {
        String tool (pageNames[row]);
        tool << String (" ") << "settings";
        return tool;
    }

    int indexOfPage (const String& name) const
    {
        return pageNames.indexOf (name);
    }

private:
    friend class PreferencesComponent;

    void addItem (const String& name)
    {
        pageNames.addIfNotAlreadyThere (name);
        updateContent();
    }

    Font font;
    PreferencesComponent& owner;
    StringArray pageNames;
    String page;
};

//==============================================================================
class SettingsPage : public Component
{
public:
    SettingsPage() = default;
    virtual ~SettingsPage() {}

protected:
    virtual void layoutSetting (Rectangle<int>& r, Label& label, Component& setting, const int valueWidth = -1)
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

//==============================================================================
class OSCSettingsPage : public SettingsPage,
                        private AsyncUpdater
{
public:
    OSCSettingsPage (Context& w, GuiService& g)
        : world (w), gui (g)
    {
        auto& settings = world.settings();
        addAndMakeVisible (enabledLabel);
        enabledLabel.setFont (Font (12.0, Font::bold));
        enabledLabel.setText ("OSC Host Enabled?", dontSendNotification);
        addAndMakeVisible (enabledButton);
        enabledButton.setYesNoText ("Yes", "No");
        enabledButton.setClickingTogglesState (true);
        enabledButton.setToggleState (settings.isOscHostEnabled(), dontSendNotification);
        enabledButton.onClick = [this]() {
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
        portSlider.textFromValueFunction = [] (double value) -> String {
            return String (roundToInt (value));
        };
        portSlider.setRange (1.0, 65535.0, 1.0);
        portSlider.setValue ((double) settings.getOscHostPort());
        portSlider.setSliderStyle (Slider::IncDecButtons);
        portSlider.setTextBoxStyle (Slider::TextBoxLeft, false, 82, 22);
        portSlider.onValueChange = [this]() {
            world.settings().setOscHostPort (roundToInt (portSlider.getValue()));
            triggerAsyncUpdate();
        };
    }

    ~OSCSettingsPage() {}

    void resized() override
    {
        auto r = getLocalBounds();
        layoutSetting (r, enabledLabel, enabledButton);
        layoutSetting (r, hostLabel, hostField, getWidth() / 2);
        layoutSetting (r, portLabel, portSlider, getWidth() / 4);
    }

private:
    Context& world;
    GuiService& gui;
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
        if (auto* const osc = gui.sibling<OSCService>())
            osc->refreshWithSettings (true);
    }

    void updateEnablement()
    {
        world.settings().setOscHostEnabled (enabledButton.getToggleState());
        hostField.setEnabled (enabledButton.getToggleState());
        portSlider.setEnabled (enabledButton.getToggleState());
    }
};

//==============================================================================
class PluginSettingsComponent : public SettingsPage,
                                public Button::Listener
{
public:
    PluginSettingsComponent (Context& w)
        : plugins (w.plugins()),
          settings (w.settings())

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
            toggle->setColour (ToggleButton::textColourId, Colors::textColor);
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

    void paint (Graphics&) override {}

    void buttonClicked (Button*) override
    {
        writeSetting();
        restoreSetting();
    }

private:
    PluginManager& plugins;
    Settings& settings;

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
        toks.addTokens (settings.getUserSettings()->getValue (key), ",", "'");
        for (auto* c : formatToggles)
            c->setToggleState (toks.contains (c->getName()), dontSendNotification);
    }

    void writeSetting()
    {
        StringArray toks;
        for (auto* c : formatToggles)
            if (c->getToggleState())
                toks.add (c->getName());

        toks.trim();
        const auto value = toks.joinIntoString (",");
        settings.getUserSettings()->setValue (key, value);
        settings.saveIfNeeded();
    }
};

//==============================================================================
class GeneralSettingsPage : public SettingsPage,
                            public Value::Listener,
                            public FilenameComponentListener,
                            public Button::Listener
{
public:
    enum ComboBoxIDs
    {
        ClockSourceInternal = 1,
        ClockSourceMidiClock = 2
    };

    GeneralSettingsPage (Context& world, GuiService& g)
        : pluginSettings (world),
          defaultSessionFile ("Default Session", File(), true, false,
                              false, // bool isForSaving,
                              "*.els", //const String& fileBrowserWildcard,
                              "", //const String& enforcedSuffix,
                              "None"), //const String& textWhenNothingSelected)

          settings (world.settings()),
          engine (world.audio()),
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
        addAndMakeVisible (checkForUpdates);
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

        openLastSessionLabel.setText ("Open last used Session", dontSendNotification);
        openLastSessionLabel.setFont (Font (12.0, Font::bold));
        addAndMakeVisible (openLastSession);
        openLastSession.setClickingTogglesState (true);
        openLastSession.setToggleState (settings.openLastUsedSession(), dontSendNotification);
        openLastSession.getToggleStateValue().addListener (this);

        addAndMakeVisible (askToSaveSessionLabel);
        askToSaveSessionLabel.setText ("Ask to save sessions on exit", dontSendNotification);
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

        addAndMakeVisible (desktopScaleLabel);
        desktopScaleLabel.setText ("Desktop scale", dontSendNotification);
        desktopScaleLabel.setFont (Font (12.0, Font::bold));
        addAndMakeVisible (desktopScale);
        desktopScale.textFromValueFunction = [] (double value) -> String {
            return String (value, 2);
        };
        desktopScale.setRange (0.1, 8.0, 0.01);
        desktopScale.setValue ((double) settings.getDesktopScale());
        desktopScale.setSliderStyle (Slider::IncDecButtons);
        desktopScale.setTextBoxStyle (Slider::TextBoxLeft, false, 82, 22);
        desktopScale.onValueChange = [this]() {
            settings.setDesktopScale (desktopScale.getValue());
            desktopScale.setValue (settings.getDesktopScale(), dontSendNotification);
            if (settings.getDesktopScale() != Desktop::getInstance().getGlobalScaleFactor())
            {
                Desktop::getInstance().setGlobalScaleFactor (settings.getDesktopScale());
                if (auto* parent = findParentComponentOfClass<PreferencesComponent>())
                    parent->updateSize();
            }
        };

        addAndMakeVisible (defaultSessionFileLabel);
        defaultSessionFileLabel.setText ("Default new Session", dontSendNotification);
        defaultSessionFileLabel.setFont (Font (12.0, Font::bold));
        addAndMakeVisible (defaultSessionFile);
        defaultSessionFile.setCurrentFile (settings.getDefaultNewSessionFile(), dontSendNotification);
        defaultSessionFile.addListener (this);
        addAndMakeVisible (defaultSessionClearButton);
        defaultSessionClearButton.setButtonText ("X");
        defaultSessionClearButton.addListener (this);

        const int source = String ("internal") == settings.getUserSettings()->getValue ("clockSource")
                               ? ClockSourceInternal
                               : ClockSourceMidiClock;
        clockSourceBox.setSelectedId (source, dontSendNotification);
        clockSource.setValue (source);
        clockSource.addListener (this);

        addAndMakeVisible (mainContentLabel);
        mainContentLabel.setText ("UI Type", dontSendNotification);
        mainContentLabel.setFont (Font (12.0, Font::bold));
        addAndMakeVisible (mainContentBox);
        mainContentBox.addItem ("Standard", 1);
        // mainContentBox.addItem ("Workspace", 2);
        if (settings.getMainContentType() == "standard")
            mainContentBox.setSelectedId (1, dontSendNotification);
        else
        {
            jassertfalse;
        } // invalid content type
        mainContentBox.getSelectedIdAsValue().addListener (this);
    }

    virtual ~GeneralSettingsPage() noexcept
    {
        clockSource.removeListener (this);
        mainContentBox.getSelectedIdAsValue().removeListener (this);
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

        r.removeFromTop (spacingBetweenSections);
        r2 = r.removeFromTop (settingHeight);
        mainContentLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
        mainContentBox.setBounds (r2.withSizeKeepingCentre (r2.getWidth(), settingHeight));

        layoutSetting (r, systrayLabel, systray);
        layoutSetting (r, desktopScaleLabel, desktopScale, getWidth() / 4);

        layoutSetting (r, defaultSessionFileLabel, defaultSessionFile, 190 - settingHeight);
        defaultSessionClearButton.setBounds (defaultSessionFile.getRight(),
                                             defaultSessionFile.getY(),
                                             settingHeight - 2,
                                             defaultSessionFile.getHeight());

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
        else if (value.refersToSameSourceAs (clockSource))
        {
            const var val = ClockSourceInternal == (int) clockSource.getValue() ? "internal" : "midiClock";
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
        else if (value.refersToSameSourceAs (mainContentBox.getSelectedIdAsValue()))
        {
            auto uitype = settings.getMainContentType();
            if (1 == mainContentBox.getSelectedId())
                uitype = "standard";

            if (uitype != settings.getMainContentType())
            {
                bool changeType = true;
                if (changeType)
                {
                    settings.setMainContentType (uitype);
                    ViewHelpers::postMessageFor (this, new ReloadMainContentMessage());
                }
                else
                {
                    mainContentBox.setSelectedId (1, dontSendNotification);
                }
            }
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

    Label desktopScaleLabel;
    Slider desktopScale;

    Label mainContentLabel;
    ComboBox mainContentBox;

    Settings& settings;
    AudioEnginePtr engine;
    GuiService& gui;
};

//==============================================================================
class AudioSettingsComponent : public SettingsPage
{
public:
    AudioSettingsComponent (DeviceManager& d)
        : devs (d, 1, DeviceManager::maxAudioChannels, 1, DeviceManager::maxAudioChannels, false, false, false, false),
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
    // element::AudioDeviceSelectorComponent devs;
    juce::AudioDeviceSelectorComponent devs;
    DeviceManager& devices;
};

//==============================================================================
class MidiSettingsPage : public SettingsPage,
                         public ComboBox::Listener,
                         public Button::Listener,
                         public ChangeListener,
                         public Timer
{
public:
    MidiSettingsPage (Context& g)
        : devices (g.devices()),
          settings (g.settings()),
          midi (g.midi()),
          world (g)
    {
        addAndMakeVisible (midiOutputLabel);
        midiOutputLabel.setFont (Font (12.0, Font::bold));
        midiOutputLabel.setText ("MIDI Output Device", dontSendNotification);

        addAndMakeVisible (midiOutput);
        midiOutput.addListener (this);

        addAndMakeVisible (midiOutLatencyLabel);
        midiOutLatencyLabel.setFont (Font (12.0, Font::bold));
        midiOutLatencyLabel.setText ("Output latency (ms)", dontSendNotification);
        addAndMakeVisible (midiOutLatencyLabel);

        addAndMakeVisible (midiOutLatency);
        midiOutLatency.textFromValueFunction = [] (double value) -> String {
            return String (roundToInt (value));
        };
        midiOutLatency.setRange (-1000.0, 1000.0, 1.0);
        midiOutLatency.setValue ((double) settings.getMidiOutLatency());
        midiOutLatency.setSliderStyle (Slider::IncDecButtons);
        midiOutLatency.setTextBoxStyle (Slider::TextBoxLeft, false, 82, 22);
        midiOutLatency.onValueChange = [this]() {
            world.settings().setMidiOutLatency (midiOutLatency.getValue());
            if (auto e = world.audio())
                e->applySettings (world.settings());
        };
#if JUCE_WINDOWS
        midiOutLatencyLabel.setEnabled (false);
        midiOutLatency.setEnabled (false);
#endif

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

        addAndMakeVisible (midiInputHeader);
        midiInputHeader.setText ("Active MIDI Inputs", dontSendNotification);
        midiInputHeader.setFont (Font (12, Font::bold));

        midiInputs = std::make_unique<MidiInputs> (*this);
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
        if ((midiInputs && midiInputs->getNumDevices() != MidiInput::getAvailableDevices().size()) || midiOutput.getNumItems() - 1 != MidiOutput::getAvailableDevices().size())
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
        layoutSetting (r, midiOutLatencyLabel, midiOutLatency, getWidth() / 4);
        layoutSetting (r, generateClockLabel, generateClock);
        layoutSetting (r, sendClockToInputLabel, sendClockToInput);
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
            if (auto engine = world.audio())
                engine->applySettings (settings);
        }
        else if (button == &sendClockToInput)
        {
            settings.setSendMidiClockToInput (sendClockToInput.getToggleState());
            sendClockToInput.setToggleState (settings.sendMidiClockToInput(), dontSendNotification);
            if (auto engine = world.audio())
                engine->applySettings (settings);
        }
    }

    void comboBoxChanged (ComboBox* box) override
    {
        const auto name = outputs[midiOutput.getSelectedId() - 10];
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
    Context& world;

    Label midiOutputLabel;
    ComboBox midiOutput;
    Label midiOutLatencyLabel;
    Slider midiOutLatency;
    Label generateClockLabel;
    SettingButton generateClock;
    Label sendClockToInputLabel;
    SettingButton sendClockToInput;
    Label midiInputHeader;
    Array<MidiDeviceInfo> outputs;

    class MidiInputs : public Component,
                       public Button::Listener
    {
    public:
        MidiInputs (MidiSettingsPage& o)
            : owner (o) {}

        int getNumDevices() const { return midiInputs.size(); }

        void updateDevices()
        {
            midiInputLabels.clearQuick (true);
            midiInputs.clearQuick (true);
            inputs = MidiInput::getAvailableDevices();

            for (const auto& d : inputs)
            {
                auto* label = midiInputLabels.add (new Label());
                label->setFont (Font (12));
                label->setText (d.name, dontSendNotification);
                addAndMakeVisible (label);

                auto* btn = midiInputs.add (new SettingButton());
                btn->setName (d.name);
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
                midiInputLabels.getUnchecked (i)->setBounds (r2.removeFromLeft (getWidth() / 2));
                midiInputs.getUnchecked (i)->setBounds (
                    r2.removeFromLeft (toggleWidth).withSizeKeepingCentre (toggleWidth, toggleHeight));
            }
        }

        void buttonClicked (Button* btn) override
        {
            auto sb = dynamic_cast<SettingButton*> (btn);
            if (midiInputs.contains (sb))
            {
                owner.midi.setMidiInputEnabled (inputs[midiInputs.indexOf (sb)],
                                                btn->getToggleState());
            }
        }

        void updateSelection()
        {
            for (auto* input : midiInputs)
            {
                int idx = midiInputs.indexOf (input);
                input->setToggleState (owner.midi.isMidiInputEnabled (inputs[idx]), dontSendNotification);
            }
        }

    private:
        friend class MidiSettingsPage;
        MidiSettingsPage& owner;
        Array<MidiDeviceInfo> inputs;
        OwnedArray<Label> midiInputLabels;
        OwnedArray<SettingButton> midiInputs;
    };

    friend class MidiInputs;
    std::unique_ptr<MidiInputs> midiInputs;
    Viewport midiInputView;

    void updateDevices()
    {
        outputs = MidiOutput::getAvailableDevices();
        midiOutput.clear (dontSendNotification);
        midiOutput.setTextWhenNoChoicesAvailable ("<none>");

        int i = 0;
        midiOutput.addItem ("<< none >>", 1);
        midiOutput.addSeparator();
        for (const auto& d : outputs)
        {
            midiOutput.addItem (d.name, 10 + i);
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
            midiOutput.setSelectedId (10 + outputs.indexOf (out->getDeviceInfo()));
        else
            midiOutput.setSelectedId (1);
    }

    void updateInputSelection()
    {
        if (midiInputs)
            midiInputs->updateSelection();
    }
};

//==============================================================================
class MirrorsSettingsPage : public SettingsPage,
                            public TableListBoxModel,
                            private AsyncUpdater
{
    GuiService& _ui;

public:
    MirrorsSettingsPage (GuiService& ui)
        : _ui (ui)
    {
        addAndMakeVisible (_table);
        auto& header = _table.getHeader();

        header.addColumn ("Enabled", 1, 50, 50, 50, TableHeaderComponent::notResizable);
        header.addColumn ("Host", 2, 150, 75, -1, TableHeaderComponent::notSortable);
        header.addColumn ("Username", 3, 75, 60, -1, TableHeaderComponent::notSortable);
        header.addColumn ("Password", 4, 100, 60, -1, TableHeaderComponent::notSortable);
        _table.setHeaderHeight (22);
        _table.setRowHeight (20);
        _table.setModel (this);
        updateRepo();

        addAndMakeVisible (checkButton);
        checkButton.onClick = [this]() {
            _ui.checkUpdates();
        };

        addAndMakeVisible (addButton);
        addButton.onClick = [this]() {
            _repos.push_back ({});
            _table.updateContent();
            _table.selectRow (getNumRows() - 1);
            _table.repaint();
            saveRepos();
        };

        addAndMakeVisible (removeButton);
        removeButton.onClick = [this]() {
            auto selected = (size_t) _table.getSelectedRow();
            if (selected >= 0 && selected < _repos.size())
            {
                _repos.erase (_repos.begin() + selected);
                _table.updateContent();
                _table.repaint();
                saveRepos();
            }
        };

        setSize (30 + 360 + 220 + 220, 500);
    }

    ~MirrorsSettingsPage()
    {
        _table.setModel (nullptr);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto rb = r.removeFromBottom (24);
        rb.removeFromLeft (2);
        checkButton.setBounds (rb.removeFromLeft (90));
        rb.removeFromRight (2);
        removeButton.setBounds (rb.removeFromRight (90));
        rb.removeFromRight (3);
        addButton.setBounds (rb.removeFromRight (90));

        r.removeFromBottom (4);
        _table.setBounds (r);
    }

    int getNumRows() override { return (int) _repos.size(); }

    void paintRowBackground (Graphics& g, int r, int w, int h, bool selected) override
    {
        ViewHelpers::drawBasicTextRow ({}, g, w, h, selected);
    }

    void paintCell (Graphics& g, int r, int cID, int w, int h, bool selected) override
    {
        if (r >= (int) _repos.size())
            return;

        String text = "";
        const auto& repo = _repos.at (r);
        switch (cID)
        {
            case 1:
                text.clear();
                getLookAndFeel().drawTickBox (g, *this, (w / 2) - (h / 2) + 1, 1, h - 2, h - 2, repo.enabled, true, selected, false);
                break;
            case 2:
                text = _repos.at (r).host;
                break;
            case 3:
                text = _repos.at (r).username;
                break;
            case 4: {
                for (int i = (int) _repos.at (r).password.length(); --i >= 0;)
                    text << (juce_wchar) 0x2022;
                break;
            }
        }

        if (text.isNotEmpty())
        {
            g.setColour (Colors::textColor);
            g.drawText (text, 0, 0, w, h, Justification::centred, true);
        }
        else
        {
        }
    }

    void cellClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
        bool needsSaved = false;
        if (columnId == 1)
        {
            _repos.at (rowNumber).enabled = ! _repos.at (rowNumber).enabled;
            needsSaved = true;
        }
        _table.selectRow (rowNumber);
        _table.repaintRow (rowNumber);

        if (needsSaved)
            saveRepos();
    }

    String getCellTooltip (int rowNumber, int columnId) override
    {
        if (columnId == 2)
        {
            return String (_repos.at (rowNumber).host);
        }
        return TableListBoxModel::getCellTooltip (rowNumber, columnId);
    }
    Component* refreshComponentForCell (int row, int column, bool selected, Component* existing) override
    {
        if (column == 1)
            return nullptr;
        ValueLabel* vlab = dynamic_cast<ValueLabel*> (existing);
        if (vlab == nullptr)
            vlab = new ValueLabel (*this);
        vlab->update (row, column);
        return vlab;
    }

#if 0
    virtual Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate);
   
    virtual void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);
    virtual int getColumnAutoSizeWidth (int columnId);
    
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows);
#endif
private:
    juce::TableListBox _table;
    juce::TextButton addButton { "Add" },
        removeButton { "Remove" },
        checkButton { "Check..." };

    struct Repo
    {
        std::string host;
        std::string username;
        std::string password;
        bool enabled { false };
    };
    std::vector<Repo> _repos;

    friend class ValueLabel;
    class ValueLabel : public juce::TextEditor
    {
        MirrorsSettingsPage& owner;

    public:
        ValueLabel() = delete;
        ValueLabel (MirrorsSettingsPage& o)
            : owner (o)
        {
            setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
            setColour (TextEditor::outlineColourId, Colours::transparentBlack);
        }

        virtual ~ValueLabel()
        {
            onTextChange = nullptr;
        }

        void update (int r, int c)
        {
            column = c;
            row = r;

            if (row >= (int) owner._repos.size())
                return;

            auto repo = owner._repos.at (row);

            setText ("", dontSendNotification);
            setCaretPosition (0);
            switch (column)
            {
                case 2:
                    insertTextAtCaret (repo.host);
                    break;
                case 3:
                    insertTextAtCaret (repo.username);
                    break;
                case 4:
                    setPasswordCharacter ((juce_wchar) 0x2022);
                    insertTextAtCaret (repo.password);
                    break;
            }

            onTextChange = std::bind (&ValueLabel::textWasChanged, this);
        }

        void mouseDown (const MouseEvent& ev) override
        {
            owner._table.selectRow (row);
            TextEditor::mouseDown (ev);
        }

    private:
        void textWasChanged()
        {
            if (row >= (int) owner._repos.size())
                return;

            auto& repos = owner._repos;

            switch (column)
            {
                case 2:
                    repos.at (row).host = getText().trim().toStdString();
                    break;
                case 3:
                    repos.at (row).username = getText().trim().toStdString();
                    break;
                case 4:
                    repos.at (row).password = getText().trim().toStdString();
                    break;
            }

            owner.triggerAsyncUpdate();
        }

        int column { 0 };
        int row { 0 };
    };

    void handleAsyncUpdate() override { saveRepos(); }

    static std::unique_ptr<XmlElement> readNetworkFile()
    {
        auto file = networkFile();
        if (! file.existsAsFile())
            return nullptr;
        return XmlDocument::parse (file);
    }

    void saveRepos()
    {
        auto xml = readNetworkFile();
        if (xml == nullptr)
            return;

        if (auto repos = xml->getChildByName ("Repositories"))
        {
            xml->removeChildElement (repos, true);
            repos = xml->createNewChildElement ("Repositories");

            for (const auto& repo : _repos)
            {
                auto r = repos->createNewChildElement ("Repository");
                if (auto c = r->createNewChildElement ("Host"))
                    c->addTextElement (repo.host);
                if (auto c = r->createNewChildElement ("Username"))
                    c->addTextElement (repo.username);
                if (auto c = r->createNewChildElement ("Password"))
                    c->addTextElement (repo.password);
                if (auto c = r->createNewChildElement ("Enabled"))
                    c->addTextElement (repo.enabled ? "1" : "0");
            }
        }

        XmlElement::TextFormat format;
        format.addDefaultHeader = true;
        format.customEncoding = "UTF-8";
        if (! xml->writeTo (networkFile(), format))
        {
            const auto fn = networkFile().getFileName().toStdString();
            std::clog << "[element] failed to write " << fn << std::endl;
        }
    }

    void updateRepo()
    {
        if (auto xml = readNetworkFile())
        {
            setEnabled (true);
            _repos.clear();

            if (auto xml2 = xml->getChildByName ("Repositories"))
            {
                for (const auto* const e : xml2->getChildIterator())
                {
                    Repo repo;

                    if (auto c = e->getChildByName ("Host"))
                        repo.host = c->getAllSubText().toStdString();
                    if (auto c = e->getChildByName ("Username"))
                        repo.username = c->getAllSubText().toStdString();
                    if (auto c = e->getChildByName ("Password"))
                        repo.password = c->getAllSubText().toStdString();

                    if (auto c = e->getChildByName ("Enabled"))
                    {
                        auto st = c->getAllSubText();
                        repo.enabled = st.getIntValue() != 0;
                    }

                    if (! repo.host.empty())
                    {
                        std::clog << "[element] found repo: " << repo.host << std::endl;
                        _repos.push_back (repo);
                    }
                }
            }
        }
        else
        {
            setEnabled (false);
        }

        _table.updateContent();
        repaint();
        resized();
    }

    static File networkFile() noexcept
    {
        return DataPath::applicationDataDir().getChildFile ("installer/network.xml");
    }
};

//==============================================================================
PreferencesComponent::PreferencesComponent (GuiService& ui)
    : _context (ui.context()), _ui (ui)
{
    pageList = std::make_unique<PageList> (*this);
    addAndMakeVisible (pageList.get());
    pageList->setName ("Page List");

    pageComponent = std::make_unique<Component>();
    addAndMakeVisible (pageComponent.get());
    pageComponent->setName ("new component");

    updateSize();

    addPage (EL_GENERAL_SETTINGS_NAME);
    addPage (EL_AUDIO_SETTINGS_NAME);
    addPage (EL_MIDI_SETTINGS_NAME);
    addPage (EL_OSC_SETTINGS_NAME);

    setPage (EL_GENERAL_SETTINGS_NAME);
}

PreferencesComponent::~PreferencesComponent()
{
    pageList = nullptr;
    pageComponent = nullptr;
    pages.clear();
    _ui.refreshMainMenu();
}

//==============================================================================
void PreferencesComponent::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor);
}

void PreferencesComponent::resized()
{
    pageList->setBounds (8, 8, 184, 480);
    pageComponent->setBounds (208, 32, 376, 448);
}

bool PreferencesComponent::keyPressed (const KeyPress& key)
{
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getModifiers().isAltDown() && (key.getKeyCode() == 'm' || key.getKeyCode() == 'M'))
    {
        bool haveMirrors = false;
        for (auto child : pageList->getChildren())
        {
            if (nullptr != dynamic_cast<MirrorsSettingsPage*> (child))
            {
                haveMirrors = true;
                break;
            }
        }

        if (! haveMirrors)
        {
            addPage (EL_REPOSITORY_PREFERENCE_NAME);
            return true;
        }
    }
    return false;
}

void PreferencesComponent::addPage (const String& name)
{
    if (! pageList->pageNames.contains (name))
        pageList->addItem (name);
}

Component* PreferencesComponent::createPageForName (const String& name)
{
    if (name == EL_GENERAL_SETTINGS_NAME)
    {
        return new GeneralSettingsPage (_context, _ui);
    }
    else if (name == EL_AUDIO_SETTINGS_NAME)
    {
        return new AudioSettingsComponent (_context.devices());
    }
    else if (name == EL_PLUGINS_PREFERENCE_NAME)
    {
        return new PluginSettingsComponent (_context);
    }
    else if (name == EL_MIDI_SETTINGS_NAME)
    {
        return new MidiSettingsPage (_context);
    }
    else if (name == EL_OSC_SETTINGS_NAME)
    {
        return new OSCSettingsPage (_context, _ui);
    }
    else if (name == EL_REPOSITORY_PREFERENCE_NAME)
    {
        return new MirrorsSettingsPage (_ui);
    }

    return nullptr;
}

void PreferencesComponent::setPage (const String& name)
{
    if (nullptr != pageComponent && name == pageComponent->getName())
        return;

    if (pageComponent)
    {
        removeChildComponent (pageComponent.get());
    }

    pageComponent.reset (createPageForName (name));

    if (pageComponent)
    {
        pageComponent->setName (name);
        addAndMakeVisible (pageComponent.get());
        pageList->selectRow (pageList->indexOfPage (name));
    }
    else
    {
        pageComponent = std::make_unique<Component> (name);
    }
    resized();
}

void PreferencesComponent::updateSize()
{
    setSize (600, 500);
}

} /* namespace element */

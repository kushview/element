// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>
#include <element/ui.hpp>
#include <element/ui/content.hpp>
#include <element/ui/mainwindow.hpp>
#include <element/ui/preferences.hpp>
#include <element/ui/updater.hpp>
#include <element/version.hpp>

#include "engine/midiengine.hpp"
#include "engine/midipanic.hpp"
#include "messages.hpp"
#include "services/oscservice.hpp"
#include "ui/buttons.hpp"
#include "ui/viewhelpers.hpp"

namespace element {

class Preferences::PageList : public ListBox,
                              public ListBoxModel
{
public:
    PageList (Preferences& prefs)
        : owner (prefs),
          font (FontOptions (16.0f))
    {
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
    friend class Preferences;

    void addItem (const String& name)
    {
        pageNames.addIfNotAlreadyThere (name);
        updateContent();
    }

    Font font;
    Preferences& owner;
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
    virtual void layoutSetting (Rectangle<int>& r, Label& label, Component& setting, const int valueWidth = -1, const int keyWidth = -1)
    {
        const int spacingBetweenSections = 6;
        const int settingHeight = 22;
        const int toggleWidth = valueWidth > 0 ? valueWidth : 40;
        const int toggleHeight = 18;
        const int labelWidth = keyWidth > 0 ? keyWidth : getWidth() / 2;

        r.removeFromTop (spacingBetweenSections);
        auto r2 = r.removeFromTop (settingHeight);
        label.setBounds (r2.removeFromLeft (labelWidth));
        r2 = r2.removeFromLeft (toggleWidth);
        if (nullptr != dynamic_cast<SettingButton*> (&setting))
            r2 = r2.withSizeKeepingCentre (toggleWidth, toggleHeight);
        setting.setBounds (r2);
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
        enabledLabel.setFont (Font (FontOptions (12.0, Font::bold)));
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
        hostLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        hostLabel.setText ("OSC Host", dontSendNotification);
        addAndMakeVisible (hostField);
        hostField.setReadOnly (true);
        hostField.setText (IPAddress::getLocalAddress().toString());

        addAndMakeVisible (portLabel);
        portLabel.setFont (Font (FontOptions (12.0, Font::bold)));
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
        activeFormats.setFont (Font (FontOptions (18.0, Font::bold)));
        addAndMakeVisible (formatNotice);
        formatNotice.setText ("Note: enabled format changes take effect upon restart", dontSendNotification);
        formatNotice.setFont (Font (FontOptions (12.0, Font::italic)));
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
        activeFormats.setFont (Font (FontOptions (15.0f).withStyle ("Bold")));
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
    [[maybe_unused]] bool hasChanged = false;

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
        clockSourceLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (clockSourceBox);
        clockSourceBox.addItem ("Internal", ClockSourceInternal);
        clockSourceBox.addItem ("MIDI Clock", ClockSourceMidiClock);
        clockSource.referTo (clockSourceBox.getSelectedIdAsValue());
#if ELEMENT_UPDATER
        addAndMakeVisible (checkForUpdatesLabel);
        checkForUpdatesLabel.setText ("Check for updates on startup", dontSendNotification);
        checkForUpdatesLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (checkForUpdates);
        checkForUpdates.setClickingTogglesState (true);
        checkForUpdates.setToggleState (settings.checkForUpdates(), dontSendNotification);
        checkForUpdates.getToggleStateValue().addListener (this);
#endif
        addAndMakeVisible (scanForPlugsLabel);
        scanForPlugsLabel.setText ("Scan plugins on startup", dontSendNotification);
        scanForPlugsLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (scanForPlugins);
        scanForPlugins.setClickingTogglesState (true);
        scanForPlugins.setToggleState (settings.scanForPluginsOnStartup(), dontSendNotification);
        scanForPlugins.getToggleStateValue().addListener (this);

        addAndMakeVisible (showPluginWindowsLabel);
        showPluginWindowsLabel.setText ("Automatically show plugin windows", dontSendNotification);
        showPluginWindowsLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (showPluginWindows);
        showPluginWindows.setClickingTogglesState (true);
        showPluginWindows.setToggleState (settings.showPluginWindowsWhenAdded(), dontSendNotification);
        showPluginWindows.getToggleStateValue().addListener (this);

        addAndMakeVisible (pluginWindowsOnTopLabel);
        pluginWindowsOnTopLabel.setText ("Plugin windows on top by default", dontSendNotification);
        pluginWindowsOnTopLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (pluginWindowsOnTop);
        pluginWindowsOnTop.setClickingTogglesState (true);
        pluginWindowsOnTop.setToggleState (settings.pluginWindowsOnTop(), dontSendNotification);
        pluginWindowsOnTop.getToggleStateValue().addListener (this);

        addAndMakeVisible (hidePluginWindowsLabel);
        hidePluginWindowsLabel.setText ("Hide plugin windows when app inactive", dontSendNotification);
        hidePluginWindowsLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (hidePluginWindows);
        hidePluginWindows.setClickingTogglesState (true);
        hidePluginWindows.setToggleState (settings.hidePluginWindowsWhenFocusLost(), dontSendNotification);
        hidePluginWindows.getToggleStateValue().addListener (this);

        addAndMakeVisible (openLastSessionLabel);

#if ! ELEMENT_SE
        const String sessionStr = "session";
#else
        const String sessionStr = "graph";
#endif

        openLastSessionLabel.setText (String ("Open last used XXX").replace ("XXX", sessionStr),
                                      dontSendNotification);
        openLastSessionLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (openLastSession);
        openLastSession.setClickingTogglesState (true);
        openLastSession.setToggleState (settings.openLastUsedSession(), dontSendNotification);
        openLastSession.getToggleStateValue().addListener (this);

        addAndMakeVisible (askToSaveSessionLabel);
        askToSaveSessionLabel.setText (String ("Ask to save XXXs on exit").replace ("XXX", sessionStr),
                                       dontSendNotification);
        askToSaveSessionLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (askToSaveSession);
        askToSaveSession.setClickingTogglesState (true);
        askToSaveSession.setToggleState (settings.askToSaveSession(), dontSendNotification);
        askToSaveSession.getToggleStateValue().addListener (this);

        addAndMakeVisible (systrayLabel);
        systrayLabel.setText ("Show system tray", dontSendNotification);
        systrayLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (systray);
        systray.setClickingTogglesState (true);
        systray.setToggleState (settings.isSystrayEnabled(), dontSendNotification);
        systray.getToggleStateValue().addListener (this);

        addAndMakeVisible (desktopScaleLabel);
        desktopScaleLabel.setText ("Desktop scale", dontSendNotification);
        desktopScaleLabel.setFont (Font (FontOptions (12.0, Font::bold)));
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
                if (auto* parent = findParentComponentOfClass<Preferences>())
                    parent->updateSize();
            }
        };

        addAndMakeVisible (legacyCtlLabel);
        legacyCtlLabel.setText ("Enable legacy controllers?", dontSendNotification);
        addAndMakeVisible (legacyCtl);
        legacyCtl.setClickingTogglesState (true);
        legacyCtl.setToggleState (settings.getBool ("legacyControllers", false), dontSendNotification);
        legacyCtl.getToggleStateValue().addListener (this);

        addAndMakeVisible (defaultSessionFileLabel);
        defaultSessionFileLabel.setText ("Default new Session", dontSendNotification);
        defaultSessionFileLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        addAndMakeVisible (defaultSessionFile);
        defaultSessionFile.setCurrentFile (settings.getDefaultNewSessionFile(), dontSendNotification);
        defaultSessionFile.addListener (this);
        addAndMakeVisible (defaultSessionClearButton);
        defaultSessionClearButton.setButtonText ("X");
        defaultSessionClearButton.addListener (this);
#if ELEMENT_SE
        defaultSessionFileLabel.setVisible (false);
        defaultSessionFile.setVisible (false);
#endif

        const int source = String ("internal") == settings.getClockSource()
                               ? ClockSourceInternal
                               : ClockSourceMidiClock;
        clockSourceBox.setSelectedId (source, dontSendNotification);
        clockSource.setValue (source);
        clockSource.addListener (this);

        addAndMakeVisible (mainContentLabel);
        mainContentLabel.setText ("UI Type", dontSendNotification);
        mainContentLabel.setFont (Font (FontOptions (12.0, Font::bold)));
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
#if ELEMENT_UPDATER
        r.removeFromTop (spacingBetweenSections);
        r2 = r.removeFromTop (settingHeight);
        checkForUpdatesLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
        checkForUpdates.setBounds (r2.removeFromLeft (toggleWidth)
                                       .withSizeKeepingCentre (toggleWidth, toggleHeight));
#endif
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
        layoutSetting (r, legacyCtlLabel, legacyCtl);

#if ! ELEMENT_SE
        layoutSetting (r, defaultSessionFileLabel, defaultSessionFile, 190 - settingHeight);
        defaultSessionClearButton.setBounds (defaultSessionFile.getRight(),
                                             defaultSessionFile.getY(),
                                             settingHeight - 2,
                                             defaultSessionFile.getHeight());
#endif

        if (pluginSettings.isVisible())
        {
            r.removeFromTop (spacingBetweenSections * 2);
            pluginSettings.setBounds (r);
        }
    }

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (legacyCtl.getToggleStateValue()))
        {
            settings.set ("legacyControllers", legacyCtl.getToggleState());
        }
#if ELEMENT_UPDATER
        else if (value.refersToSameSourceAs (checkForUpdates.getToggleStateValue()))
        {
            settings.setCheckForUpdates (checkForUpdates.getToggleState());
            jassert (settings.checkForUpdates() == checkForUpdates.getToggleState());
        }
#endif
        // clock source
        else if (value.refersToSameSourceAs (clockSource))
        {
            const var val = ClockSourceInternal == (int) clockSource.getValue() ? "internal" : "midiClock";
            settings.setClockSource (val);
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

    Label legacyCtlLabel;
    SettingButton legacyCtl;

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
    [[maybe_unused]] DeviceManager& devices;
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
          world (g),
          panic (*this)
    {
        addAndMakeVisible (midiOutputLabel);
        midiOutputLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        midiOutputLabel.setText ("MIDI Output Device", dontSendNotification);

        addAndMakeVisible (midiOutput);
        midiOutput.addListener (this);

        addAndMakeVisible (midiOutLatencyLabel);
        midiOutLatencyLabel.setFont (Font (FontOptions (12.0, Font::bold)));
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
        generateClockLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        generateClockLabel.setText ("Generate MIDI Clock", dontSendNotification);
        addAndMakeVisible (generateClock);
        generateClock.setYesNoText ("Yes", "No");
        generateClock.setClickingTogglesState (true);
        generateClock.setToggleState (settings.generateMidiClock(), dontSendNotification);
        generateClock.addListener (this);

        addAndMakeVisible (sendClockToInputLabel);
        sendClockToInputLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        sendClockToInputLabel.setText ("Send Clock to MIDI Input?", dontSendNotification);
        addAndMakeVisible (sendClockToInput);
        sendClockToInput.setYesNoText ("Yes", "No");
        sendClockToInput.setClickingTogglesState (true);
        sendClockToInput.setToggleState (settings.sendMidiClockToInput(), dontSendNotification);
        sendClockToInput.addListener (this);

        addAndMakeVisible (panicLabel);
        panicLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        panicLabel.setText ("MIDI Panic CC", juce::dontSendNotification);
        addAndMakeVisible (panic);
        panic.stabilize();

        addAndMakeVisible (startStopContLabel);
        startStopContLabel.setFont (Font (FontOptions (12.0, Font::bold)));
        startStopContLabel.setText (TRANS ("Transport: MIDI Start/Stop"),
                                    juce::dontSendNotification);
        addAndMakeVisible (startStopCont);
        startStopCont.setYesNoText ("Yes", "No");
        startStopCont.setClickingTogglesState (true);
        startStopCont.setToggleState (settings.transportRespondToStartStopContinue(),
                                      dontSendNotification);
        startStopCont.addListener (this);

        addAndMakeVisible (midiInputHeader);
        midiInputHeader.setText ("Active MIDI Inputs", dontSendNotification);
        midiInputHeader.setFont (Font (FontOptions (12.0f).withStyle ("Bold")));

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
        startStopCont.removeListener (this);
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
        layoutSetting (r, startStopContLabel, startStopCont);
        layoutSetting (r, panicLabel, panic, getWidth() / 2);

        r.removeFromTop (roundToInt ((double) spacingBetweenSections * 1.5));
        midiInputHeader.setBounds (r.removeFromTop (24));

        midiInputView.setBounds (r);
        midiInputs->updateSize();
    }

    void buttonClicked (Button* button) override
    {
        bool sendChanges = true;

        if (button == &generateClock)
        {
            settings.setGenerateMidiClock (generateClock.getToggleState());
            generateClock.setToggleState (settings.generateMidiClock(), dontSendNotification);
        }
        else if (button == &sendClockToInput)
        {
            settings.setSendMidiClockToInput (sendClockToInput.getToggleState());
            sendClockToInput.setToggleState (settings.sendMidiClockToInput(),
                                             dontSendNotification);
        }
        else if (button == &startStopCont)
        {
            settings.setTransportRespondToStartStopContinue (startStopCont.getToggleState());
            startStopCont.setToggleState (settings.transportRespondToStartStopContinue(),
                                          dontSendNotification);
        }
        else
        {
            sendChanges = false;
        }

        if (sendChanges)
            if (auto engine = world.audio())
                engine->applySettings (settings);
    }

    void comboBoxChanged (ComboBox* box) override
    {
        const auto dev = outputs[midiOutput.getSelectedId() - 10];
        if (box == &midiOutput)
            midi.setDefaultMidiOutput (dev);
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
    Label startStopContLabel;
    SettingButton startStopCont;

    Label midiInputHeader;
    Array<MidiDeviceInfo> outputs;

    Label panicLabel;
    class Panic : public Component
    {
    public:
        Panic (MidiSettingsPage& o) : owner (o)
        {
            addAndMakeVisible (enabled);
            enabled.setTooltip ("Enabled?");
            enabled.setButtonText ("");
            enabled.setToggleState (false, dontSendNotification);
            enabled.onClick = [this]() { save(); };

            addAndMakeVisible (channel);
            channel.setTooltip ("CC channel. 0 = omni");
            channel.setSliderStyle (Slider::IncDecButtons);
            channel.setRange (0.0, 16.0, 1.0);
            channel.setValue (1.0, juce::dontSendNotification);
            channel.setTextBoxStyle (Slider::TextBoxLeft, false, 30, channel.getTextBoxHeight());
            channel.setWantsKeyboardFocus (false);
            channel.onValueChange = [this]() { save(); };

            addAndMakeVisible (ccNumber);
            ccNumber.setTooltip ("CC number");
            ccNumber.setSliderStyle (Slider::IncDecButtons);
            ccNumber.setRange (0.0, 127, 1.0);
            ccNumber.setValue (21, juce::dontSendNotification);
            ccNumber.setTextBoxStyle (Slider::TextBoxLeft, false, 34, ccNumber.getTextBoxHeight());
            ccNumber.setWantsKeyboardFocus (false);
            ccNumber.onValueChange = channel.onValueChange;

            stabilize();
        }

        ~Panic()
        {
            enabled.onClick = nullptr;
            ccNumber.onValueChange = nullptr;
            channel.onValueChange = nullptr;
        }

        void resized() override
        {
            auto r1 = getLocalBounds();
            enabled.setBounds (r1.removeFromLeft (r1.getHeight()));
            auto r2 = r1.removeFromRight (r1.getWidth() * 0.5);
            channel.setBounds (r1);
            ccNumber.setBounds (r2);
        }

        void stabilize()
        {
            const auto params = owner.settings.getMidiPanicParams();
            enabled.setToggleState (params.enabled, juce::dontSendNotification);
            channel.setValue (params.channel, juce::dontSendNotification);
            ccNumber.setValue (params.ccNumber, juce::dontSendNotification);
        }

    private:
        ToggleButton enabled;
        Slider channel;
        Slider ccNumber;
        MidiSettingsPage& owner;

        void save()
        {
            MidiPanicParams params = {
                enabled.getToggleState(),
                (int) channel.getValue(),
                (int) ccNumber.getValue()
            };

            owner.settings.setMidiPanicParams (params);
            auto audio = owner.world.audio();
            audio->applySettings (owner.settings);
        }
    } panic;

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
                label->setFont (Font (FontOptions (12.0f)));
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
#if ELEMENT_UPDATER
class UpdatesSettingsPage : public SettingsPage,
                            public Button::Listener
{
public:
    UpdatesSettingsPage (Context& w)
        : world (w)
    {
        addAndMakeVisible (releaseChannelLabel);
        releaseChannelLabel.setText ("Release Channel", dontSendNotification);
        releaseChannelLabel.setFont (Font (FontOptions (12.0, Font::bold)));

        addAndMakeVisible (releaseChannelBox);
        releaseChannelBox.addItem ("Stable", 1);
        releaseChannelBox.addItem ("Preview", 2);
        releaseChannelBox.setSelectedId (1, dontSendNotification);

        addAndMakeVisible (authorizationLabel);
        authorizationLabel.setText ("Preview Access", dontSendNotification);
        authorizationLabel.setFont (Font (FontOptions (15.0f).withStyle ("Bold")));

        addAndMakeVisible (statusLabel);
        statusLabel.setText ("Not authorized", dontSendNotification);
        statusLabel.setFont (Font (FontOptions (12.0)));
        statusLabel.setColour (Label::textColourId, Colours::grey);

        addAndMakeVisible (authorizeButton);
        authorizeButton.setButtonText ("Sign in with Kushview.net");
        authorizeButton.addListener (this);

        addAndMakeVisible (signOutButton);
        signOutButton.setButtonText ("Sign Out");
        signOutButton.addListener (this);
        signOutButton.setVisible (false);

        updateAuthorizationState();
    }

    ~UpdatesSettingsPage()
    {
        authorizeButton.removeListener (this);
        signOutButton.removeListener (this);
    }

    void buttonClicked (Button* button) override
    {
        if (button == &authorizeButton)
        {
            startOAuthFlow();
        }
        else if (button == &signOutButton)
        {
            signOut();
        }
    }

    void resized() override
    {
        const int spacingBetweenSections = 6;
        const int settingHeight = 22;

        Rectangle<int> r (getLocalBounds());

        // Release channel selector
        auto r2 = r.removeFromTop (settingHeight);
        releaseChannelLabel.setBounds (r2.removeFromLeft (getWidth() / 2));
        releaseChannelBox.setBounds (r2.withSizeKeepingCentre (r2.getWidth(), settingHeight));

        // Authorization section
        r.removeFromTop (spacingBetweenSections * 2);
        authorizationLabel.setBounds (r.removeFromTop (24));

        r.removeFromTop (spacingBetweenSections);

        // Status label
        statusLabel.setBounds (r.removeFromTop (settingHeight));

        // Buttons
        r.removeFromTop (spacingBetweenSections);
        auto buttonRow = r.removeFromTop (settingHeight);
        authorizeButton.setBounds (buttonRow.removeFromLeft (180));
        buttonRow.removeFromLeft (spacingBetweenSections);
        signOutButton.setBounds (buttonRow.removeFromLeft (100));
    }

private:
    Context& world;

    Label releaseChannelLabel;
    ComboBox releaseChannelBox;

    Label authorizationLabel;
    Label statusLabel;
    TextButton authorizeButton;
    TextButton signOutButton;

    /** Initiates the OAuth authorization flow.
        
        Opens the user's default browser to kushview.net OAuth page.
        The website will redirect back to the app via custom URL scheme
        after successful authentication.
     */
    void startOAuthFlow()
    {
        // TODO: Implement OAuth flow
        // 1. Generate state parameter for CSRF protection
        // 2. Build authorization URL with client_id, redirect_uri, scope
        // 3. Launch in default browser
        // 4. Register URL handler to receive callback
        
        const String authUrl = "https://kushview.net/oauth/authorize?"
                               "client_id=element-app&"
                               "redirect_uri=element://auth/callback&"
                               "response_type=code&"
                               "scope=updates";
        
        URL (authUrl).launchInDefaultBrowser();
        
        Logger::writeToLog ("OAuth: Authorization flow started");
    }

    /** Handles the OAuth callback with authorization code.
        
        Exchanges the authorization code for an access token and
        refresh token, then stores them securely.
        
        @param code The authorization code received from the OAuth callback
     */
    void handleOAuthCallback (const String& code)
    {
        // TODO: Implement token exchange
        // 1. POST to token endpoint with code
        // 2. Parse response to get access_token and refresh_token
        // 3. Store tokens securely (keychain/credential manager)
        // 4. Update UI and configure updater
        
        Logger::writeToLog ("OAuth: Received authorization code: " + code);
        
        // For now, just update UI as if authorized
        updateAuthorizationState (true, "user@example.com");
    }

    /** Signs out the user and clears stored tokens. */
    void signOut()
    {
        // TODO: Implement sign out
        // 1. Clear stored tokens from keychain/credential manager
        // 2. Reset updater to use stable channel URL
        // 3. Update UI
        
        Logger::writeToLog ("OAuth: Signing out");
        updateAuthorizationState (false);
    }

    /** Updates the UI to reflect current authorization state.
        
        @param authorized Whether the user is currently authorized
        @param email Optional email address to display when authorized
     */
    void updateAuthorizationState (bool authorized = false, const String& email = String())
    {
        if (authorized)
        {
            statusLabel.setText ("Authorized as: " + email, dontSendNotification);
            statusLabel.setColour (Label::textColourId, Colours::green);
            authorizeButton.setVisible (false);
            signOutButton.setVisible (true);
            releaseChannelBox.setItemEnabled (2, true); // Enable Preview
        }
        else
        {
            statusLabel.setText ("Not authorized", dontSendNotification);
            statusLabel.setColour (Label::textColourId, Colours::grey);
            authorizeButton.setVisible (true);
            signOutButton.setVisible (false);
            releaseChannelBox.setItemEnabled (2, false); // Disable Preview
            
            // Switch to Stable if Preview was selected
            if (releaseChannelBox.getSelectedId() == 2)
                releaseChannelBox.setSelectedId (1, dontSendNotification);
        }
    }
};
#endif

//==============================================================================
Preferences::Preferences (GuiService& ui)
    : _context (ui.context()), _ui (ui)
{
    pageList = std::make_unique<PageList> (*this);
    addAndMakeVisible (pageList.get());
    pageList->setName ("PageList");

    pageComponent = std::make_unique<Component>();
    addAndMakeVisible (pageComponent.get());
    pageComponent->setName ("Page");

    updateSize();
}

Preferences::~Preferences()
{
    pageList = nullptr;
    pageComponent = nullptr;
    pages.clear();
    _ui.refreshMainMenu();
}

//==============================================================================
void Preferences::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor);
}

void Preferences::resized()
{
    auto r = getLocalBounds().reduced (8);
    pageList->setBounds (r.removeFromLeft (110));
    if (pageComponent)
        pageComponent->setBounds (r.reduced (8));
}

bool Preferences::keyPressed (const KeyPress& key)
{
    return false;
}

void Preferences::addPage (const String& name)
{
    if (! pageList->pageNames.contains (name))
        pageList->addItem (name);
}

Component* Preferences::createPageForName (const String& name)
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
#if ELEMENT_UPDATER
    else if (name == EL_REPOSITORY_PREFERENCE_NAME)
    {
        return new UpdatesSettingsPage (_context);
    }
#endif
    return nullptr;
}

void Preferences::addDefaultPages()
{
    if (pageList->getNumRows() > 0)
        return;

    addPage (EL_GENERAL_SETTINGS_NAME);
    addPage (EL_AUDIO_SETTINGS_NAME);
    addPage (EL_MIDI_SETTINGS_NAME);
#if ! ELEMENT_SE
    addPage (EL_OSC_SETTINGS_NAME);
#endif
#if ELEMENT_UPDATER
    addPage (EL_REPOSITORY_PREFERENCE_NAME);
#endif

    setPage (EL_GENERAL_SETTINGS_NAME);
}

void Preferences::setPage (const String& name)
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

void Preferences::updateSize()
{
    setSize (600, 500);
}

} /* namespace element */

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.h>

#include <element/context.hpp>

#include <element/ui.hpp>
#include <element/ui/about.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/style.hpp>

namespace element {

//==============================================================================
class AboutScreen : public juce::Component
{
public:
    AboutScreen();
    void resized() override;
    void paint (juce::Graphics& g) override;

    void setAboutInfo (const AboutInfo& details);
    AboutInfo aboutInfo() const noexcept { return info; }

private:
    AboutInfo info;
    juce::Label titleLabel { "title", "Element" },
        versionLabel { "version" },
        copyrightLabel { "copyright", juce::String (juce::CharPointer_UTF8 ("\xc2\xa9")) + juce::String (" 2023 Kushview, LLC.") };
    juce::HyperlinkButton aboutButton { "About Us", juce::URL ("https://kushview.net") };
    juce::Rectangle<float> elementLogoBounds;
    std::unique_ptr<juce::Drawable> elementLogo;
    juce::DrawableImage logo;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::TextButton copyVersionButton;

    void updateAboutInfo();
    void copyVersion();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutScreen)
};

//==============================================================================
class AboutDialog : public juce::DialogWindow
{
public:
    AboutDialog (UI& ui)
        : juce::DialogWindow ("About Element",
                              ui.getLookAndFeel().findColour (juce::DocumentWindow::backgroundColourId),
                              true,
                              false),
          _ui (ui)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new AboutScreen(), true);
    }

    inline void closeButtonPressed() override
    {
        _ui.commands().invokeDirectly (Commands::showAbout, true);
    }

private:
    UI& _ui;
};

} // namespace element

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
class AboutComponent : public juce::Component
{
public:
    AboutComponent();
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutComponent)
};

//==============================================================================
class AboutDialog : public juce::DialogWindow
{
public:
    AboutDialog (GuiService& g)
        : juce::DialogWindow ("About Element",
                              g.getLookAndFeel().findColour (juce::DocumentWindow::backgroundColourId),
                              true,
                              false),
          gui (g)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new AboutComponent(), true);
    }

    void closeButtonPressed() override
    {
        gui.commands().invokeDirectly (Commands::showAbout, true);
    }

private:
    GuiService& gui;
};

} // namespace element

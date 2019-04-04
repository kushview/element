
#pragma once

#include "ElementApp.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "session/CommandManager.h"
#include "Commands.h"
#include "Globals.h"
#include "Utils.h"

namespace Element {

class AboutComponent    : public Component
{
public:
    AboutComponent()
    {
        addAndMakeVisible (titleLabel);
        titleLabel.setJustificationType (Justification::centred);
        titleLabel.setFont (Font (32.0f, Font::FontStyleFlags::bold));

        auto buildDate = Time::getCompilationDate();
        addAndMakeVisible (versionLabel);
        versionLabel.setText (Util::appName() + String(" v") + ProjectInfo::versionString
                              + "\nBuild date: " + String (buildDate.getDayOfMonth())
                                                 + " " + Time::getMonthName (buildDate.getMonth(), true)
                                                 + " " + String (buildDate.getYear()),
                              dontSendNotification);

        versionLabel.setJustificationType (Justification::centred);
        versionLabel.setFont (Font (12.f));

        addAndMakeVisible (copyrightLabel);
        copyrightLabel.setJustificationType (Justification::centred);
        copyrightLabel.setFont (Font (11.f));

        addAndMakeVisible (aboutButton);
        aboutButton.setTooltip ({});
        aboutButton.setColour (HyperlinkButton::textColourId, Colors::toggleBlue);

        if (showPurchaseButton())
        {
            addAndMakeVisible (licenseButton);
            licenseButton.onClick = [this]
            {
                const URL url (getPurchaseURL());
                url.launchInDefaultBrowser();
            };
        }

        setSize (500, 240);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromBottom (20);

        auto rightSlice  = bounds.removeFromRight (150);
        auto leftSlice   = bounds.removeFromLeft (150);
        auto centreSlice = bounds;

        rightSlice.removeFromRight (20);
        auto iconSlice = rightSlice.removeFromRight (64);
        huckleberryLogoBounds = iconSlice.removeFromBottom (64).toFloat();

        elementLogoBounds = leftSlice.removeFromTop (150).toFloat();
        elementLogoBounds.setWidth (elementLogoBounds.getWidth() + 100);
        elementLogoBounds.setHeight (elementLogoBounds.getHeight() + 100);

        copyrightLabel.setBounds (leftSlice.removeFromBottom (20));

        auto titleHeight = 40;

        centreSlice.removeFromTop ((centreSlice.getHeight() / 2) - (titleHeight));

        titleLabel.setBounds (centreSlice.removeFromTop (titleHeight));

        centreSlice.removeFromTop (10);
        versionLabel.setBounds (centreSlice.removeFromTop (40));

        centreSlice.removeFromTop (10);

        if (licenseButton.isVisible())
            licenseButton.setBounds (centreSlice.removeFromTop (25).reduced (25, 0));

        aboutButton.setBounds (centreSlice.removeFromBottom (20));
    }

    void paint (Graphics& g) override
    {
        g.fillAll (findColour (DocumentWindow::backgroundColourId));

        // if (elementLogo != nullptr)
        //     elementLogo->drawWithin (g, elementLogoBounds.translated (-75, -75), RectanglePlacement::centred, 1.0);

        if (huckleberryLogo != nullptr)
            huckleberryLogo->drawWithin (g, huckleberryLogoBounds, RectanglePlacement::centred, 1.0);
    }

    inline bool showPurchaseButton() const
    {
        #if defined (EL_FREE)
         return true;
        #endif
        return false;
    }

    inline URL getPurchaseURL() const 
    {
        return URL ("https://kushview.net/element/purchase/");
    }

private:
    Label titleLabel { "title", Util::appName().toUpperCase() },
          versionLabel { "version" },
          copyrightLabel { "copyright", String (CharPointer_UTF8 ("\xc2\xa9")) + String (" 2019 Kushview, LLC.") };

    HyperlinkButton aboutButton { "About Us", URL ("https://kushview.net") };
    TextButton licenseButton { "Purchase License" };

    Rectangle<float> huckleberryLogoBounds, elementLogoBounds;

    std::unique_ptr<Drawable> elementLogo { Drawable::createFromImageData (BinaryData::ElementIcon_png,
                                                                           BinaryData::ElementIcon_pngSize) };

    std::unique_ptr<Drawable> huckleberryLogo { Drawable::createFromImageData (BinaryData::ElementIcon_png,
                                                                               BinaryData::ElementIcon_pngSize) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutComponent)
};

class AboutDialog : public DialogWindow
{
public:
    AboutDialog (GuiController& g)
        : DialogWindow ("About Element", 
            g.getLookAndFeel().findColour (DocumentWindow::backgroundColourId),
            true, false),
          gui (g)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new AboutComponent(), true);
    }

    void closeButtonPressed() override
    {
        gui.getWorld().getCommandManager().invokeDirectly (
            Commands::showAbout, true);
    }

private:
    GuiController& gui;
};

}

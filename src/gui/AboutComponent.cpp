/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/AboutComponent.h"

#define EL_LICENSE_TEXT \
"Copyright (C) 2014-%YEAR%  Kushview, LLC.  All rights reserved.\r\n\r\n" \
\
"This program is free software; you can redistribute it and/or modify\r\n" \
"it under the terms of the GNU General Public License as published by\r\n" \
"the Free Software Foundation; either version 3 of the License, or\r\n" \
"(at your option) any later version.\r\n\r\n" \
\
"This program is distributed in the hope that it will be useful,\r\n" \
"but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n" \
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n" \
"GNU General Public License for more details.\r\n\r\n" \
\
"You should have received a copy of the GNU General Public License\r\n" \
"along with this program; if not, write to the Free Software\r\n" \
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\r\n"

namespace Element {

class AboutCreditsPanel : public Component
{
public:
    AboutCreditsPanel()
    {
        setSize (100, 24);
    }
    
    void addSection (const String& title, const StringArray& names)
    {
        auto* section = sections.add (new Section());
        section->title.setText (title, dontSendNotification);
        addAndMakeVisible (section->title);
        for (const auto& name : names)
        {
            auto* nameLabel = section->names.add (new Label (name, name));
            nameLabel->setFont (13.f);
            addAndMakeVisible (nameLabel);
        }

        setSize (getWidth(), getTotalHeight());
        resized();
    }

    void resized() override
    {
        int y = 0;
        for (auto* section : sections)
        {
            section->title.setBounds (0, y, getWidth(), titleHeight);
            y += titleHeight;
            for (auto* name : section->names)
            {
                name->setBounds (8, y, getWidth(), nameHeight);
                y += nameHeight;
            }
        }
    }

private:
    struct Section
    {
        Label title;
        OwnedArray<Label> names;
    };

    OwnedArray<Section> sections;
    int titleHeight = 24;
    int nameHeight = 20;
    int getTotalHeight()
    {
        auto size = sections.size() * titleHeight;
        for (auto* section : sections)
            for (auto* name : section->names)
                size += nameHeight;
        return size;
    }
};

class AboutCreditsComponent : public Component
{
public:
    AboutCreditsComponent()
    {
        addAndMakeVisible (view);
        view.setViewedComponent (&panel, false);
    }

    AboutCreditsPanel& getPanel() { return panel; }

    void resized() override
    {
        panel.setSize (getWidth() - 14, panel.getHeight());
        view.setBounds (getLocalBounds());
    }

private:
    AboutCreditsPanel panel;
    Viewport view;
};

//=============================================================================

class LicenseTextComponent : public Component
{
public:
    LicenseTextComponent()
    {
        addAndMakeVisible (text);
        text.setCaretVisible (false);
        text.setMultiLine (true, false);
        text.setFont (Font (Font::getDefaultMonospacedFontName(), 13.f, Font::plain));
        text.setText (String(EL_LICENSE_TEXT).replace ("%YEAR%", String (Time::getCurrentTime().getYear())));
        text.setReadOnly (true);
    }

    void resized() override
    {
        text.setBounds (getLocalBounds());
    }

private:
    TextEditor text;
};

//=============================================================================

AboutComponent::AboutComponent()
{
    elementLogo = Drawable::createFromImageData (
        BinaryData::ElementIcon_png, BinaryData::ElementIcon_pngSize);

    addAndMakeVisible (titleLabel);
    titleLabel.setJustificationType (Justification::centred);
    titleLabel.setFont (Font (34.0f, Font::FontStyleFlags::bold));

    auto buildDate = Time::getCompilationDate();
    addAndMakeVisible (versionLabel);
    versionLabel.setText (String(" v") + ProjectInfo::versionString
                            + "\nBuild date: " + String (buildDate.getDayOfMonth())
                                                + " " + Time::getMonthName (buildDate.getMonth(), true)
                                                + " " + String (buildDate.getYear()),
                            dontSendNotification);

    versionLabel.setJustificationType (Justification::centred);
    versionLabel.setFont (Font (13.f));

    addAndMakeVisible (copyrightLabel);
    copyrightLabel.setJustificationType (Justification::centred);
    copyrightLabel.setFont (Font (13.f));

    addAndMakeVisible (aboutButton);
    aboutButton.setTooltip ({});
    aboutButton.setColour (HyperlinkButton::textColourId, Colors::toggleBlue);

    addAndMakeVisible (tabs);
    tabs.setTabBarDepth (24);
    tabs.setOutline (0);
    const auto tabc = findColour (TextEditor::backgroundColourId);

    auto* authors = new AboutCreditsComponent();
    authors->getPanel().addSection ("Lead Developer", { 
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
    });

    

    tabs.addTab ("Authors", tabc, authors, true);

    auto* donors = new AboutCreditsComponent();
    donors->getPanel().addSection ("Gold Donors", { 
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
        "Michael Fisher (mfisher31)",
    });

    tabs.addTab ("Donors",  tabc, donors, true);
    tabs.addTab ("License", tabc, new LicenseTextComponent(), true);
    setSize (520, 400);
}

void AboutComponent::resized()
{
    auto bounds = getLocalBounds();
    elementLogoBounds.setBounds (14, 14, 72, 72);
    auto topSlice = bounds.removeFromTop (90);
    topSlice.removeFromTop (10);
    titleLabel.setBounds (topSlice.removeFromTop (40));
    versionLabel.setBounds (topSlice.removeFromTop (24));
    copyrightLabel.setBounds (topSlice.removeFromTop (24));
    bounds.removeFromTop (2);
    tabs.setBounds (bounds.reduced (4));
}

void AboutComponent::paint (Graphics& g)
{
    g.fillAll (findColour (DocumentWindow::backgroundColourId));

    if (elementLogo != nullptr)
        elementLogo->drawWithin (g, elementLogoBounds, RectanglePlacement::centred, 1.0);
}

}

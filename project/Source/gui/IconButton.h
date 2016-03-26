/*
    IconButton.h - This file is part of Element
    Copyright (C) 2013  Michael Fisher <mfisher31@gmail.com>

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

#ifndef ELEMENT_ICONBUTTON_H
#define ELEMENT_ICONBUTTON_H

class IconButton : public Button,
                   public ButtonListener
{
public:
    IconButton (const String&, const String& slug,
                bool toggle = false)
        : Button (slug), padding (0), isToggle (toggle)
    {
        // icon = Tango::getDrawable (section, slug);
        addListener (this);
    }

    //boost::signal<void()>& signalClicked() { return clickedSignal; }

    void mouseDown (const MouseEvent& ev)
    {
//            if (isToggle)
//              setToggleState (! getToggleState(), true);

        Button::mouseDown (ev);
    }

    bool hitTest (int x, int y)
    {
        // detect alpha maybe
        return Button::hitTest (x, y);
    }

    void paintButton (Graphics& g, bool mouseIsOver, bool mouseIsDown)
    {
        g.setColour (Colours::grey);
        g.fillRoundedRectangle (0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f);

        g.setColour (Colours::darkgrey);
        g.drawRoundedRectangle (0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f, 1.0f);

        Rectangle<float> r (4.0f, 4.0f, (float)getHeight() - 8.0f, (float)getHeight() - 8.0f);

        icon->drawWithin (g, r, RectanglePlacement::centred,
                                getIconAlpha (mouseIsDown, mouseIsOver));
    }

    void buttonClicked (Button*) {
    //    clickedSignal();
    }

private:
    ScopedPointer<Drawable> icon;
    float padding;
    bool isToggle;

    const float getIconAlpha (bool isDown, bool isOver)
    {
        if (isToggle && getToggleState())
        {
            return 1.0f;
        }
        else if (isDown)
        {
            return 0.95f;
        }
        else if (isOver)
        {
            return 0.80f;
        }
        else
        {
            return 0.70f;
        }
    }
};


class TransportButtons : public Component
{
public:
    TransportButtons()
        : record ("actions", "media-record", true),
          stop ("actions", "media-playback-stop"),
          play ("actions", "media-playback-start", true)
    {
        addAndMakeVisible (&record);
        record.setSize (22, 22);
        layout.setItemLayout (0,22, 22, 22);

        addAndMakeVisible (&stop);
        stop.setSize (22, 22);
        layout.setItemLayout (1, 22, 22, 22);

        addAndMakeVisible (&play);
        play.setSize (22, 22);
        layout.setItemLayout (2, 22, 22, 22);
    }

    void resized()
    {
        Component* comps[] = { &record, &stop, &play };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), 22, false, false);
    }

private:
    IconButton record,stop,play;
    StretchableLayoutManager layout;
};

#endif /* ELEMENT_ICONBUTTON_H */

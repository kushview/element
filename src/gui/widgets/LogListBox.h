/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    Author Eliot Akira <me@eliotakira.com>
    
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

#pragma once

#include "JuceHeader.h"

namespace element {

class LogListBox : public ListBox,
                   public ListBoxModel,
                   public AsyncUpdater
{
public:
    /** Constructor */
    LogListBox()
    {
        setModel (this);
    }

    /** Destructor */
    ~LogListBox() override = default;

    int getNumRows() override
    {
        return logList.size();
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        ignoreUnused (rowIsSelected);
        g.setFont (Font (Font::getDefaultMonospacedFontName(),
                         g.getCurrentFont().getHeight(),
                         Font::plain));
        if (isPositiveAndBelow (row, logList.size()))
            ViewHelpers::drawBasicTextRow (logList[row], g, width, height, false);
    }

    /** Sets the maximum allowed messages in the log history */
    void setMaxMessages (int newMax)
    {
        if (newMax <= 0 || newMax == maxMessages)
            return;
        maxMessages = newMax;
        triggerAsyncUpdate();
    }

    /** Add message from std::string */
    void addMessage (const std::string& message)
    {
        addMessage (String (message));
    }

    /** Add a new message to the history */
    void addMessage (const String& message)
    {
        if (logList.size() > maxMessages)
            logList.remove (0);
        logList.add (message);
        triggerAsyncUpdate();
    }

    /** Clears the message history */
    void clear()
    {
        logList.clear();
        triggerAsyncUpdate();
    }

    /** @internal */
    void handleAsyncUpdate() override
    {
        updateContent();
        scrollToEnsureRowIsOnscreen (logList.size() - 1);
        repaint();
    }

private:
    int maxMessages { 100 };
    StringArray logList;
};

} // namespace element

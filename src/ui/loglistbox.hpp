// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

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
        g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), g.getCurrentFont().getHeight(), 0));
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

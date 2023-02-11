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

#pragma once

#include "JuceHeader.h"
#include <element/signals.hpp>

namespace element {

class ContentComponent;
class PluginProcessor;

/** The audio processor editor used for Element plugins */
class PluginEditor : public AudioProcessorEditor
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor();

    //==========================================================================
    ContentComponent* getContentComponent();

    //==========================================================================
    PluginProcessor& getProcessor() { return processor; }

    //==========================================================================
    void setWantsPluginKeyboardFocus (bool focus);
    bool getWantsPluginKeyboardFocus() const;

    //==========================================================================
    int getLatencySamples() const;
    void setReportZeroLatency (bool force);
    bool isReportingZeroLatency() const;

    //==========================================================================
    void paint (Graphics&) override;
    void resized() override;
    bool keyPressed (const KeyPress& key) override;
    bool keyStateChanged (bool) override { return true; }

private:
    PluginProcessor& processor;
    SafePointer<Component> content;
    SignalConnection perfParamChangedConnection;

    const int paramTableSize = 100;
    bool paramTableVisible = false;
    class ParamTable;
    std::unique_ptr<ParamTable> paramTable;
    class ParamTableToggle;
    std::unique_ptr<ParamTableToggle> paramToggle;

    void updatePerformanceParamEnablements();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace element

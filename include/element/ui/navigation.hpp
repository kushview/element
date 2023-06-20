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

#include <element/juce/gui_basics.hpp>

// FIXME: this class isn't in element namespace.
class AudioIOPanelView;

namespace element {

class Context;
class PluginsPanelView;
class DataPathTreeComponent;
class SessionTreePanel;

class NavigationConcertinaPanel : public juce::ConcertinaPanel {
public:
    NavigationConcertinaPanel (Context& g);
    ~NavigationConcertinaPanel();

    void saveState (juce::PropertiesFile* props);
    void restoreState (juce::PropertiesFile* props);

    int getIndexOfPanel (juce::Component* panel);
    Component* findPanelByName (const juce::String& name);
    void showPanel (const juce::String& name);
    void hidePanel (const juce::String& name);
    void setPanelName (const juce::String& panel, const juce::String& newName);

    template <class T>
    inline T* findPanel()
    {
        for (int i = getNumPanels(); --i >= 0;)
            if (T* panel = dynamic_cast<T*> (getPanel (i)))
                return panel;
        return nullptr;
    }

    void clearPanels();

    void updateContent();

    const juce::StringArray& getNames() const;
    const int getHeaderHeight() const;

    void setHeaderHeight (const int newHeight);

private:
    Context& globals;
    int headerHeight;
    int defaultPanelHeight;

    juce::StringArray names, namesHidden;
    juce::OwnedArray<juce::Component> comps;
    void addPanelInternal (const int index,
                           juce::Component* comp, 
                           const juce::String& name = juce::String(), 
                           juce::Component* header = nullptr);

    class Header;
    class ElementsHeader;
    class UserDataPathHeader;

    class LookAndFeel;
    std::unique_ptr<LookAndFeel> lookAndFeel;
};

} // namespace element

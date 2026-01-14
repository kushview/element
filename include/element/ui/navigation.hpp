// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

class AudioIOPanelView; // FIXME: this class isn't in element namespace.

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

    void paint (juce::Graphics& g) override;

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
};

} // namespace element

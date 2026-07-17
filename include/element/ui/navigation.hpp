// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <vector>

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
    void insertPanel (juce::Component* comp, int index = -1);
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

    /** Describes a navigation panel registered with the concertina. Every panel,
        including the built-ins, is created from one of these. */
    struct PanelDescription {
        juce::String name;        // identity, header label, and persistence key
        juce::String componentID; // component id; empty means "same as name"
        int index { -1 };         // position among panels; -1 appends in registration order
        int preferredSize { 0 };  // initial expanded height in px; 0 = collapsed to header
        bool hidden { false };    // initial visibility
    };

    /** Builds a fresh panel component on demand. */
    using PanelFactory = std::function<juce::Component*()>;

    /** Builds a custom header for a panel. Return nullptr to use the default header. */
    using PanelHeaderFactory = std::function<juce::Component*(juce::Component& panel)>;

    using juce::ConcertinaPanel::addPanel;
    using juce::ConcertinaPanel::removePanel;

    /** Register a navigation panel.

        A registered panel is (re)created by updateContent(), so it survives
        showPanel()/hidePanel() and other rebuilds, and persists by name. The panel
        is also created immediately, so no manual updateContent() call is required.

        @param desc     Identity, placement, and initial size/visibility for the panel.
        @param factory  Called to construct the panel component (Navigation takes ownership).
        @param header   Optional; builds a custom header for the panel (Navigation takes ownership).
    */
    void addPanel (PanelDescription desc, PanelFactory factory, PanelHeaderFactory header = nullptr);

    /** Unregister and remove a panel previously added with addPanel().

        @param name The name the panel was registered with.
    */
    void removePanel (const juce::String& name);

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

    juce::StringArray names;
    juce::OwnedArray<juce::Component> comps;
    void addPanelInternal (const int index,
                           juce::Component* comp,
                           const juce::String& name = juce::String(),
                           juce::Component* header = nullptr);

    struct RegisteredPanel {
        PanelDescription desc;
        PanelFactory factory;
        PanelHeaderFactory header;
    };
    std::vector<RegisteredPanel> registered;
    void registerBuiltInPanels();
    void createRegisteredPanels();
    void createPanel (const RegisteredPanel& panel);
    void setPanelHidden (const juce::String& name, bool hidden);

    class Header;
    class ElementsHeader;
    class UserDataPathHeader;
    class LookAndFeel;
};

} // namespace element

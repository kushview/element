
#pragma once

#include "gui/ContentComponent.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/views/ControllerDevicesView.h"
#include "gui/views/ControllerMapsView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/KeymapEditorView.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/views/SessionTreeContentView.h"
#include "gui/views/SessionSettingsView.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/views/NodeEditorContentView.h"

namespace Element {

template<class ViewType>
class ContentViewPanel :  public WorkspacePanel
{
public:
    ContentViewPanel() : WorkspacePanel()
    {
        addAndMakeVisible (view);
    }

    ~ContentViewPanel() { }
    
    void initializeView (AppController& app) override   { view.initializeView (app); }
    void didBecomeActive() override                     { view.didBecomeActive(); }
    void stabilizeContent() override                    { view.stabilizeContent(); }

    void resized() override
    {
        view.setBounds (getLocalBounds());
    }

protected:
    ViewType view;
};

class ControllerDevicesPanel : public ContentViewPanel<ControllerDevicesView>
{
public:
    ControllerDevicesPanel() { setName ("Controllers"); }
    ~ControllerDevicesPanel() = default;
};

class ControllerMapsPanel : public ContentViewPanel<ControllerMapsView>
{
public:
    ControllerMapsPanel() { setName ("Maps"); }
    ~ControllerMapsPanel() = default;
};

class GraphSettingsPanel : public ContentViewPanel<GraphSettingsView>
{
public:
    GraphSettingsPanel() { setName ("Graph Settings"); }
    ~GraphSettingsPanel() = default;
};

class KeymapEditorPanel : public ContentViewPanel<KeymapEditorView>
{
public:
    KeymapEditorPanel() { setName ("Keymappings"); }
    ~KeymapEditorPanel() = default;
};

class NodeChannelStripPanel : public ContentViewPanel<NodeChannelStripView>
{
public:
    NodeChannelStripPanel() { setName ("Strip"); }
    ~NodeChannelStripPanel() = default;
};

class NodeEditorPanel : public ContentViewPanel<NodeEditorContentView>
{
public:
    NodeEditorPanel() { setName ("Node"); }
    ~NodeEditorPanel() = default;
};

class NodeMidiPanel : public ContentViewPanel<NodeMidiContentView>
{
public:
    NodeMidiPanel() { setName ("MIDI"); }
    ~NodeMidiPanel() = default;
};

class SessionPanel : public ContentViewPanel<SessionTreeContentView>
{
public:
    SessionPanel() { setName ("Session"); }
    ~SessionPanel() = default;
};

class SessionSettingsPanel : public ContentViewPanel<SessionSettingsView>
{
public:
    SessionSettingsPanel() { setName ("Session Settings"); }
    ~SessionSettingsPanel() = default;
};

}

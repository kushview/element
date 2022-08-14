
#include "controllers/GuiController.h"
#include "engine/nodes/NodeTypes.h"
#include "engine/nodeobject.hpp"

#include "gui/nodes/AudioIONodeEditor.h"
#include "gui/nodes/AudioRouterEditor.h"
#include "gui/nodes/MidiIONodeEditor.h"
#include "gui/nodes/MidiMonitorNodeEditor.h"
#include "gui/nodes/MidiProgramMapEditor.h"
#include "gui/nodes/MidiRouterEditor.h"
#include "gui/nodes/LuaNodeEditor.h"
#include "gui/nodes/OSCReceiverNodeEditor.h"
#include "gui/nodes/OSCSenderNodeEditor.h"
#include "gui/nodes/VolumeNodeEditor.h"
#include "gui/nodes/ScriptNodeEditor.h"

#include "gui/NodeEditorFactory.h"
#include "scripting/ScriptingEngine.h"
#include "globals.hpp"

namespace Element {

class FallbackNodeEditorSource : public NodeEditorSource
{
    GuiController& gui;

public:
    FallbackNodeEditorSource (GuiController& g)
        : gui (g) {}

    void findEditors (Array<NodeEditorDescription>&) override {}

    NodeEditorComponent* instantiate (const String& identifier, const Node& node, NodeEditorPlacement placement) override

    {
        if (node.getFormat() != EL_INTERNAL_FORMAT_NAME || identifier != EL_NODE_EDITOR_DEFAULT_ID)
        {
            return nullptr;
        }

        switch (placement)
        {
            case NodeEditorPlacement::PluginWindow:
                return instantiateForPluginWindow (node);
                break;
            case NodeEditorPlacement::NavigationPanel:
                return instantiateForNavigationPanel (node);
                break;
            default:
                break;
        }

        return nullptr;
    }

private:
    NodeEditorComponent* instantiateForPluginWindow (const Node& node)
    {
        const auto NID = node.getIdentifier().toString();
        if (NID == EL_INTERNAL_ID_MIDI_ROUTER)
        {
            return new MidiRouterEditor (node);
        }
        else if (NID == EL_INTERNAL_ID_MIDI_MONITOR)
        {
            return new MidiMonitorNodeEditor (node);
        }
        else if (NID == EL_INTERNAL_ID_OSC_RECEIVER)
        {
            return new OSCReceiverNodeEditor (node);
        }
        else if (NID == EL_INTERNAL_ID_OSC_SENDER)
        {
            return new OSCSenderNodeEditor (node);
        }
        else if (NID.contains (EL_INTERNAL_ID_VOLUME))
        {
            return new VolumeNodeEditor (node, gui);
        }
        else if (NID == EL_INTERNAL_ID_LUA)
        {
            return new LuaNodeEditor (node);
        }
        else if (NID == EL_INTERNAL_ID_SCRIPT)
        {
            return new ScriptNodeEditor (gui.getWorld().getScriptingEngine(), node);
        }
        else if (NID == EL_INTERNAL_ID_MIDI_PROGRAM_MAP)
        {
            auto* const pgced = new MidiProgramMapEditor (node);
            if (auto* object = dynamic_cast<MidiProgramMapNode*> (node.getObject()))
                pgced->setSize (object->getWidth(), object->getHeight());

            return pgced;
        }
        else if (NID == EL_INTERNAL_ID_AUDIO_ROUTER)
        {
            auto* ared = new AudioRouterEditor (node);
            ared->setAutoResize (true);
            ared->adjustBoundsToMatrixSize (32);
            return ared;
        }

        return nullptr;
    }

    NodeEditorComponent* instantiateForNavigationPanel (const Node& node)
    {
        if (node.isMidiInputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new MidiIONodeEditor (node, gui.getWorld().getMidiEngine(), true, false);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.isMidiOutputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new MidiIONodeEditor (node, gui.getWorld().getMidiEngine(), false, true);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.getIdentifier() == EL_INTERNAL_ID_MIDI_PROGRAM_MAP)
        {
            auto* const programChangeMapEditor = new MidiProgramMapEditor (node);
            programChangeMapEditor->setStoreSize (false);
            programChangeMapEditor->setFontSize (programChangeMapEditor->getDefaultFontSize(), false);
            programChangeMapEditor->setFontControlsVisible (false);
            return programChangeMapEditor;
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_MIDI_MONITOR)
        {
            auto* const midiMonitorEditor = new MidiMonitorNodeEditor (node);
            return midiMonitorEditor;
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_AUDIO_ROUTER)
        {
            auto* const audioRouterEditor = new AudioRouterEditor (node);
            return audioRouterEditor;
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_MIDI_ROUTER)
        {
            auto* const midiRouterEditor = new MidiRouterEditor (node);
            return midiRouterEditor;
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_SCRIPT)
        {
            auto* se = new ScriptNodeEditor (gui.getWorld().getScriptingEngine(), node);
            se->setToolbarVisible (false);
            return se;
        }

        NodeObjectPtr object = node.getObject();
        auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (proc != nullptr)
        {
            if (node.getFormat() == "Element" && ! proc->hasEditor())
                return new GenericNodeEditor (node);
        }

        return nullptr;
    }
};

NodeEditorFactory::NodeEditorFactory()
{
    add<AudioRouterEditor> (EL_INTERNAL_ID_AUDIO_ROUTER, NodeEditorPlacement::PluginWindow);
}

NodeEditorFactory::NodeEditorFactory (GuiController& g)
    : NodeEditorFactory()
{
    fallback.reset (new FallbackNodeEditorSource (g));
}

NodeEditorFactory::~NodeEditorFactory()
{
    fallback = nullptr;
}

std::unique_ptr<AudioProcessorEditor> NodeEditorFactory::createAudioProcessorEditor (const Node& node)
{
    std::unique_ptr<AudioProcessorEditor> editor = nullptr;
    NodeObjectPtr object = node.getObject();
    AudioProcessor* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;

    if (proc == nullptr)
        return nullptr;

    editor.reset (proc->hasEditor() ? proc->createEditorIfNeeded()
                                    : new GenericAudioProcessorEditor (proc));

    return editor;
}

} // namespace Element

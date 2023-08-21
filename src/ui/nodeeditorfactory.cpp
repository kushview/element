// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui.hpp>
#include <element/processor.hpp>
#include <element/ui/grapheditor.hpp>

#include "nodes/nodetypes.hpp"
#include "nodes/ionodeeditor.hpp"
#include "nodes/audioroutereditor.hpp"
#include "nodes/midideviceeditor.hpp"
#include "nodes/midimonitoredtor.hpp"
#include "nodes/midiprogrammapeditor.hpp"
#include "nodes/midiroutereditor.hpp"
#include "nodes/oscreceivereditor.hpp"
#include "nodes/oscsendereditor.hpp"
#include "nodes/volumeeditor.hpp"
#include "nodes/scriptnodeeditor.hpp"
#include "../nodes/mcu.hpp"

#include "ui/nodeeditorfactory.hpp"
#include "scripting.hpp"
#include <element/context.hpp>

namespace element {

class FallbackNodeEditorSource : public NodeEditorSource
{
    GuiService& gui;

public:
    FallbackNodeEditorSource (GuiService& g)
        : gui (g) {}

    void findEditors (Array<NodeEditorDescription>&) override {}

    NodeEditor* instantiate (const String& identifier, const Node& node, NodeEditorPlacement placement) override

    {
        if (node.getFormat() != EL_NODE_FORMAT_NAME || identifier != EL_NODE_EDITOR_DEFAULT_ID)
        {
            return nullptr;
        }

        const auto NID = node.getIdentifier().toString();
        if (NID == EL_NODE_ID_GRAPH)
        {
            return new GraphNodeEditor (node);
        }
        else if (NID == "el.MCU")
        {
            return new MackieControlEditor (node);
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
    NodeEditor* instantiateForPluginWindow (const Node& node)
    {
        const auto NID = node.getIdentifier().toString();
        if (NID == EL_NODE_ID_MIDI_ROUTER)
        {
            return new MidiRouterEditor (node);
        }
        else if (NID == EL_NODE_ID_MIDI_MONITOR)
        {
            return new MidiMonitorNodeEditor (node);
        }
        else if (NID == EL_NODE_ID_OSC_RECEIVER)
        {
            return new OSCReceiverNodeEditor (node);
        }
        else if (NID == EL_NODE_ID_OSC_SENDER)
        {
            return new OSCSenderNodeEditor (node);
        }
        else if (NID.contains (EL_NODE_ID_VOLUME))
        {
            return new VolumeNodeEditor (node, gui);
        }
        else if (NID == EL_NODE_ID_SCRIPT)
        {
            return new ScriptNodeEditor (gui.context().scripting(), node);
        }
        else if (NID == EL_NODE_ID_MIDI_PROGRAM_MAP)
        {
            auto* const pgced = new MidiProgramMapEditor (node);
            if (auto* object = dynamic_cast<MidiProgramMapNode*> (node.getObject()))
                pgced->setSize (object->getWidth(), object->getHeight());

            return pgced;
        }
        else if (NID == EL_NODE_ID_AUDIO_ROUTER)
        {
            auto* ared = new AudioRouterEditor (node);
            ared->setAutoResize (true);
            ared->adjustBoundsToMatrixSize (32);
            return ared;
        }

        return nullptr;
    }

    NodeEditor* instantiateForNavigationPanel (const Node& node)
    {
        if (node.isMidiInputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new MidiIONodeEditor (node, gui.context().midi(), true, false);
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
                return new MidiIONodeEditor (node, gui.context().midi(), false, true);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.getIdentifier() == EL_NODE_ID_MIDI_PROGRAM_MAP)
        {
            auto* const programChangeMapEditor = new MidiProgramMapEditor (node);
            programChangeMapEditor->setStoreSize (false);
            programChangeMapEditor->setFontSize (programChangeMapEditor->getDefaultFontSize(), false);
            programChangeMapEditor->setFontControlsVisible (false);
            return programChangeMapEditor;
        }
        else if (node.getIdentifier() == EL_NODE_ID_MIDI_MONITOR)
        {
            auto* const midiMonitorEditor = new MidiMonitorNodeEditor (node);
            return midiMonitorEditor;
        }
        else if (node.getIdentifier() == EL_NODE_ID_AUDIO_ROUTER)
        {
            auto* const audioRouterEditor = new AudioRouterEditor (node);
            return audioRouterEditor;
        }
        else if (node.getIdentifier() == EL_NODE_ID_MIDI_ROUTER)
        {
            auto* const midiRouterEditor = new MidiRouterEditor (node);
            return midiRouterEditor;
        }
        else if (node.getIdentifier() == EL_NODE_ID_SCRIPT)
        {
            auto* se = new ScriptNodeEditor (gui.context().scripting(), node);
            se->setToolbarVisible (false);
            return se;
        }

        ProcessorPtr object = node.getObject();
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
    add<AudioRouterEditor> (EL_NODE_ID_AUDIO_ROUTER, NodeEditorPlacement::PluginWindow);
}

NodeEditorFactory::NodeEditorFactory (GuiService& g)
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
    ProcessorPtr object = node.getObject();
    AudioProcessor* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;

    if (proc == nullptr)
        return nullptr;

    editor.reset (proc->hasEditor() ? proc->createEditorIfNeeded()
                                    : new GenericAudioProcessorEditor (*proc));

    return editor;
}

} // namespace element

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

#include "gui/GuiCommon.h"
#include "session/PluginManager.h"
#include "session/Presets.h"

namespace Element {

inline static void addMidiDevicesToMenu (PopupMenu& menu, const bool isInput,
                                         const int offset = 80000)
{
    jassert (offset > 0);
    const StringArray devices = isInput ? MidiInput::getDevices() : MidiOutput::getDevices();
    for (int i = 0; i < devices.size(); ++i)
        menu.addItem (i + offset, devices [i], true, false);
}

inline static String getMidiDeviceForMenuResult (const int result, const bool isInput,
                                                 const int offset = 80000)
{
    jassert (offset > 0 && result >= offset);
    const int index = result - offset;
    const StringArray devices = isInput ? MidiInput::getDevices() : MidiOutput::getDevices();
    return isPositiveAndBelow (index, devices.size()) ? devices [index] : String();
}

class PluginsPopupMenu : public PopupMenu
{
public:
    PluginsPopupMenu (Component* sender)
    {
        jassert (sender);
        auto* cc = ViewHelpers::findContentComponent (sender);
        jassert (cc);
        plugins = &cc->getGlobals().getPluginManager();
    }
    
    bool isPluginResultCode (const int resultCode)
    {
        return (plugins->getKnownPlugins().getIndexChosenByMenu (resultCode) >= 0) ||
               (isPositiveAndBelow (int(resultCode - 20000), unverified.size()));
    }
    
    const PluginDescription* getPluginDescription (int resultCode, bool& verified)
    {
        jassert (plugins);
        int index = plugins->getKnownPlugins().getIndexChosenByMenu (resultCode);
        if (index >= 0)
        {
            verified = true;
            return plugins->getKnownPlugins().getType (index);
        }
        
        verified = false;
        index = resultCode - 20000;
        return isPositiveAndBelow(index, unverified.size()) ? unverified.getUnchecked(index) : nullptr;
    }
    
    void addPluginItems()
    {
        if (hasAddedPlugins)
            return;
        hasAddedPlugins = true;
        plugins->getKnownPlugins().addToMenu (*this, KnownPluginList::sortByManufacturer);
    
        PopupMenu unvMenu;
       #if JUCE_MAC
        StringArray unvFormats = { "AudioUnit", "VST", "VST3", "LV2" };
       #else
        StringArray unvFormats = { "VST", "VST3" };
       #endif
        
        unverified.clearQuick (true);
        for (const auto& name : unvFormats)
        {
            PopupMenu menu;
            const int lastSize = unverified.size();
            plugins->getUnverifiedPlugins (name, unverified);
            if (auto* format = plugins->getAudioPluginFormat (name))
                for (int i = lastSize; i < unverified.size(); ++i)
                    menu.addItem (i + 20000, format->getNameOfPluginFromIdentifier (
                        unverified.getUnchecked(i)->fileOrIdentifier));
            if (menu.getNumItems() > 0)
                unvMenu.addSubMenu (name, menu);
        }
        
        if (unvMenu.getNumItems() > 0) {
            addSeparator();
            addSubMenu ("Unverified", unvMenu);
        }
    }
    
private:
    OwnedArray<PluginDescription> unverified;
    Component* sender;
    PluginManager* plugins;
    bool hasAddedPlugins = false;
};

// MARK: Node Popup Menu

class NodePopupMenu : public PopupMenu
{
public:
    enum ItemIds
    {
        Duplicate = 1,
        RemoveNode,
        Disconnect,
        DisconnectInputs,
        DisconnectOutputs,
        DisconnectMidi,
        LastItem
    };
    
    typedef std::initializer_list<ItemIds> ItemList;
    
    explicit NodePopupMenu() { }
    
    NodePopupMenu (const Node& n, std::function<void (NodePopupMenu&)> beforeMainItems = nullptr)
        : node (n)
    {
        if (beforeMainItems)
        {
            beforeMainItems (*this);
            addSeparator();
        }
        addMainItems (false);
    }
    
    NodePopupMenu (const Node& n, const Port& p)
        : node (n), port (p)
    {
        addMainItems (false);
        NodeArray siblings;
        addSeparator();
        
        if (port.isInput())
        {
            PopupMenu items;
            node.getPossibleSources (siblings);
            for (auto& src : siblings)
            {
                PopupMenu srcMenu;
                PortArray ports;
                src.getPorts (ports, PortType::Audio, false);
                if (ports.isEmpty())
                    continue;
                for (const auto& p : ports)
                    addItemInternal (srcMenu, p.getName(), new SingleConnectOp (src, p, node, port));
                items.addSubMenu (src.getName(), srcMenu);
            }
            
            addSubMenu ("Sources", items);
        }
        else
        {
            PopupMenu items;
            node.getPossibleDestinations (siblings);
            for (auto& dst : siblings)
            {
                PopupMenu srcMenu;
                PortArray ports;
                dst.getPorts (ports, PortType::Audio, true);
                if (ports.isEmpty())
                    continue;
                for (const auto& p : ports)
                    addItemInternal (srcMenu, p.getName(), new SingleConnectOp (node, port, dst, p));
                items.addSubMenu (dst.getName(), srcMenu);
            }
            
            addSubMenu ("Destinations", items);
        }
    }
    
    ~NodePopupMenu()
    {
        reset();
    }

    inline void addOptionsSubmenu()
    {
        PopupMenu menu;
        int index = 30000;
        GraphNodePtr ptr = node.getGraphNode();
        menu.addItem (index++, "Mute input ports", ptr != nullptr, ptr && ptr->isMutingInputs());
        addSubMenu ("Options", menu, ptr != nullptr);
    }

    inline void addReplaceSubmenu (PluginManager& plugins)
    {
        PopupMenu menu;
        plugins.getKnownPlugins()
            .addToMenu (menu, KnownPluginList::sortByCategory, 
                        node.getFileOrIdentifier().toString());
        addSubMenu ("Replace", menu);
    }

    inline void addProgramsMenu (const String& subMenuName = "Factory Presets")
    {
        PopupMenu programs; getProgramsMenu (programs);
        addSubMenu (subMenuName, programs);
    }
    
    inline void addPresetsMenu (PresetCollection& collection, const String& subMenuName = "Presets")
    {
        PopupMenu presets;
        getPresetsMenu (collection, presets);
        addSubMenu (subMenuName, presets);
    }
    
    inline void getPresetsMenu (PresetCollection& collection, PopupMenu& menu)
    {
       #if EL_USE_PRESETS
        const int offset = 20000;
        if (node.isAudioIONode() || node.isMidiIONode())
            return;
        const String format = node.getProperty (Tags::format).toString();
        addItemInternal (menu, "Add Preset", new AddPresetOp (node));
        
        menu.addSeparator();

        {
            PopupMenu progs;
            getProgramsMenu (progs);
            menu.addSubMenu ("Factory Presets", progs);
        }
        
        if (format == "VST")
        {
            PopupMenu native;
            addItemInternal (native, "Save FXB/FXP", new FXBPresetOp (node, false));
            addItemInternal (native, "Load FXB/FXP", new FXBPresetOp (node, true));
            menu.addSubMenu ("Native Presets", native);
        }
        
        auto identifier = node.getProperty(Tags::identifier).toString();
        if (identifier.isEmpty())
            identifier = node.getProperty (Tags::file);
        
        presetItems.clear();
        collection.getPresetsFor (node, presetItems);
       
        menu.addSeparator();
        
        if (presetItems.size() <= 0)
            menu.addItem (offset, "(none)", false);
        
        for (int i = 0; i < presetItems.size(); ++i)
            menu.addItem (offset + i, presetItems[i]->name);
       #endif
    }
    
    inline void getProgramsMenu (PopupMenu& menu)
    {
        const int offset = 10000;
        const int current = node.getCurrentProgram();
        for (int i = 0; i < node.getNumPrograms(); ++i) {
            menu.addItem (offset + i, node.getProgramName (i), true, i == current);
        }
    }
    
    Message* createMessageForResultCode (const int result)
    {
        if (result == RemoveNode)
            return new RemoveNodeMessage (node);
        else if (result == Duplicate)
            return new DuplicateNodeMessage (node);
        else if (result == Disconnect)
            return new DisconnectNodeMessage (node);
        else if (result == DisconnectInputs)
            return new DisconnectNodeMessage (node, true, false);
        else if (result == DisconnectOutputs)
            return new DisconnectNodeMessage (node, false, true);
        else if (result == DisconnectMidi)
            return new DisconnectNodeMessage (node, true, true, false, true);
        else if (auto* op = resultMap [result])
        {
            if (auto* const msg = op->createMessage())
                return msg;
            op->perform();
        }
        else if (result >= 10000 && result < 20000)
        {
            Node(node).setCurrentProgram (result - 10000);
        }
        else if (result >= 20000 && result < 30000)
        {
            Node n (node);
            const int index = result - 20000;
            if (auto* const item = presetItems [index])
            {
                const auto data = Node::parse (item->file);
                if (n.isValid() && data.isValid() && data.hasProperty (Tags::state))
                {
                    const String state = data.getProperty(Tags::state).toString();
                    n.getValueTree().setProperty (Tags::state, state, 0);
                    if (data.hasProperty (Tags::programState))
                        n.getValueTree().setProperty (Tags::programState, data.getProperty (Tags::programState), 0);
                    n.restorePluginState();
                }

                if (n.isValid() && data.isValid() && data.hasProperty (Tags::name))
                {
                    if (data[Tags::name].toString().isNotEmpty())
                        n.setProperty (Tags::name, data[Tags::name]);
                }
            }
        }
        else if (result >= 30000 && result < 40000)
        {
            const int index = result - 30000;
            switch (index)
            {
                case 0:
                    node.setMuteInput (! node.isMutingInputs());
                    break;
            }
        }
        
        return nullptr;
    }
    
    Message* showAndCreateMessage()
    {
        return createMessageForResultCode (this->show());
    }
    
    void reset()
    {
        this->clear();
        resultMap.clear();
        deleter.clearQuick (true);
        presetItems.clear();
        currentResultOpId = firstResultOpId;
    }

    void setNode (const Node& n, const bool header = true)
    {
        reset();
        node = n;
        if (node.isValid())
            addMainItems (header);
    }

private:
    Node node;
    OwnedArray<PresetDescription> presetItems;
    Port port;
    const int firstResultOpId = 1024;
    int currentResultOpId = 1024;
    
    struct ResultOp
    {
        ResultOp() { }
        virtual ~ResultOp () { }
        virtual bool isActive() { return true; }
        virtual bool isTicked() { return false; }
        virtual Message* createMessage() { return nullptr; }
        virtual bool perform() { return false; }
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResultOp);
    };
    
    struct EnableNodeOp : public ResultOp
    {
        const Node node;
        EnableNodeOp (const Node& n) : node (n) { }
        bool isTicked() override { return false; }
        bool perform() override
        {
            if (GraphNodePtr ptr = node.getGraphNode())
            {
                ptr->setEnabled (! ptr->isEnabled());
                auto data = node.getValueTree();
                data.setProperty (Tags::enabled, ptr->isEnabled(), nullptr);
                return true;
            }

            return false;
        }
    };

    struct SingleConnectOp : public ResultOp
    {
        SingleConnectOp (const Node& sn, const Port& sp, const Node& dn, const Port& dp)
            : sourceNode(sn), destNode(dn),  sourcePort (sp), destPort (dp)
        { }
        
        const Node sourceNode, destNode;
        const Port sourcePort, destPort;
        
        bool isTicked()
        {
            return Node::connectionExists (sourceNode.getParentArcsNode(),
                                           sourceNode.getNodeId(), sourcePort.getIndex(),
                                           destNode.getNodeId(), destPort.getIndex());
        }
        
        Message* createMessage()
        {
            return new AddConnectionMessage (sourceNode.getNodeId(), sourcePort.getIndex(),
                                             destNode.getNodeId(), destPort.getIndex());
        }
    };
    
    struct AddPresetOp : public ResultOp
    {
        AddPresetOp (const Node& n)
            : node (n) { }
        const Node node;
        Message* createMessage()
        {
            return new AddPresetMessage (node);
        }
    };

    struct FXBPresetOp : public ResultOp
    {
        FXBPresetOp (const Node& n, const bool isLoad)
            : node (n), load (isLoad) { }
        const Node node;
        const bool load;
        bool perform() override
        {
           #if JUCE_PLUGINHOST_VST
            const auto format = node.getProperty(Tags::format).toString();
            if (format != "VST")
                return false;
            
            auto gn = node.getGraphNode();
            auto* const proc = (gn) ? gn->getAudioPluginInstance() : nullptr;

            if (! proc)
                return false;

            if (load)
            {
                DataPath dataPath;
                const auto file = dataPath.getRootDir().getChildFile("Presets");
                FileChooser chooser ("Open FXB/FXP Preset", File(), "*.fxb;*.fxp", true);
                bool wasOk = true;
                if (chooser.browseForFileToOpen())
                {
                    FileInputStream stream (chooser.getResult());
                    MemoryBlock block;
                    stream.readIntoMemoryBlock (block);
                    if (block.getSize() > 0)
                        wasOk = VSTPluginFormat::loadFromFXBFile (proc, block.getData(), block.getSize());
                }

                if (! wasOk)
                {
                    // TODO: alert
                }
            }
            else
            {
                DataPath dataPath;
                String path = "Presets/"; path << proc->getName();
                const auto file = dataPath.getRootDir().getChildFile(path)
                    .withFileExtension("fxp").getNonexistentSibling();
                FileChooser chooser ("Save FXB/FXP Preset", file, "*.fxb;*.fxp", true);
                if (chooser.browseForFileToSave (true))
                {
                    const File f (chooser.getResult());
                    MemoryBlock block;
                    if (VSTPluginFormat::saveToFXBFile (proc, block, f.hasFileExtension ("fxb")))
                    {
                        FileOutputStream stream (f);
                        stream.write (block.getData(), block.getSize());
                        stream.flush();
                    }
                    else
                    {
                        // TODO: alert
                    }
                }
            }

            return true;
            
           #else
            DBG("[EL] FXB/FXP presets not yet supported on this platform.");
            return true;
           #endif
        }
    };

    HashMap<int, ResultOp*> resultMap;
    OwnedArray<ResultOp> deleter;
    
    void addMainItems (const bool showHeader)
    {
        if (showHeader)
            addSectionHeader (node.getName());

        addItemInternal (*this, node.isEnabled() ? "Disable" : "Enable", new EnableNodeOp (node));
        addSeparator();

        {
            PopupMenu disconnect;
            disconnect.addItem (Disconnect, "All Ports");
            disconnect.addItem (DisconnectMidi, "MIDI Ports");
            disconnect.addSeparator();
            disconnect.addItem (DisconnectInputs, "Input Ports");
            disconnect.addItem (DisconnectOutputs, "Output Ports");
            addSubMenu ("Disconnect", disconnect);
        }

        addItem (Duplicate, getNameForItem (Duplicate), !node.isIONode());
        addSeparator();
        addItem (RemoveNode, getNameForItem (RemoveNode));
    }
    
    void addItemInternal (PopupMenu& menu, const String& name, ResultOp* op)
    {
        menu.addItem (currentResultOpId, name, op->isActive(), op->isTicked());
        resultMap.set (currentResultOpId, deleter.add (op));
        ++currentResultOpId;
    }
    
    String getNameForItem (ItemIds item)
    {
        switch (item)
        {
            case Disconnect: return "Disconnect"; break;
            case Duplicate:  return "Duplicate"; break;
            case RemoveNode: return "Remove"; break;
            default: jassertfalse; break;
        }
        return "Unknown Item";
    }
};

}

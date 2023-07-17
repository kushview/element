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

#include <element/plugins.hpp>
#include <element/processor.hpp>

#include "engine/graphnode.hpp"
#include "gui/GuiCommon.h"
#include "session/presetmanager.hpp"
#include "gui/BlockComponent.h"
#include "./utils.hpp"

namespace element {

class PluginsPopupMenu : public PopupMenu
{
public:
    PluginsPopupMenu (Component* sender)
    {
        jassert (sender != nullptr);
        auto* cc = ViewHelpers::findContentComponent (sender);
        jassert (cc != nullptr);
        plugins = &cc->context().plugins();
        jassert (plugins != nullptr);
        available = plugins->getKnownPlugins().getTypes();
    }

    bool isPluginResultCode (const int resultCode)
    {
        return (plugins->getKnownPlugins().getIndexChosenByMenu (available, resultCode) >= 0) || (isPositiveAndBelow (int (resultCode - 20000), unverified.size()));
    }

    PluginDescription getPluginDescription (int resultCode, bool& verified)
    {
        jassert (plugins != nullptr);
        int index = plugins->getKnownPlugins().getIndexChosenByMenu (available, resultCode);
        if (isPositiveAndBelow (index, available.size()))
        {
            verified = true;
            return available.getReference (index);
        }

        verified = false;
        index = resultCode - 20000;
        return isPositiveAndBelow (index, unverified.size())
                   ? *unverified.getUnchecked (index)
                   : PluginDescription();
    }

    void addPluginItems()
    {
        if (hasAddedPlugins)
            return;
        hasAddedPlugins = true;
        plugins->getKnownPlugins().addToMenu (*this, available, KnownPluginList::sortByManufacturer);

        PopupMenu unvMenu;
        unverified.clearQuick (true);
        for (const auto& name : Util::getSupportedAudioPluginFormats())
        {
            PopupMenu menu;
            const int lastSize = unverified.size();
            plugins->getUnverifiedPlugins (name, unverified);
            if (auto* format = plugins->getAudioPluginFormat (name))
                for (int i = lastSize; i < unverified.size(); ++i)
                    menu.addItem (i + 20000, format->getNameOfPluginFromIdentifier (unverified.getUnchecked (i)->fileOrIdentifier));
            if (menu.getNumItems() > 0)
                unvMenu.addSubMenu (name, menu);
        }

        if (unvMenu.getNumItems() > 0)
        {
            addSeparator();
            addSubMenu ("Unverified", unvMenu);
        }
    }

private:
    Array<PluginDescription> available;
    OwnedArray<PluginDescription> unverified;
    Component* sender { nullptr };
    PluginManager* plugins { nullptr };
    bool hasAddedPlugins = false;
};

//==============================================================================
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

    explicit NodePopupMenu() {}

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
        ProcessorPtr ptr = node.getObject();
        menu.addItem (index++, "Mute input ports", ptr != nullptr, ptr && ptr->isMutingInputs());
        addOversamplingSubmenu (menu);
        addSubMenu (TRANS ("Options"), menu, ptr != nullptr);
    }

    inline void addColorSubmenu (ColourSelector& selector)
    {
        PopupMenu color;
        color.addCustomItem (std::numeric_limits<int>::max(),
                             selector,
                             220,
                             300,
                             false,
                             nullptr);
        addSubMenu (TRANS ("Color"), color, true);
    }

    inline void addOversamplingSubmenu (PopupMenu& menuToAddTo)
    {
        PopupMenu osMenu;
        int index = 40000;
        ProcessorPtr ptr = node.getObject();

        if (ptr == nullptr || ptr->isAudioIONode() || ptr->isMidiIONode()) // not the right type of node
            return;

        osMenu.addItem (index++, "Off", true, ptr->getOversamplingFactor() == 1);
        osMenu.addSeparator();
        osMenu.addItem (index++, "2x", true, ptr->getOversamplingFactor() == 2);
        osMenu.addItem (index++, "4x", true, ptr->getOversamplingFactor() == 4);
        osMenu.addItem (index++, "8x", true, ptr->getOversamplingFactor() == 8);

        menuToAddTo.addSubMenu ("Oversample", osMenu);
    }

    inline void addReplaceSubmenu (PluginManager& plugins)
    {
        PopupMenu menu;
        KnownPluginList::addToMenu (menu,
                                    plugins.getKnownPlugins().getTypes(),
                                    KnownPluginList::sortByCategory,
                                    node.getFileOrIdentifier().toString());
        addSubMenu ("Replace", menu);
    }

    inline void addProgramsMenu (const String& subMenuName = "Factory Presets")
    {
        PopupMenu programs;
        getProgramsMenu (programs);
        addSubMenu (subMenuName, programs);
    }

    inline void addPresetsMenu (PresetManager& collection, const String& subMenuName = "Presets")
    {
        PopupMenu presets;
        getPresetsMenu (collection, presets);
        addSubMenu (subMenuName, presets);
    }

    inline void getPresetsMenu (PresetManager& collection, PopupMenu& menu)
    {
        const int offset = 20000;
        if (node.isAudioIONode() || node.isMidiIONode())
            return;
        const String format = node.getProperty (tags::format).toString();
        addItemInternal (menu, "Add Preset", new AddPresetOp (node));
        addItemInternal (menu, "Save as default...", new SaveDefaultNodeOp (node));
        addItemInternal (menu, "Reset default...", new ResetDefaultNodeOp (node));
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

        auto identifier = node.getProperty (tags::identifier).toString();
        if (identifier.isEmpty())
            identifier = node.getProperty (tags::file);

        presetItems.clear();
        collection.getPresetsFor (node, presetItems);

        menu.addSeparator();

        if (presetItems.size() <= 0)
            menu.addItem (offset, "(none)", false);

        for (int i = 0; i < presetItems.size(); ++i)
            menu.addItem (offset + i, presetItems[i]->name);
    }

    inline void getProgramsMenu (PopupMenu& menu)
    {
        const int offset = 10000;
        const int current = node.getCurrentProgram();
        for (int i = 0; i < node.getNumPrograms(); ++i)
        {
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
        else if (auto* op = resultMap[result])
        {
            if (auto* const msg = op->createMessage())
                return msg;
            op->perform();
        }
        else if (result >= 10000 && result < 20000)
        {
            Node (node).setCurrentProgram (result - 10000);
        }
        else if (result >= 20000 && result < 30000)
        {
            Node n (node);
            const int index = result - 20000;
            if (auto* const item = presetItems[index])
            {
                const auto data = File::isAbsolutePath (item->file)
                                      ? Node::parse (File (item->file))
                                      : ValueTree();
                if (n.isValid() && data.isValid() && data.hasProperty (tags::state))
                {
                    const String state = data.getProperty (tags::state).toString();
                    n.data().setProperty (tags::state, state, 0);
                    if (data.hasProperty (tags::programState))
                        n.data().setProperty (tags::programState, data.getProperty (tags::programState), 0);
                    n.restorePluginState();
                }

                if (n.isValid() && data.isValid() && data.hasProperty (tags::name))
                {
                    if (data[tags::name].toString().isNotEmpty())
                        n.setProperty (tags::name, data[tags::name]);
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
        else if (result >= 40000 && result < 50000)
        {
            const int osFactor = (int) powf (2, float (result - 40000));
            if (auto gNode = node.getObject())
                gNode->setOversamplingFactor (osFactor);
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
    OwnedArray<PresetInfo> presetItems;
    Port port;
    const int firstResultOpId = 1024;
    int currentResultOpId = 1024;

    struct ResultOp
    {
        ResultOp() {}
        virtual ~ResultOp() {}
        virtual bool isActive() { return true; }
        virtual bool isTicked() { return false; }
        virtual Message* createMessage() { return nullptr; }
        virtual bool perform() { return false; }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResultOp);
    };

    struct EnableNodeOp : public ResultOp
    {
        const Node node;
        EnableNodeOp (const Node& n) : node (n) {}
        bool isTicked() override { return false; }
        bool perform() override
        {
            if (ProcessorPtr ptr = node.getObject())
            {
                ptr->setEnabled (! ptr->isEnabled());
                auto data = node.data();
                data.setProperty (tags::enabled, ptr->isEnabled(), nullptr);
                return true;
            }

            return false;
        }
    };

    struct SingleConnectOp : public ResultOp
    {
        SingleConnectOp (const Node& sn, const Port& sp, const Node& dn, const Port& dp)
            : sourceNode (sn), destNode (dn), sourcePort (sp), destPort (dp)
        {
        }

        const Node sourceNode, destNode;
        const Port sourcePort, destPort;

        bool isTicked()
        {
            return Node::connectionExists (sourceNode.getParentArcsNode(),
                                           sourceNode.getNodeId(),
                                           sourcePort.index(),
                                           destNode.getNodeId(),
                                           destPort.index());
        }

        Message* createMessage()
        {
            return new AddConnectionMessage (sourceNode.getNodeId(), sourcePort.index(), destNode.getNodeId(), destPort.index());
        }
    };

    struct AddPresetOp : public ResultOp
    {
        AddPresetOp (const Node& n)
            : node (n) {}
        const Node node;
        Message* createMessage()
        {
            return new AddPresetMessage (node);
        }
    };

    struct SaveDefaultNodeOp : public ResultOp
    {
        SaveDefaultNodeOp (const Node& n)
            : node (n) {}
        const Node node;
        Message* createMessage()
        {
            return new SaveDefaultNodeMessage (node);
        }
    };

    struct ResetDefaultNodeOp : public ResultOp
    {
        ResetDefaultNodeOp (const Node& n)
            : node (n) {}
        const Node node;
        bool perform() override
        {
            String subpath = "nodes/";
            subpath << node.getProperty (tags::pluginIdentifierString).toString()
                    << "/default.eln";
            auto file = DataPath::applicationDataDir().getChildFile (subpath);
            if (file.existsAsFile())
                file.deleteFile();
            return true;
        }
    };

    struct FXBPresetOp : public ResultOp
    {
        FXBPresetOp (const Node& n, const bool isLoad)
            : node (n), load (isLoad) {}
        const Node node;
        const bool load;
        bool perform() override
        {
#if JUCE_PLUGINHOST_VST
            const auto format = node.getProperty (tags::format).toString();
            if (format != "VST")
                return false;

            auto gn = node.getObject();
            auto* const proc = (gn) ? gn->getAudioPluginInstance() : nullptr;

            if (! proc)
                return false;

            if (load)
            {
                DataPath dataPath;
                const auto file = dataPath.getRootDir().getChildFile ("Presets");
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
                String path = "Presets/";
                path << proc->getName();
                const auto file = dataPath.getRootDir().getChildFile (path).withFileExtension ("fxp").getNonexistentSibling();
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
            DBG ("[element] FXB/FXP presets not yet supported on this platform.");
            return true;
#endif
        }
    };

    struct RenameNodeOp : public ResultOp
    {
        RenameNodeOp (const Node& n)
            : node (n) {}
        Node node;
        bool perform() override
        {
            AlertWindow win ("Rename Node", "Enter a new node name:", AlertWindow::NoIcon, nullptr);
            win.addTextEditor ("name", node.getName(), "", false);
            win.addButton ("Rename", 1, KeyPress (KeyPress::returnKey));
            win.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

            if (1 == win.runModalLoop())
            {
                if (auto* const ed = win.getTextEditor ("name"))
                    if (ed->getText().isNotEmpty())
                        node.setProperty (tags::name, ed->getText());
            }

            return true;
        }
    };

    HashMap<int, ResultOp*> resultMap;
    OwnedArray<ResultOp> deleter;

    void addMainItems (const bool showHeader)
    {
        if (showHeader)
            addSectionHeader (node.getName());

        addItemInternal (*this, node.isEnabled() ? "Disable" : "Enable", new EnableNodeOp (node));
        addItemInternal (*this, "Rename", new RenameNodeOp (node));
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

        addItem (Duplicate, getNameForItem (Duplicate), ! node.isIONode());
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
            case Disconnect:
                return "Disconnect";
                break;
            case Duplicate:
                return "Duplicate";
                break;
            case RemoveNode:
                return "Remove";
                break;
            default:
                jassertfalse;
                break;
        }
        return "Unknown Item";
    }
};

} // namespace element

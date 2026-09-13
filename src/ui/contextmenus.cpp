// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/guicommon.hpp"
#include "ui/contextmenus.hpp"
#include "presetmanager.hpp"

namespace element {

namespace {

void toggleEnabled (Node node)
{
    if (ProcessorPtr ptr = node.getObject())
    {
        ptr->setEnabled (! ptr->isEnabled());
        auto data = node.data();
        data.setProperty (tags::enabled, ptr->isEnabled(), nullptr);
    }
}

void renameNode (Node node)
{
    AlertWindow win ("Rename Node", "Enter a new node name:", AlertWindow::NoIcon, nullptr);
    win.addTextEditor ("name", node.getName(), "", false);
    win.addButton ("Rename", 1, KeyPress (KeyPress::returnKey));
    win.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

    if (1 == win.runModalLoop())
        if (auto* const ed = win.getTextEditor ("name"))
            if (ed->getText().isNotEmpty())
                node.setProperty (tags::name, ed->getText());
}

void resetDefaultNode (const Node& node)
{
    String subpath = "nodes/";
    subpath << node.getProperty (tags::pluginIdentifierString).toString()
            << "/default.eln";
    auto file = DataPath::applicationDataDir().getChildFile (subpath);
    if (file.existsAsFile())
        file.deleteFile();
}

void loadPreset (Node node, const PresetInfo& preset)
{
    const auto data = File::isAbsolutePath (preset.file)
                          ? Node::parse (File (preset.file))
                          : ValueTree();
    if (! node.isValid() || ! data.isValid())
        return;

    if (data.hasProperty (tags::state))
    {
        node.data().setProperty (tags::state, data.getProperty (tags::state).toString(), nullptr);
        if (data.hasProperty (tags::programState))
            node.data().setProperty (tags::programState, data.getProperty (tags::programState), nullptr);
        node.restorePluginState();
    }

    if (data[tags::name].toString().isNotEmpty())
        node.setProperty (tags::name, data[tags::name]);
}

void loadOrSaveFXB (const Node& node, bool load)
{
#if JUCE_PLUGINHOST_VST
    if (node.getProperty (tags::format).toString() != "VST")
        return;

    auto gn = node.getObject();
    auto* const proc = (gn) ? gn->getAudioPluginInstance() : nullptr;
    if (! proc)
        return;

    if (load)
    {
        FileChooser chooser ("Open FXB/FXP Preset", File(), "*.fxb;*.fxp", true);
        bool wasOk = true;
        if (chooser.browseForFileToOpen())
        {
            FileInputStream stream (chooser.getResult());
            MemoryBlock block;
            stream.readIntoMemoryBlock (block);
            if (block.getSize() > 0)
            {
                if (auto* vst = proc->getVSTClient())
                    wasOk = vst->loadFromFXBFile ({ static_cast<const std::byte*> (block.getData()), block.getSize() });
            }
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
            auto* vst = proc->getVSTClient();
            if (vst != nullptr && vst->saveToFXBFile (block, f.hasFileExtension ("fxb")))
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
#else
    juce::ignoreUnused (node, load);
    DBG ("[element] FXB/FXP presets not yet supported on this platform.");
#endif
}

/** Adds the plugins of a KnownPluginList tree as items, one submenu per
    folder, with the plugin matching currentIdentifier ticked. */
void addPluginTreeItems (PopupMenu& menu,
                         const KnownPluginList::PluginTree& tree,
                         const String& currentIdentifier,
                         const std::function<void (const PluginDescription&)>& onChosen)
{
    for (auto* sub : tree.subFolders)
    {
        PopupMenu subMenu;
        addPluginTreeItems (subMenu, *sub, currentIdentifier, onChosen);
        menu.addSubMenu (sub->folder, subMenu);
    }

    for (const auto& desc : tree.plugins)
    {
        auto name = desc.name;
        for (const auto& other : tree.plugins)
        {
            if (&other != &desc && other.name == desc.name)
            {
                name << " (" << desc.pluginFormatName << ")";
                break;
            }
        }

        menu.addItem (name, true, desc.fileOrIdentifier == currentIdentifier, [onChosen, desc]() {
            onChosen (desc);
        });
    }
}

} // namespace

//==============================================================================
NodePopupMenu::NodePopupMenu (Component* c, const Node& n, std::function<void()> removeAction)
    : sender (c), node (n)
{
    addMainItems (std::move (removeAction));
}

NodePopupMenu::NodePopupMenu (Component* c, const Node& n, const Port& p)
    : sender (c), node (n)
{
    addMainItems (nullptr);
    addSeparator();
    addConnectionSubmenu (p);
}

std::function<void()> NodePopupMenu::postAction (std::function<Message*()> makeMessage) const
{
    auto target = sender;
    return [target, makeMessage]() {
        if (auto* c = target.getComponent())
            ViewHelpers::postMessageFor (c, makeMessage());
    };
}

//==============================================================================
void NodePopupMenu::addMainItems (std::function<void()> removeAction)
{
    const auto n = node;

    addItem (node.isEnabled() ? "Disable" : "Enable", [n]() { toggleEnabled (n); });
    addItem ("Rename", [n]() { renameNode (n); });
    addSeparator();

    {
        PopupMenu disconnect;
        disconnect.addItem ("All Ports", postAction ([n]() { return new DisconnectNodeMessage (n); }));
        disconnect.addItem ("MIDI Ports", postAction ([n]() { return new DisconnectNodeMessage (n, true, true, false, true); }));
        disconnect.addSeparator();
        disconnect.addItem ("Input Ports", postAction ([n]() { return new DisconnectNodeMessage (n, true, false); }));
        disconnect.addItem ("Output Ports", postAction ([n]() { return new DisconnectNodeMessage (n, false, true); }));
        addSubMenu ("Disconnect", disconnect);
    }

    addItem ("Duplicate", ! node.isIONode(), false, postAction ([n]() { return new DuplicateNodeMessage (n); }));
    addSeparator();

    if (! removeAction)
        removeAction = postAction ([n]() { return new RemoveNodeMessage (n); });
    addItem ("Remove", std::move (removeAction));
}

void NodePopupMenu::addConnectionSubmenu (const Port& port)
{
    const bool isInput = port.isInput();
    NodeArray siblings;
    if (isInput)
        node.getPossibleSources (siblings);
    else
        node.getPossibleDestinations (siblings);

    PopupMenu items;
    for (auto& other : siblings)
    {
        PortArray ports;
        other.getPorts (ports, PortType::Audio, ! isInput);
        if (ports.isEmpty())
            continue;

        PopupMenu portMenu;
        for (const auto& p : ports)
        {
            const Node src = isInput ? other : node;
            const Port srcPort = isInput ? p : port;
            const Node dst = isInput ? node : other;
            const Port dstPort = isInput ? port : p;

            const bool ticked = Node::connectionExists (src.getParentArcsNode(),
                                                        src.getNodeId(),
                                                        srcPort.index(),
                                                        dst.getNodeId(),
                                                        dstPort.index());

            portMenu.addItem (p.getName(), true, ticked, postAction ([src, srcPort, dst, dstPort]() {
                                  return new AddConnectionMessage (src.getNodeId(), srcPort.index(), dst.getNodeId(), dstPort.index());
                              }));
        }

        items.addSubMenu (other.getName(), portMenu);
    }

    addSubMenu (isInput ? "Sources" : "Destinations", items);
}

//==============================================================================
void NodePopupMenu::addOptionsSubmenu()
{
#if ! ELEMENT_SE
    PopupMenu menu;
    ProcessorPtr ptr = node.getObject();
    auto n = node;
    menu.addItem ("Mute input ports", ptr != nullptr, ptr && ptr->isMutingInputs(), [n]() mutable {
        n.setMuteInput (! n.isMutingInputs());
    });
    addOversamplingSubmenu (menu);
    addSubMenu (TRANS ("Options"), menu, ptr != nullptr);
#endif
}

void NodePopupMenu::addOversamplingSubmenu (PopupMenu& menuToAddTo)
{
    ProcessorPtr ptr = node.getObject();
    if (ptr == nullptr || ptr->isAudioIONode() || ptr->isMidiIONode())
        return;

    PopupMenu osMenu;
    const auto n = node;
    const int current = ptr->getOversamplingFactor();
    for (const int factor : { 1, 2, 4, 8 })
    {
        const String name = factor == 1 ? "Off" : String (factor) + "x";
        osMenu.addItem (name, true, current == factor, [n, factor]() {
            if (auto obj = n.getObject())
                obj->setOversamplingFactor (factor);
        });
        if (factor == 1)
            osMenu.addSeparator();
    }

    menuToAddTo.addSubMenu ("Oversample", osMenu);
}

void NodePopupMenu::addColorSubmenu (ColourSelector& selector)
{
    PopupMenu color;
    color.addCustomItem (std::numeric_limits<int>::max(), selector, 220, 300, false, nullptr);
    addSubMenu (TRANS ("Color"), color, true);
}

void NodePopupMenu::addReplaceSubmenu (PluginManager& plugins)
{
#if ! ELEMENT_SE
    const auto tree = KnownPluginList::createTree (plugins.getVisiblePluginTypes(),
                                                   KnownPluginList::sortByManufacturer);
    if (tree == nullptr)
        return;

    auto target = sender;
    const auto n = node;
    PopupMenu menu;
    addPluginTreeItems (menu, *tree, node.getFileOrIdentifier().toString(), [target, n] (const PluginDescription& desc) {
        if (auto* c = target.getComponent())
            ViewHelpers::postMessageFor (c, new ReplaceNodeMessage (n, desc));
    });
    addSubMenu ("Replace", menu);
#else
    juce::ignoreUnused (plugins);
#endif
}

//==============================================================================
void NodePopupMenu::addPresetsMenu (PresetManager& presets, const String& name)
{
    PopupMenu menu;
    addPresetItems (presets, menu);
    addSubMenu (name, menu, ! node.isGraph() && ! node.isIONode());
}

void NodePopupMenu::addPresetItems (PresetManager& presets, PopupMenu& menu)
{
    if (node.isDuplex())
        return;

    const auto n = node;
    menu.addItem (TRANS ("Save node..."), postAction ([n]() { return new AddPresetMessage (n); }));
    menu.addItem ("Save as default...", postAction ([n]() { return new SaveDefaultNodeMessage (n); }));
    menu.addItem ("Reset default...", [n]() { resetDefaultNode (n); });
    menu.addSeparator();

    {
        PopupMenu programs;
        addProgramItems (programs);
        menu.addSubMenu (TRANS ("Factory Presets"), programs);
    }

    if (node.getProperty (tags::format).toString() == "VST")
    {
        PopupMenu native;
        native.addItem ("Save FXB/FXP", [n]() { loadOrSaveFXB (n, false); });
        native.addItem ("Load FXB/FXP", [n]() { loadOrSaveFXB (n, true); });
        menu.addSubMenu (TRANS ("Native Presets"), native);
    }

#if ! ELEMENT_SE
    OwnedArray<PresetInfo> items;
    presets.getPresetsFor (node, items);

    menu.addSeparator();
    if (items.isEmpty())
        menu.addItem ("(none)", false, false, nullptr);

    for (const auto* preset : items)
    {
        const PresetInfo info (*preset);
        menu.addItem (String (info.name), [n, info]() { loadPreset (n, info); });
    }
#else
    juce::ignoreUnused (presets);
#endif
}

void NodePopupMenu::addProgramItems (PopupMenu& menu)
{
    const int current = node.getCurrentProgram();
    for (int i = 0; i < node.getNumPrograms(); ++i)
    {
        auto n = node;
        menu.addItem (node.getProgramName (i), true, i == current, [n, i]() mutable {
            n.setCurrentProgram (i);
        });
    }
}

//==============================================================================
void NodePopupMenu::addScriptItems()
{
    if (! node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_SCRIPT))
        return;

    auto target = sender;
    const auto n = node;
    addSeparator();
    addItem ("Edit DSP Script", [target, n]() { ViewHelpers::presentScriptEditor (target, n, false); });
    addItem ("Edit UI Script", [target, n]() { ViewHelpers::presentScriptEditor (target, n, true); });
}

} // namespace element

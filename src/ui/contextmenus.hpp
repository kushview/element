// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/plugins.hpp>
#include <element/processor.hpp>

#include "engine/graphnode.hpp"
#include "ui/guicommon.hpp"
#include "presetmanager.hpp"
#include "ui/block.hpp"
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
        available = plugins->getVisiblePluginTypes();
        favorites = plugins->getFavoritePluginTypes();
    }

    bool isPluginResultCode (const int resultCode)
    {
        // clang-format off
        return (isPositiveAndBelow (int (resultCode - favoriteResultOffset), favorites.size())) ||
               (plugins->getKnownPlugins().getIndexChosenByMenu (available, resultCode) >= 0) ||
               (isPositiveAndBelow (int (resultCode - 20000), unverified.size()));
        // clang-format on
    }

    PluginDescription getPluginDescription (int resultCode, bool& verified)
    {
        jassert (plugins != nullptr);

        // Favorites submenu — a subset of the visible/known plugins, so verified.
        int index = resultCode - favoriteResultOffset;
        if (isPositiveAndBelow (index, favorites.size()))
        {
            verified = true;
            return favorites.getReference (index);
        }

        index = plugins->getKnownPlugins().getIndexChosenByMenu (available, resultCode);
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

        if (! favorites.isEmpty())
        {
            PopupMenu favMenu;
            for (int i = 0; i < favorites.size(); ++i)
                favMenu.addItem (favoriteResultOffset + i, favorites.getReference (i).name);
            addSubMenu ("Favorites", favMenu);
            addSeparator();
        }

        plugins->getKnownPlugins().addToMenu (*this, available, KnownPluginList::sortByManufacturer);

        PopupMenu unvMenu;
        unverified.clearQuick (true);
        for (const auto& name : Util::compiledAudioPluginFormats())
        {
            PopupMenu menu;
            const int lastSize = unverified.size();
            plugins->getUnverifiedPlugins (name, unverified);
            if (auto* format = plugins->getAudioPluginFormat (name))
            {
                for (int i = lastSize; i < unverified.size(); ++i)
                    menu.addItem (i + 20000, format->getNameOfPluginFromIdentifier (unverified.getUnchecked (i)->fileOrIdentifier));
            }
            else if (name == "LV2")
            {
                for (int i = lastSize; i < unverified.size(); ++i)
                    menu.addItem (i + 20000, unverified[i]->name);
            }
            else
            {
                // Provider formats (e.g. CLAP): identifiers are file paths.
                for (int i = lastSize; i < unverified.size(); ++i)
                {
                    const auto& fid = unverified.getUnchecked (i)->fileOrIdentifier;
                    menu.addItem (i + 20000,
                                  juce::File::isAbsolutePath (fid)
                                      ? juce::File (fid).getFileNameWithoutExtension()
                                      : fid);
                }
            }

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
    // Favorites use a private menu-id range so they never collide with the
    // main list (which uses KnownPluginList's large menuIdBase) or the
    // unverified submenu (which uses the 20000 range).
    static constexpr int favoriteResultOffset = 10000;
    Array<PluginDescription> available, favorites;
    OwnedArray<PluginDescription> unverified;
    PluginManager* plugins { nullptr };
    bool hasAddedPlugins = false;
};

//==============================================================================
/** Context menu for a single node.

    Every item performs its own action when chosen, so callers only build the
    menu and show it. JUCE dispatches item actions asynchronously, and each
    action captures the node and the sender by value, so neither the menu nor
    the component that showed it needs to outlive the show call.
*/
class NodePopupMenu : public juce::PopupMenu
{
public:
    /** Creates the menu with the standard items for a node.

        @param sender       Component used to locate the Content that receives messages
        @param node         The node the menu acts on
        @param removeAction Optional replacement for the default "Remove" behaviour,
                            which posts a RemoveNodeMessage for the node
    */
    NodePopupMenu (juce::Component* sender, const Node& node, std::function<void()> removeAction = nullptr);

    /** Creates the menu for a node and one of its ports, adding a submenu of
        the possible connections for that port.

        @param sender Component used to locate the Content that receives messages
        @param node   The node the menu acts on
        @param port   The port to list connections for
    */
    NodePopupMenu (juce::Component* sender, const Node& node, const Port& port);

    /** Adds the "Options" submenu: mute inputs and oversampling. */
    void addOptionsSubmenu();

    /** Adds the "Color" submenu containing the given selector. */
    void addColorSubmenu (juce::ColourSelector& selector);

    /** Adds the "Replace" submenu listing the plugins the node can be replaced with. */
    void addReplaceSubmenu (PluginManager& plugins);

    /** Adds the presets submenu: save/reset defaults, factory programs, native
        presets and user presets.

        @param presets The preset collection to list user presets from
        @param name    The submenu title
    */
    void addPresetsMenu (PresetManager& presets, const juce::String& name = "Presets");

    /** Adds items to edit the DSP and UI scripts. Does nothing unless the node
        is a Script node. */
    void addScriptItems();

private:
    juce::Component::SafePointer<juce::Component> sender;
    Node node;

    void addMainItems (std::function<void()> removeAction);
    void addConnectionSubmenu (const Port& port);
    void addOversamplingSubmenu (juce::PopupMenu& menu);
    void addProgramItems (juce::PopupMenu& menu);
    void addPresetItems (PresetManager& presets, juce::PopupMenu& menu);

    /** Returns an action that posts the message built by makeMessage to the
        sender's Content, if the sender still exists when the action runs. */
    std::function<void()> postAction (std::function<juce::Message*()> makeMessage) const;
};

} // namespace element

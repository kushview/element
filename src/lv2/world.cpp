// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <lv2/event/event.h>
#include <lv2/midi/midi.h>
#include <lv2/ui/ui.h>
#include <lv2/patch/patch.h>

#include <lvtk/symbols.hpp>
#include <lvtk/options.hpp>
#include <lvtk/ext/atom.hpp>
#include <lvtk/ext/bufsize.hpp>
#include <lvtk/ext/state.hpp>

#include "lv2/lv2features.hpp"
#include "lv2/module.hpp"
#include "lv2/workerfeature.hpp"
#include "lv2/world.hpp"
#include "lv2/logfeature.hpp"

#ifndef EL_LV2_NUM_WORKERS
#define EL_LV2_NUM_WORKERS 1
#endif

namespace element {

//=============================================================================
/** A generic feature.

    This class will NOT take ownership of the feature data. The real data
    owner should not be deleted or free'd while GenericFeature is in use.
*/
class GenericFeature : public LV2Feature
{
public:
    GenericFeature (const LV2_Feature& f)
    {
        _uri = f.URI; // keep a safe copy of the URI.
        _feature.URI = _uri.toRawUTF8();
        _feature.data = f.data;
    }

    const LV2_Feature* getFeature() const override { return &_feature; }
    const juce::String& getURI() const override { return _uri; }

private:
    juce::String _uri;
    LV2_Feature _feature;
};

//=============================================================================
class OptionsFeature : public LV2Feature
{
public:
    OptionsFeature (lvtk::Symbols& symbolMap)
    {
        uri = LV2_OPTIONS__options;
        feat.URI = uri.toRawUTF8();
        feat.data = options;

        minBlockLengthOption = LV2_Options_Option { LV2_OPTIONS_INSTANCE,
                                                    0,
                                                    symbolMap.map (LV2_BUF_SIZE__minBlockLength),
                                                    sizeof (int),
                                                    symbolMap.map (LV2_ATOM__Int),
                                                    &minBlockLengthValue };
        maxBlockLengthOption = LV2_Options_Option { LV2_OPTIONS_INSTANCE,
                                                    0,
                                                    symbolMap.map (LV2_BUF_SIZE__maxBlockLength),
                                                    sizeof (int),
                                                    symbolMap.map (LV2_ATOM__Int),
                                                    &maxBlockLengthValue };
        options[0] = minBlockLengthOption;
        options[1] = maxBlockLengthOption;
        options[2] = LV2_Options_Option { LV2_OPTIONS_BLANK, 0, 0, 0, 0 };
    }

    virtual ~OptionsFeature() {}

    LV2_Options_Option minBlockLengthOption, maxBlockLengthOption;
    LV2_Options_Option options[3];
    const int minBlockLengthValue = 128;
    const int maxBlockLengthValue = 8192;
    const String& getURI() const { return uri; }
    const LV2_Feature* getFeature() const { return &feat; }

private:
    String uri;
    LV2_Feature feat;
};

//=============================================================================
class BoundedBlockLengthFeature : public LV2Feature
{
public:
    BoundedBlockLengthFeature()
    {
        uri = LV2_BUF_SIZE__boundedBlockLength;
        feat.URI = uri.toRawUTF8();
        feat.data = nullptr;
    }

    virtual ~BoundedBlockLengthFeature() {}
    const String& getURI() const { return uri; }
    const LV2_Feature* getFeature() const { return &feat; }

private:
    String uri;
    LV2_Feature feat;
};

//=============================================================================
World::World()
{
#if JUCE_MAC
    StringArray path;
    path.add (File ("/Library/Audio/Plug-Ins/LV2").getFullPathName());
    path.add (File::getSpecialLocation (File::userHomeDirectory)
                  .getChildFile ("Library/Audio/Plug-Ins/LV2")
                  .getFullPathName());
    path.trim();
    setenv ("LV2_PATH", path.joinIntoString (":").toRawUTF8(), 1);
    DBG ("LV2_PATH = " << String (getenv ("LV2_PATH")));
#endif

    world = lilv_world_new();

    lv2_InputPort = lilv_new_uri (world, LV2_CORE__InputPort);
    lv2_OutputPort = lilv_new_uri (world, LV2_CORE__OutputPort);
    lv2_AudioPort = lilv_new_uri (world, LV2_CORE__AudioPort);
    lv2_AtomPort = lilv_new_uri (world, LV2_ATOM__AtomPort);
    lv2_ControlPort = lilv_new_uri (world, LV2_CORE__ControlPort);
    lv2_EventPort = lilv_new_uri (world, LV2_EVENT__EventPort);
    lv2_CVPort = lilv_new_uri (world, LV2_CORE__CVPort);

    lv2_control = lilv_new_uri (world, LV2_CORE__control);
    lv2_enumeration = lilv_new_uri (world, LV2_CORE__enumeration);

    midi_MidiEvent = lilv_new_uri (world, LV2_MIDI__MidiEvent);
    work_schedule = lilv_new_uri (world, LV2_WORKER__schedule);
    work_interface = lilv_new_uri (world, LV2_WORKER__interface);
    options_options = lilv_new_uri (world, LV2_OPTIONS__options);

    patch_writable = lilv_new_uri (world, LV2_PATCH__writable);

    ui_CocoaUI = lilv_new_uri (world, LV2_UI__CocoaUI);
    ui_WindowsUI = lilv_new_uri (world, LV2_UI__WindowsUI);
    ui_X11UI = lilv_new_uri (world, LV2_UI__X11UI);
    ui_GtkUI = lilv_new_uri (world, LV2_UI__GtkUI);
    ui_Gtk3UI = lilv_new_uri (world, LV2_UI__Gtk3UI);
    ui_Qt4UI = lilv_new_uri (world, LV2_UI__Qt4UI);
    ui_Qt5UI = lilv_new_uri (world, LV2_UI__Qt5UI);
    ui_JUCEUI = lilv_new_uri (world, ELEMENT__JUCEUI);
    ui_UI = lilv_new_uri (world, LV2_UI__UI);
    trueNode = lilv_new_bool (world, true);
    falseNode = lilv_new_bool (world, false);

    lilv_world_set_option (world, LILV_OPTION_DYN_MANIFEST, trueNode);

    lilv_world_load_all (world);
#if JLV2_SUIL_INIT
    suil_init (nullptr, nullptr, SUIL_ARG_NONE);
#endif
    suil = suil_host_new (LV2ModuleUI::portWrite,
                          LV2ModuleUI::portIndex,
                          LV2ModuleUI::portSubscribe,
                          LV2ModuleUI::portUnsubscribe);
    suil_host_set_touch_func (suil, LV2ModuleUI::touch);

    currentThread = 0;
    numThreads = EL_LV2_NUM_WORKERS;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.add (new WorkThread ("lv2_worker_" + String (i + 1), EL_LV2_RING_BUFFER_SIZE));
    }

    addFeature (new GenericFeature (*symbolMap.map_feature()), false);
    addFeature (new GenericFeature (*symbolMap.unmap_feature()), false);
    addFeature (new LogFeature(), false);
    addFeature (new OptionsFeature (symbolMap), false);
    addFeature (new BoundedBlockLengthFeature(), true);
}

World::~World()
{
    features.clear();

#define _node_free(n) lilv_node_free (const_cast<LilvNode*> (n))
    _node_free (lv2_InputPort);
    _node_free (lv2_OutputPort);
    _node_free (lv2_AudioPort);
    _node_free (lv2_AtomPort);
    _node_free (lv2_ControlPort);
    _node_free (lv2_EventPort);
    _node_free (lv2_CVPort);
    _node_free (lv2_enumeration);
    _node_free (midi_MidiEvent);
    _node_free (work_schedule);
    _node_free (work_interface);
    _node_free (options_options);
    _node_free (ui_CocoaUI);
    _node_free (ui_WindowsUI);
    _node_free (ui_GtkUI);
    _node_free (ui_Gtk3UI);
    _node_free (ui_Qt4UI);
    _node_free (ui_Qt5UI);
    _node_free (ui_X11UI);
    _node_free (ui_JUCEUI);

    lilv_world_free (world);
    world = nullptr;
    suil_host_free (suil);
    suil = nullptr;
}

lvtk::Node World::get (const LilvNode* subject, const LilvNode* pred) const noexcept
{
    return { lilv_world_get (this->world, subject, pred, nullptr) };
}

lvtk::Node World::makeURI (const std::string& uriStr) const noexcept
{
    return { lilv_new_uri (this->world, uriStr.c_str()) };
}

LV2Module* World::createModule (const String& uri)
{
    if (const LilvPlugin* plugin = getPlugin (uri))
        return new LV2Module (*this, plugin);
    return nullptr;
}

void World::fillPluginDescription (const String& uri, PluginDescription& desc) const
{
    jassertfalse;
#if 0
    if (const LilvPlugin* plugin = getPlugin (uri))
    {
        LV2PluginModel model (*const_cast<World*> (this), plugin);
        desc.category = model.getClassLabel();
        desc.descriptiveName = String();
        desc.fileOrIdentifier = uri;
        desc.hasSharedContainer = false;
        desc.isInstrument = model.getMidiPort() != LV2UI_INVALID_PORT_INDEX;
        desc.lastFileModTime = Time();
        desc.manufacturerName = model.getAuthorName();
        desc.name = model.getName();
        desc.numInputChannels  = model.getNumPorts (PortType::Audio, true);
        desc.numOutputChannels = model.getNumPorts (PortType::Audio, false);
        desc.pluginFormatName = String ("LV2");
        desc.uid = desc.fileOrIdentifier.hashCode();
        desc.version = String();
    }
#endif
}

const LilvPlugin* World::getPlugin (const String& uri) const
{
    LilvNode* p (lilv_new_uri (world, uri.toUTF8()));
    const LilvPlugin* plugin = lilv_plugins_get_by_uri (getAllPlugins(), p);
    lilv_node_free (p);

    return plugin;
}

String World::getPluginName (const String& uri) const
{
    auto* uriNode = lilv_new_uri (world, uri.toRawUTF8());
    const auto* plugin = lilv_plugins_get_by_uri (
        lilv_world_get_all_plugins (world), uriNode);
    lilv_node_free (uriNode);
    uriNode = nullptr;

    String name;

    if (plugin != nullptr)
    {
        auto* nameNode = lilv_plugin_get_name (plugin);
        name = String::fromUTF8 (lilv_node_as_string (nameNode));
        lilv_node_free (nameNode);
    }

    return name;
}

void World::getSupportedPlugins (StringArray& list) const
{
    const LilvPlugins* plugins (lilv_world_get_all_plugins (world));
    LILV_FOREACH (plugins, iter, plugins)
    {
        const LilvPlugin* plugin = lilv_plugins_get (plugins, iter);
        const String uri = String::fromUTF8 (lilv_node_as_uri (lilv_plugin_get_uri (plugin)));
        if (isPluginSupported (uri))
            list.add (uri);
    }
}

const LilvPlugins* World::getAllPlugins() const
{
    return lilv_world_get_all_plugins (world);
}

WorkThread& World::getWorkThread()
{
    while (threads.size() < numThreads)
    {
        threads.add (new WorkThread ("LV2 Worker " + String (threads.size()), EL_LV2_RING_BUFFER_SIZE));
    }

    const int threadIndex = currentThread;
    if (++currentThread >= numThreads)
        currentThread = 0;

    return *threads.getUnchecked (threadIndex);
}

bool World::isFeatureSupported (const String& featureURI) const
{
    static const StringArray additional = {
        LV2_WORKER__schedule,
        LV2_STATE__loadDefaultState
    };

    if (features.contains (featureURI) || additional.contains (featureURI))
        return true;

    DBG ("[element] lv2 feature " + featureURI + " not supported.");
    return false;
}

bool World::isPluginAvailable (const String& uri)
{
    return (getPlugin (uri) != nullptr);
}

bool World::isPluginSupported (const String& uri) const
{
    if (const LilvPlugin* plugin = getPlugin (uri))
        return isPluginSupported (plugin);
    return false;
}

bool World::isPluginSupported (const LilvPlugin* plugin) const
{
    // Required features support
    LilvNodes* nodes = lilv_plugin_get_required_features (plugin);
    LILV_FOREACH (nodes, iter, nodes)
    {
        const LilvNode* node (lilv_nodes_get (nodes, iter));
        if (! isFeatureSupported (CharPointer_UTF8 (lilv_node_as_uri (node))))
        {
            return false; // Feature not supported
        }
    }

    lilv_nodes_free (nodes);
    nodes = nullptr;

    // Check this plugin's port types are supported
    const uint32 numPorts = lilv_plugin_get_num_ports (plugin);
    for (uint32 i = 0; i < numPorts; ++i)
    {
        // noop
        juce::ignoreUnused (i);
        // const LilvPort* port (lilv_plugin_get_port_by_index (plugin, i));
        // if (lilv_port_is_a (plugin, port, lv2_CVPort))
        //     return false;
    }

    return true;
}

} // namespace element

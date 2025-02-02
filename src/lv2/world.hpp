// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <suil/suil.h>

#include <lvtk/host/node.hpp>

#include <element/symbolmap.hpp>

#include "lv2/lv2features.hpp"

#ifndef ELEMENT_PREFIX
#define ELEMENT_PREFIX "https://lvtk.org/ns/jlv2#"
#endif

namespace element {

class LV2Module;
class WorkThread;

/** Slim wrapper around LilvWorld.  Publishes commonly used LilvNodes and
    manages heavy weight features (like LV2 Worker)
 */
class World
{
public:
    World() = delete;
    World (SymbolMap&);
    ~World();

    const LilvNode* lv2_InputPort;
    const LilvNode* lv2_OutputPort;
    const LilvNode* lv2_AudioPort;
    const LilvNode* lv2_AtomPort;
    const LilvNode* lv2_ControlPort;
    const LilvNode* lv2_EventPort;
    const LilvNode* lv2_CVPort;

    const LilvNode* lv2_control;
    const LilvNode* lv2_enumeration;

    const LilvNode* midi_MidiEvent;

    const LilvNode* work_schedule;
    const LilvNode* work_interface;

    const LilvNode* options_options;

    const LilvNode* patch_writable;

    const LilvNode* rdfs_range;

    const LilvNode* ui_CocoaUI;
    const LilvNode* ui_WindowsUI;
    const LilvNode* ui_X11UI;
    const LilvNode* ui_GtkUI;
    const LilvNode* ui_Gtk3UI;
    const LilvNode* ui_Qt4UI;
    const LilvNode* ui_Qt5UI;
    const LilvNode* ui_UI;
    const LilvNode* trueNode;
    const LilvNode* falseNode;

    lvtk::Node get (const LilvNode* subject, const LilvNode* pred) const noexcept;
    lvtk::Node makeURI (const std::string& uriStr) const noexcept;

    /** Create an LV2Module for a uri string */
    LV2Module* createModule (const String& uri);

    /** Fill a PluginDescription for a plugin uri */
    void fillPluginDescription (const String& uri, PluginDescription& desc) const;

    /** Get an LilvPlugin for a uri string */
    const LilvPlugin* getPlugin (const String& uri) const;

    /** Get all Available Plugins */
    const LilvPlugins* getAllPlugins() const;

    /** Returns true if a feature is supported */
    bool isFeatureSupported (const String& featureURI) const;

    /** Returns true if the plugin uri is availble */
    bool isPluginAvailable (const String& uri);

    /** Returns true if the plugin is supported on this system */
    bool isPluginSupported (const String& uri) const;

    /** Returns true if the plugin is supported on this system */
    bool isPluginSupported (const LilvPlugin* plugin) const;

    /** Return the underlying LilvWorld* pointer */
    inline LilvWorld* getWorld() const { return world; }

    /** Add a supported feature */
    inline void addFeature (LV2Feature* feat, bool rebuild = true) { features.add (feat, rebuild); }

    /** Get supported features */
    inline LV2FeatureArray& getFeatures() { return features; }

    /** Get supported features as a juce array.
        This can be used when instantiating plugins and uis. Don't
        forget to terminate the array with nullptr before passing
        to a plugin instance */
    inline void getFeatures (Array<const LV2_Feature*>& feats) const { features.getFeatures (feats); }

    /** Get a worker thread */
    WorkThread& getWorkThread();

    /** Returns the total number of available worker threads */
    inline int32 getNumWorkThreads() const { return numThreads; }

    /** Returns a plugin's name by URI, or empty if not found */
    String getPluginName (const String& uri) const;

    void getSupportedPlugins (StringArray&) const;

    inline SuilHost* getSuilHost() const { return suil; }

    inline const LilvNode* getNativeWidgetType() const
    {
#if JUCE_MAC
        return ui_CocoaUI;
#elif JUCE_WINDOWS
        return ui_WindowsUI;
#else
        return ui_X11UI;
#endif
    }

    /** Map a URI */
    const uint32 map (const String& uri) { return symbolMap.map (uri.toRawUTF8()); }

    /** Unmap a URID */
    String unmap (uint32 urid) { return symbolMap.unmap (urid); }

    SymbolMap& symbols() noexcept { return symbolMap; }

private:
    LilvWorld* world = nullptr;
    SuilHost* suil = nullptr;
    SymbolMap& symbolMap;
    LV2FeatureArray features;

    // a simple rotating thread pool
    int currentThread, numThreads;
    OwnedArray<WorkThread> threads;
};

} // namespace element

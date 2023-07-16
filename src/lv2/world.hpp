/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
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

#include <suil/suil.h>

#include <lvtk/symbols.hpp>
#include <lvtk/host/node.hpp>

#include "lv2/lv2features.hpp"

#ifndef ELEMENT_PREFIX
#define ELEMENT_PREFIX "https://lvtk.org/ns/jlv2#"
#define ELEMENT__JUCEUI ELEMENT_PREFIX "JUCEUI"
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
    World();
    ~World();

    const LilvNode* lv2_InputPort;
    const LilvNode* lv2_OutputPort;
    const LilvNode* lv2_AudioPort;
    const LilvNode* lv2_AtomPort;
    const LilvNode* lv2_ControlPort;
    const LilvNode* lv2_EventPort;
    const LilvNode* lv2_CVPort;
    const LilvNode* lv2_enumeration;
    const LilvNode* midi_MidiEvent;
    const LilvNode* work_schedule;
    const LilvNode* work_interface;
    const LilvNode* options_options;
    const LilvNode* ui_CocoaUI;
    const LilvNode* ui_WindowsUI;
    const LilvNode* ui_X11UI;
    const LilvNode* ui_GtkUI;
    const LilvNode* ui_Gtk3UI;
    const LilvNode* ui_Qt4UI;
    const LilvNode* ui_Qt5UI;
    const LilvNode* ui_JUCEUI;
    const LilvNode* ui_UI;
    const LilvNode* trueNode;
    const LilvNode* falseNode;

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

private:
    LilvWorld* world = nullptr;
    SuilHost* suil = nullptr;
    lvtk::Symbols symbolMap;
    LV2FeatureArray features;

    // a simple rotating thread pool
    int currentThread, numThreads;
    OwnedArray<WorkThread> threads;
};

} // namespace element

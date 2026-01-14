// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"
#include <element/audioengine.hpp>
#include <element/processor.hpp>
#include <element/session.hpp>

namespace element {

class Content;
class Context;
class Node;
class NavigationConcertinaPanel;
class GuiService;

namespace ViewHelpers {

/** Draws a common text row in a normal list box */
void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding = 10, Justification alignment = Justification::centredLeft);

/** Draws a common text row in a horizontal list box */
void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected);

/** Finds the content component by traversing parent component(s) */
Content* findContentComponent (Component* c);

/** Finds the content component by traversing toplevel windows
    This will NOT work in the plugin versions
*/
Content* findContentComponent();

/** Get the engine */
AudioEnginePtr getAudioEngine (Component*);

/** Get the GUI controller */
GuiService* getGuiController (Component* c);

/** Get World */
Context* getGlobals (Component* c);

/** Get Session */
SessionPtr getSession (Component* c);

/** Invoke a command directly */
bool invokeDirectly (Component* c, const int commandID, bool async);

/** Post a message to Services
 
    This works by finding the Content and letting it handle message posting.
    If the content component wasn't found, then the passed in Message will be deleted
    immediately.  DO NOT keep a reference to messages passed in here 
 */
void postMessageFor (Component*, Message*);

/** Get a graph node for a given node.  If the node doesn't have an
    object property, then controllers are used to find the object
 */
ProcessorPtr findGraphNodeFor (Component*, const Node&);

/** This will present a plugin window */
void presentPluginWindow (Component*, const Node&);

void closePluginWindows (Component*, const bool visible = true);

void closePluginWindowsFor (Component*, Node& node, const bool visible = true);

} // namespace ViewHelpers

class ViewHelperMixin
{
public:
    virtual ~ViewHelperMixin() {}

    inline Content* content() const { return ViewHelpers::findContentComponent (componentCast()); }
    inline SessionPtr session() const { return ViewHelpers::getSession (componentCast()); }
    inline void postMessage (Message* message) { return ViewHelpers::postMessageFor (componentCast(), message); }

    void connectPorts (const Port& src, const Port& dst);
    void connectPorts (const Node& graph, const uint32 srcNode, const uint32 srcPort, const uint32 dstNode, const uint32 dstPort);

    void disconnectPorts (const Port& src, const Port& dst);

protected:
    explicit ViewHelperMixin (void* p) : superClass (*(Component*) p) {}

private:
    Component& superClass;
    Component* componentCast() const
    {
        return &superClass;
    }

    JUCE_DECLARE_NON_COPYABLE (ViewHelperMixin);
};

} // namespace element

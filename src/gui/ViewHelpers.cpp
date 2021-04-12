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

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphManager.h"
#include "engine/AudioEngine.h"
#include "gui/ContentComponent.h"
#include "gui/LookAndFeel.h"
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"
#include "session/CommandManager.h"
#include "Globals.h"
#include "Messages.h"

#include "plugins/PluginEditor.h"

namespace Element {
namespace ViewHelpers {

typedef Element::LookAndFeel LF;

void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, 
                       int padding, Justification alignment)
{
    g.saveState();
    
    if (selected)
    {
        g.setColour (Colors::elemental.darker (0.6000006f));
        g.setOpacity (1.0f);
        g.fillRect (0, 0, w, h);
    }

    g.setColour ((selected) ? LF::textColor.brighter(0.2f) : LF::textColor);
    if (text.isNotEmpty())
        g.drawText (text, padding, 0, w - padding - 2, h, alignment);

    g.restoreState();
}

void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected)
{
    g.saveState();
    g.addTransform (AffineTransform().rotated (1.57079633f, (float)w, 0.0f));
    
    if (selected)
    {
        g.setColour(LF::textColor.darker (0.6000006));
        g.setOpacity (0.60);
        g.fillRect (0, 0, h, w);
    }
    
   #if JUCE_MAC
    // g.setFont (Resources::normalFontSize);
   #endif
    
    g.setColour((selected) ? LF::textColor.contrasting() : LF::textColor);
    g.drawText (text, 40, 0, h - 40, w, Justification::centredLeft);
    
    g.restoreState();
}

/** Finds the content component by traversing toplevel windows */
ContentComponent* findContentComponent()
{
    for (int i = 0; i < DocumentWindow::getNumTopLevelWindows(); ++i)
        if (auto* main = dynamic_cast<MainWindow*> (DocumentWindow::getTopLevelWindow (i)))
            return dynamic_cast<ContentComponent*> (main->getContentComponent());
    return nullptr;
}

ContentComponent* findContentComponent (Component* c)
{
    if (auto* cc = c->findParentComponentOfClass<ContentComponent>())
        return cc;
    
    if (auto* pw = c->findParentComponentOfClass<PluginWindow>())
        return pw->getElementContentComponent();

    if (auto* ed = c->findParentComponentOfClass<PluginEditor>())
        return ed->getContentComponent();
    
    if (auto* const contentComponent = findContentComponent())
        return contentComponent;
    
    return nullptr;
}

GuiController* getGuiController (Component* c)
{
    if (auto* const cc = findContentComponent (c))
        return cc->getAppController().findChild<GuiController>();
    return nullptr;
}

AudioEnginePtr getAudioEngine (Component* c)
{
    if (auto* cc = findContentComponent (c))
        return cc->getGlobals().getAudioEngine();
    return nullptr;
}

Globals* getGlobals (Component* c)
{
    if (auto* cc = findContentComponent(c))
        return &cc->getGlobals();
    return nullptr;
}

SessionPtr getSession (Component* c)
{
    if (auto* cc = findContentComponent(c))
        return cc->getSession();
    return nullptr;
}

bool invokeDirectly (Component* c, const int commandID, bool async) {
    if (auto* g = getGlobals (c))
        return g->getCommandManager().invokeDirectly(commandID, async);
    return false;
}

NodeObjectPtr findGraphNodeFor (Component* c, const Node& node)
{
    NodeObjectPtr obj = node.getObject();
    
    if (nullptr == obj)
    {
         // noop
    }
    
    return obj;
}

void postMessageFor (Component* c, Message* m)
{
    ScopedPointer<Message> deleter (m);
    if (auto* const cc = findContentComponent (c))
        return cc->post (deleter.release());
    jassertfalse; // message not delivered
    deleter = nullptr;
}

void presentPluginWindow (Component* c, const Node& node)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->getAppController().findChild<GuiController>())
            gui->presentPluginWindow (node);
}

void closePluginWindows (Component* c, const bool visible)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->getAppController().findChild<GuiController>())
            gui->closeAllPluginWindows (visible);
}

void closePluginWindowsFor (Component* c, Node& node, const bool visible)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->getAppController().findChild<GuiController>())
            gui->closePluginWindowsFor (node, visible);
}

void presentPluginWindow (const Node& node)
{
    jassertfalse;
}

NavigationConcertinaPanel* getNavigationConcertinaPanel (Component* c)
{
    if (auto* cc = findContentComponent (c))
        return cc->getNavigationConcertinaPanel();
    return nullptr;
}
}

void ViewHelperMixin::connectPorts (const Port& src, const Port& dst)
{
    const Node srcNode (src.getNode());
    const Node dstNode (dst.getNode());
    const Node graph   (srcNode.getParentGraph());

    DBG("[EL] sending connect message: " << srcNode.getName() << " <-> " << dstNode.getName());
    postMessage (new AddConnectionMessage (srcNode.getNodeId(), src.getIndex(), 
                                           dstNode.getNodeId(), dst.getIndex(), graph));
}

void ViewHelperMixin::connectPorts (const Node& graph, const uint32 srcNode, const uint32 srcPort, 
                                                       const uint32 dstNode, const uint32 dstPort)
{
    ViewHelpers::postMessageFor (componentCast(),
        new AddConnectionMessage (srcNode, srcPort, dstNode, dstPort, graph));
}

void ViewHelperMixin::disconnectPorts (const Port& src, const Port& dst)
{
    const Node srcNode (src.getNode());
    const Node dstNode (dst.getNode());
    const Node graph   (srcNode.getParentGraph());
    postMessage (new RemoveConnectionMessage (srcNode.getNodeId(), src.getIndex(),
                                              dstNode.getNodeId(), dst.getIndex(), graph));
}

}

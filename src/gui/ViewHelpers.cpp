// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/services.hpp>
#include <element/ui.hpp>
#include "engine/graphmanager.hpp"
#include <element/audioengine.hpp>
#include <element/ui/content.hpp>
#include <element/ui/style.hpp>
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "gui/ViewHelpers.h"
#include <element/node.hpp>
#include <element/ui/commands.hpp>
#include <element/context.hpp>
#include "messages.hpp"

#include "plugineditor.hpp"

namespace element {
namespace ViewHelpers {

void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding, Justification alignment)
{
    g.saveState();

    if (selected)
    {
        g.setColour (Colors::elemental.darker (0.6000006f));
        g.setOpacity (1.0f);
        g.fillRect (0, 0, w, h);
    }

    g.setColour ((selected) ? Colors::textColor.brighter (0.2f) : Colors::textColor);
    if (text.isNotEmpty())
        g.drawText (text, padding, 0, w - padding - 2, h, alignment);

    g.restoreState();
}

void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected)
{
    g.saveState();
    g.addTransform (AffineTransform().rotated (1.57079633f, (float) w, 0.0f));

    if (selected)
    {
        g.setColour (Colors::textColor.darker (0.6000006f));
        g.setOpacity (0.60f);
        g.fillRect (0, 0, h, w);
    }

#if JUCE_MAC
    // g.setFont (Resources::normalFontSize);
#endif

    g.setColour ((selected) ? Colors::textColor.contrasting() : Colors::textColor);
    g.drawText (text, 40, 0, h - 40, w, Justification::centredLeft);

    g.restoreState();
}

/** Finds the content component by traversing toplevel windows */
Content* findContentComponent()
{
    for (int i = 0; i < DocumentWindow::getNumTopLevelWindows(); ++i)
        if (auto* main = dynamic_cast<MainWindow*> (DocumentWindow::getTopLevelWindow (i)))
            return dynamic_cast<Content*> (main->getContentComponent());
    return nullptr;
}

Content* findContentComponent (Component* c)
{
    if (auto* cc = c->findParentComponentOfClass<Content>())
        return cc;

    if (auto* pw = c->findParentComponentOfClass<PluginWindow>())
        return pw->getElementContentComponent();

    if (auto* pe = c->findParentComponentOfClass<PluginEditor>())
        return pe->getContentComponent();

    if (auto* const contentComponent = findContentComponent())
        return contentComponent;

    return nullptr;
}

GuiService* getGuiController (Component* c)
{
    if (auto* const cc = findContentComponent (c))
        return cc->services().find<GuiService>();
    return nullptr;
}

AudioEnginePtr getAudioEngine (Component* c)
{
    if (auto* cc = findContentComponent (c))
        return cc->context().audio();
    return nullptr;
}

Context* getGlobals (Component* c)
{
    if (auto* cc = findContentComponent (c))
        return &cc->context();
    return nullptr;
}

SessionPtr getSession (Component* c)
{
    if (auto* cc = findContentComponent (c))
        return cc->session();
    return nullptr;
}

bool invokeDirectly (Component* c, const int commandID, bool async)
{
    if (auto* g = getGuiController (c))
        return g->commands().invokeDirectly (commandID, async);
    return false;
}

ProcessorPtr findGraphNodeFor (Component* c, const Node& node)
{
    ProcessorPtr obj = node.getObject();

    if (nullptr == obj)
    {
        // noop
    }

    return obj;
}

void postMessageFor (Component* c, Message* m)
{
    std::unique_ptr<Message> deleter (m);
    if (auto* const cc = findContentComponent (c))
        return cc->post (deleter.release());
    jassertfalse; // message not delivered
    deleter = nullptr;
}

void presentPluginWindow (Component* c, const Node& node)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->services().find<GuiService>())
            gui->presentPluginWindow (node);
}

void closePluginWindows (Component* c, const bool visible)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->services().find<GuiService>())
            gui->closeAllPluginWindows (visible);
}

void closePluginWindowsFor (Component* c, Node& node, const bool visible)
{
    if (auto* cc = findContentComponent (c))
        if (auto* gui = cc->services().find<GuiService>())
            gui->closePluginWindowsFor (node, visible);
}

void presentPluginWindow (const Node& node)
{
    jassertfalse;
}

} // namespace ViewHelpers

void ViewHelperMixin::connectPorts (const Port& src, const Port& dst)
{
    const Node srcNode (src.getNode());
    const Node dstNode (dst.getNode());
    const Node graph (srcNode.getParentGraph());

    DBG ("[element] sending connect message: " << srcNode.getName() << " <-> " << dstNode.getName());
    postMessage (new AddConnectionMessage (srcNode.getNodeId(), src.index(), dstNode.getNodeId(), dst.index(), graph));
}

void ViewHelperMixin::connectPorts (const Node& graph, const uint32 srcNode, const uint32 srcPort, const uint32 dstNode, const uint32 dstPort)
{
    ViewHelpers::postMessageFor (componentCast(),
                                 new AddConnectionMessage (srcNode, srcPort, dstNode, dstPort, graph));
}

void ViewHelperMixin::disconnectPorts (const Port& src, const Port& dst)
{
    const Node srcNode (src.getNode());
    const Node dstNode (dst.getNode());
    const Node graph (srcNode.getParentGraph());
    postMessage (new RemoveConnectionMessage (srcNode.getNodeId(), src.index(), dstNode.getNodeId(), dst.index(), graph));
}

} // namespace element

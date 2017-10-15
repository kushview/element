
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "gui/ViewHelpers.h"
#include "gui/ContentComponent.h"
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "session/Node.h"
#include "Globals.h"

namespace Element {
namespace ViewHelpers {

typedef LookAndFeel_KV1 LF;

void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding)
{
    g.saveState();
    
    if (selected)
    {
        g.setColour (LF::elementBlue.darker (0.6000006f));
        g.setOpacity (0.60f);
        g.fillRect (0, 0, w, h);
    }

    g.setColour ((selected) ? LF::textColor.brighter(0.2f) : LF::textColor);
    g.drawText (text, padding, 0, w - padding, h, Justification::centredLeft);

    g.restoreState();
}

void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected)
{
    g.saveState();
    g.addTransform (AffineTransform::identity.rotated (1.57079633f, (float)w, 0.0f));
    
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

ContentComponent* findContentComponent (Component* c)
{
    if (auto* cc = c->findParentComponentOfClass<ContentComponent>())
        return cc;
    return nullptr;
}

GraphNodePtr findGraphNodeFor (Component* c, const Node& node)
{
    auto* cc = findContentComponent (c);
    if (! cc) return nullptr;
    auto& graph (cc->getGlobals().getAudioEngine()->getRootGraph());
    return graph.getNodeForId (node.getNodeId());
}

void postMessageFor (Component* c, Message* m)
{
    ScopedPointer<Message> deleter (m);
    if (auto* const cc = findContentComponent (c))
        return cc->post (deleter.release());
    jassertfalse; // message not delivered
    deleter = nullptr;
}

void presentPluginWindow (GraphNodePtr ptr)
{
    auto* window = PluginWindow::getWindowFor (ptr);
    if (window)
    {
        window->setVisible (true);
        window->toFront (false);
        return;
    }
    
    window = PluginWindow::createWindowFor (ptr);
    if (window)
    {
        window->setVisible (true);
    }
}

}
}


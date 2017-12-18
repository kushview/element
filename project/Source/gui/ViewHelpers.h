
#ifndef EL_VIEW_HELPERS_H
#define EL_VIEW_HELPERS_H

#include "ElementApp.h"
#include "engine/GraphNode.h"
#include "session/Session.h"

namespace Element {

class ContentComponent;
class Globals;
class Node;
class NavigationConcertinaPanel;

namespace ViewHelpers {

/** Draws a common text row in a normal list box */
void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding = 10);

/** Draws a common text row in a horizontal list box */
void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected);

/** Finds the content component by traversing parent component(s) */
ContentComponent* findContentComponent (Component* c);

/** Get World */
Globals* getGlobals (Component* c);

/** Get Session */
SessionPtr getSession (Component* c);

NavigationConcertinaPanel* getNavigationConcertinaPanel (Component* c);

/** Invoke a command directly */
bool invokeDirectly (Component* c, const int commandID, bool async);
    
/** Post a message to AppController
 
    This works by finding the ContentComponent and letting it handle message posting.
    If the content component wasn't found, then the passed in Message will be deleted
    immediately.  DO NOT keep a reference to messages passed in here 
 */
void postMessageFor (Component*, Message*);

/** Get a graph node for a given node.  If the node doesn't have an
    object property, then controllers are used to find the object
 */
GraphNodePtr findGraphNodeFor (Component*, const Node&);

/** This will present a plugin window */
void presentPluginWindow (Component*, const Node&);

void closePluginWindows (Component*, const bool visible = true);

void closePluginWindowsFor (Component*, Node& node, const bool visible = true);

}


class ViewHelperMixin
{
public:
    virtual ~ViewHelperMixin() { }

    inline ContentComponent* getContentComponent() const   { return ViewHelpers::findContentComponent (componentCast()); }
    inline SessionPtr getSession() const                   { return ViewHelpers::getSession (componentCast()); }
    inline void postMessage (Message* message)             { return ViewHelpers::postMessageFor (componentCast(), message); }
    
    void connectPorts (const Port& src, const Port& dst);
    void connectPorts (const Node& graph, const uint32 srcNode, const uint32 srcPort, 
                                          const uint32 dstNode, const uint32 dstPort);
    
    void disconnectPorts (const Port& src, const Port& dst);

protected:
    ViewHelperMixin() { }

private:
    Component* componentCast() const {
        return reinterpret_cast<Component*> (const_cast<ViewHelperMixin*> (this));
    }
};



}

#endif  // EL_VIEW_HELPERS_H

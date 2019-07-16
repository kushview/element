#pragma once

#include "JuceHeader.h"
#include "engine/GraphNode.h"
#include "session/Node.h"

namespace Element {

class NodeEditorComponent : public Component
{
protected:
    NodeEditorComponent (const Node&) noexcept;

public:
    virtual ~NodeEditorComponent() override;
    inline Node getNode() const { return node; }
    bool isRunningInPluginWindow() const;

protected:
    inline GraphNode* getNodeObject() const { return node.getGraphNode(); }
    template<class T> inline T* getNodeObjectOfType() const { return dynamic_cast<T*> (getNodeObject()); }

private:
    Node node;
};

}

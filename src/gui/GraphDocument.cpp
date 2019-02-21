#include "gui/GraphDocument.h"

namespace Element {

GraphDocument::GraphDocument()
    : FileBasedDocument (".elg", "*.elg", "Open Graph", "Save Graph")
{ }

GraphDocument::~GraphDocument()
{
    graph = { };
}

String GraphDocument::getDocumentTitle()
{
    return graph.isValid() ? graph.getName() : "Unknown";
}

Result GraphDocument::loadDocument (const File& file)
{
    const auto data = Node::parse (file);
    if (! Node::isProbablyGraphNode (data))
        return Result::fail ("Invalid graph provided");
    setGraph (Node (data, true));
    return graph.isGraph() ? Result::ok() : Result::fail ("Malformed graph");
}

Result GraphDocument::saveDocument (const File& file)
{
    if (! graph.isGraph())
    {
        DBG(graph.getValueTree().toXmlString());
        return Result::fail ("No graph is loaded");
    }

    graph.savePluginState();
    
    if (graph.writeToFile (file))
        return Result::ok();

    return Result::fail ("Could not write graph to file");
}

File GraphDocument::getLastDocumentOpened()
{
    return lastGraph;
}

void GraphDocument::setLastDocumentOpened (const File& file)
{
    lastGraph = file;
}

}

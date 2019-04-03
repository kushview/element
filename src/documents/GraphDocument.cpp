#include "documents/GraphDocument.h"

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
    jassert(session != nullptr);
    if (! session)
        return Result::fail ("Cannot load graph");
    
    auto newData = Session::readFromFile (file);
    if (newData.isValid() && newData.hasType (Tags::session))
    {
        if (! session->loadData (newData))
            return Result::fail ("Cannot load malformed graph");

        session->forEach ([](const ValueTree& tree) {
            if (! tree.hasType (Tags::node))
                return;
            Node addMising (tree, true);
            ignoreUnused (addMising);
        });
        
        bindChangeHandlers();
        return graph.isGraph() ? Result::ok() : Result::fail ("Malformed graph");
    }

    newData = Node::parse (file);
    if (! Node::isProbablyGraphNode (newData))
        return Result::fail ("Invalid graph provided");
    setGraph (Node (newData, true));
    return graph.isGraph() ? Result::ok() : Result::fail ("Malformed graph");
}

Result GraphDocument::saveDocument (const File& file)
{
    jassert(session && session->containsGraph (graph));
    if (!session || !session->containsGraph (graph))
        return Result::fail ("No graph data present");

    if (! graph.isGraph())
    {
        return Result::fail ("No graph is loaded");
    }

    session->saveGraphState();
    
    if (session->writeToFile (file))
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

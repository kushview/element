
#include "controllers/GraphController.h"
#include "session/PluginManager.h"

namespace Element {

const int GraphController::midiChannelNumber = 0x1000;

GraphController::GraphController (GraphProcessor& pg, PluginManager& pm)
    : pluginManager (pm), processor (pg), lastUID (0)
{ }

GraphController::~GraphController() { }

uint32 GraphController::getNextUID() noexcept
{
    return ++lastUID;
}

int GraphController::getNumFilters() const noexcept
{
    return processor.getNumNodes();
}

const GraphNodePtr GraphController::getNode (const int index) const noexcept
{
    return processor.getNode (index);
}

const GraphNodePtr GraphController::getNodeForId (const uint32 uid) const noexcept
{
    return processor.getNodeForId (uid);
}

uint32 GraphController::addFilter (const PluginDescription* desc, double x, double y, uint32 nodeId)
{
    if (desc != nullptr)
    {
        String errorMessage;
        auto* instance = pluginManager.createAudioPlugin (*desc, errorMessage);

        GraphNode* node = nullptr;

        if (instance != nullptr)
            node = processor.addNode (instance, nodeId);

        if (node != nullptr)
        {
            node->properties.set ("x", x);
            node->properties.set ("y", y);
            nodeId = node->nodeId;
            changed();
        }
        else
        {
            nodeId = GraphController::invalidNodeId;
            AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                         TRANS ("Couldn't create filter"),
                                         errorMessage);
        }
    }

    return nodeId;
}

void GraphController::removeFilter (const uint32 uid)
{
    if (processor.removeNode (uid))
        changed();
}

void GraphController::disconnectFilter (const uint32 id)
{
    if (processor.disconnectNode (id))
        changed();
}

void GraphController::removeIllegalConnections()
{
    if (processor.removeIllegalConnections())
        changed();
}

int GraphController::getNumConnections() const noexcept
{
    return processor.getNumConnections();
}

const GraphProcessor::Connection* GraphController::getConnection (const int index) const noexcept
{
    return processor.getConnection (index);
}

const GraphProcessor::Connection* GraphController::getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel,
                                                                          uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.getConnectionBetween (sourceFilterUID, sourceFilterChannel,
                                           destFilterUID, destFilterChannel);
}

bool GraphController::canConnect (uint32 sourceFilterUID, int sourceFilterChannel,
                                  uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.canConnect (sourceFilterUID, sourceFilterChannel,
                                 destFilterUID, destFilterChannel);
}

bool GraphController::addConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                                     uint32 destFilterUID, int destFilterChannel)
{
    const bool result = processor.addConnection (sourceFilterUID, (uint32)sourceFilterChannel,
                                             destFilterUID, (uint32)destFilterChannel);

    if (result)
        changed();

    return result;
}

void GraphController::removeConnection (const int index)
{
    processor.removeConnection (index);
    changed();
}

void GraphController::removeConnection (uint32 sourceNode, uint32 sourcePort,
                                        uint32 destNode, uint32 destPort)
{
    if (processor.removeConnection (sourceNode, sourcePort, destNode, destPort))
        changed();
}

void GraphController::clear()
{
    processor.clear();
    changed();
}

}

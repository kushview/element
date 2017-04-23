/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "controllers/GraphController.h"
#include "session/PluginManager.h"

namespace Element {

const int GraphController::midiChannelNumber = 0x1000;

GraphController::GraphController (GraphProcessor& pg, PluginManager& pm)
    : pluginManager (pm), processor (pg), lastUID (0)
{ }

GraphController::~GraphController()
{
    //processor.clear();
}

uint32 GraphController::getNextUID() noexcept
{
    return ++lastUID;
}

//==============================================================================
int GraphController::getNumFilters() const noexcept
{
    return processor.getNumNodes();
}

const GraphNodePtr GraphController::getNode (const int index) const noexcept
{
    return processor.getNode (index);
}

const GraphNodePtr
GraphController::getNodeForId (const uint32 uid) const noexcept
{
    return processor.getNodeForId (uid);
}

uint32 GraphController::addFilter (const PluginDescription* desc, double x, double y, uint32 nodeId)
{
    if (desc != nullptr)
    {
#if 1
        String errorMessage;
        Processor* instance = pluginManager.createPlugin (*desc, errorMessage);

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
#endif
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

void GraphController::setNodePosition (const int nodeId, double x, double y)
{
    const GraphNodePtr n (processor.getNodeForId (nodeId));

    if (n != nullptr)
    {
        n->properties.set ("x", jlimit (0.0, 1.0, x));
        n->properties.set ("y", jlimit (0.0, 1.0, y));
    }
}

void GraphController::getNodePosition (const int nodeId, double& x, double& y) const
{
    x = y = 0;

    const GraphNodePtr n (processor.getNodeForId (nodeId));

    if (n != nullptr)
    {
        x = (double) n->properties ["x"];
        y = (double) n->properties ["y"];
    }
}

//==============================================================================
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

void GraphController::removeConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                                    uint32 destFilterUID, int destFilterChannel)
{
    if (processor.removeConnection (sourceFilterUID, sourceFilterChannel,
                                    destFilterUID, destFilterChannel))
        changed();
}

void GraphController::clear()
{
    processor.clear();
    changed();
}
}

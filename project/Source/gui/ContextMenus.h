
#pragma once

#include "gui/GuiCommon.h"
#include "session/PluginManager.h"
namespace Element {

class PluginsPopupMenu : public PopupMenu
{
public:
    PluginsPopupMenu (Component* sender)
    {
        jassert (sender);
        auto* cc = ViewHelpers::findContentComponent (sender);
        jassert (cc);
        plugins = &cc->getGlobals().getPluginManager();
    }
    
    bool isPluginResultCode (const int resultCode) {
        return plugins->availablePlugins().getIndexChosenByMenu (resultCode) >= 0;
    }
    
    const PluginDescription* getPluginDescription (int resultCode)
    {
        jassert (plugins);
        const int index = plugins->availablePlugins().getIndexChosenByMenu (resultCode);
        return index >= 0 ? plugins->availablePlugins().getType (index) : nullptr;
    }
    
    void addPluginItems()
    {
        if (hasAddedPlugins)
            return;
        hasAddedPlugins = true;
        plugins->availablePlugins().addToMenu (*this, KnownPluginList::sortByManufacturer);
    }
    
private:
    Component* sender;
    PluginManager* plugins;
    bool hasAddedPlugins = false;
};


// MARK: Node Popup Menu

class NodePopupMenu : public PopupMenu
{
public:
    enum ItemIds
    {
        Duplicate = 1,
        RemoveNode,
        Disconnect,
        LastItem
    };
    
    typedef std::initializer_list<ItemIds> ItemList;
    
    explicit NodePopupMenu() { }
    
    NodePopupMenu (const Node& n)     // Display sibling outputs
    : node (n)
    {
        addMainItems (true);
    }
    
    NodePopupMenu (const Node& n, const Port& p)
        : node (n), port(p)
    {
        addMainItems (true);
        NodeArray siblings;
        addSeparator();
        
        if (port.isInput())
        {
            PopupMenu items;
            node.getPossibleSources (siblings);
            for (auto& src : siblings)
            {
                PopupMenu srcMenu;
                PortArray ports;
                src.getPorts (ports, PortType::Audio, false);
                if (ports.isEmpty())
                    continue;
                for (const auto& p : ports)
                    addItemInternal (srcMenu, p.getName(), new SingleConnectOp (src, p, node, port));
                items.addSubMenu (src.getName(), srcMenu);
            }
            
            addSubMenu ("Sources", items);
        }
        else
        {
            PopupMenu items;
            node.getPossibleDestinations (siblings);
            for (auto& dst : siblings)
            {
                PopupMenu srcMenu;
                PortArray ports;
                dst.getPorts (ports, PortType::Audio, true);
                if (ports.isEmpty())
                    continue;
                for (const auto& p : ports)
                    addItemInternal (srcMenu, p.getName(), new SingleConnectOp (node, port, dst, p));
                items.addSubMenu (dst.getName(), srcMenu);
            }
            
            addSubMenu ("Destinations", items);
        }
    }
    
    void getProgramsMenu (PopupMenu& menu)
    {
        const int offset = 10000;
        const int current = node.getCurrentProgram();
        for (int i = 0; i < node.getNumPrograms(); ++i) {
            menu.addItem (offset + i, node.getProgramName (i), true, i == current);
        }
    }
    
    ~NodePopupMenu()
    {
        resultMap.clear();
        deleter.clearQuick (true);
    }
    
    
    Message* createMessageForResultCode (const int result)
    {
        if (result == RemoveNode)
            return new RemoveNodeMessage (node);
        if (result == Duplicate)
            return new DuplicateNodeMessage (node);
        if (result == Disconnect)
            return new DisconnectNodeMessage (node);
        if (auto* op = resultMap [result])
            return op->createMessage();
        if (result >= 10000)
        {
            Node(node).setCurrentProgram (result - 10000);
        }
        
        return nullptr;
    }
    
    Message* showAndCreateMessage() {
        return createMessageForResultCode (this->show());
    }
    
private:
    const Node node;
    const Port port;
    const int firstResultOpId = 1024;
    int currentResultOpId = 1024;
    
    struct ResultOp
    {
        ResultOp() { }
        virtual ~ResultOp () { }
        virtual bool isActive() { return true; }
        virtual bool isTicked() { return false; }
        virtual Message* createMessage() =0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResultOp);
    };
    
    struct SingleConnectOp : public ResultOp
    {
        SingleConnectOp (const Node& sn, const Port& sp, const Node& dn, const Port& dp)
            : sourceNode(sn), destNode(dn),  sourcePort (sp), destPort (dp)
        { }
        
        const Node sourceNode, destNode;
        const Port sourcePort, destPort;
        
        bool isTicked()
        {
            return Node::connectionExists (sourceNode.getParentArcsNode(),
                                           sourceNode.getNodeId(), sourcePort.getIndex(),
                                           destNode.getNodeId(), destPort.getIndex());
        }
        
        Message* createMessage()
        {
            return new AddConnectionMessage (sourceNode.getNodeId(), sourcePort.getIndex(),
                                             destNode.getNodeId(), destPort.getIndex());
        }
    };
    
    HashMap<int, ResultOp*> resultMap;
    OwnedArray<ResultOp> deleter;
    
    void addMainItems (const bool showHeader)
    {
        if (showHeader)
            addSectionHeader (node.getName());
        addItem (Disconnect, getNameForItem (Disconnect));
        addItem (Duplicate,  getNameForItem (Duplicate));
        addSeparator();
        addItem (RemoveNode, getNameForItem (RemoveNode));
    }
    
    void addItemInternal (PopupMenu& menu, const String& name, ResultOp* op)
    {
        menu.addItem (currentResultOpId, name, op->isActive(), op->isTicked());
        resultMap.set (currentResultOpId, deleter.add (op));
        ++currentResultOpId;
    }
    
    String getNameForItem (ItemIds item)
    {
        switch (item)
        {
            case Disconnect: return "Disconnect"; break;
            case Duplicate:  return "Duplicate"; break;
            case RemoveNode: return "Remove"; break;
            default: jassertfalse; break;
        }
        return "Unknown Item";
    }
};

}

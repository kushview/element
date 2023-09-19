// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/plugins.hpp>
#include <element/ui/style.hpp>
#include <element/engine.hpp>

#include "ui/guicommon.hpp"
#include "ui/breadcrumb.hpp"
#include "ui/contextmenus.hpp"
#include "ui/horizontallistbox.hpp"
#include "ui/connectiongrid.hpp"
#include "session/presetmanager.hpp"

#include "matrixstate.hpp"

#define EL_MATRIX_SIZE 30

namespace element {
// Spacing between each patch point
static const int gridPadding = 1;

// =============================================================================
class ConnectionGrid::PatchMatrix : public PatchMatrixComponent,
                                    private ValueTree::Listener,
                                    ViewHelperMixin
{
public:
    PatchMatrix()
        : ViewHelperMixin (this),
          useHighlighting (true),
          matrix()
    {
        setSize (300, 200);
        setMatrixCellSize (EL_MATRIX_SIZE);
        nodeModels = ValueTree (tags::nodes);
        nodes.clear();
        nodeModels.addListener (this);
        graphModel.addListener (this);
    }

    ~PatchMatrix()
    {
        nodeModels.removeListener (this);
        graphModel.removeListener (this);
    }

    const Node getNode (const int index, const bool isSource) const
    {
        return nodes[isSource ? audioOutIndexes[index]
                              : audioInIndexes[index]];
    }

    const Port getPort (const int index, const bool isSource) const
    {
        const PortArray& ports = isSource ? outs : ins;
        return ports[index];
    }

    const int getAudioChannelForIndex (const int index, const bool isSource) const
    {
        return isSource ? audioOutChannels.getUnchecked (index)
                        : audioInChannels.getUnchecked (index);
    }

    void updateContent();

    void setUseHighlighting (const bool shouldUseHighlighting)
    {
        if (shouldUseHighlighting == useHighlighting)
            return;
        useHighlighting = shouldUseHighlighting;
        repaint();
    }

    void mouseExit (const juce::MouseEvent& ev) override
    {
        PatchMatrixComponent::mouseExit (ev);
        if (auto* grid = findParentComponentOfClass<ConnectionGrid>())
            grid->repaint();
    }

    void mouseMove (const MouseEvent& ev) override
    {
        PatchMatrixComponent::mouseMove (ev);

        if (useHighlighting)
            repaint();
    }

    void paintMatrixCell (Graphics& g, const int width, const int height, const int row, const int column) override
    {
        const Node src (getNode (row, true));
        const Node dst (getNode (column, false));

        if (useHighlighting && (mouseIsOverCell (row, column) && ! matrix.connected (row, column)))
        {
            g.setColour (Colors::elemental.withAlpha (0.4f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else if ((mouseIsOverRow (row) || mouseIsOverColumn (column)) && ! matrix.connected (row, column))
        {
            g.setColour (Colors::elemental.withAlpha (0.3f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else
        {
            g.setColour (matrix.connected (row, column)
                             ? Colour (Colors::elemental.brighter())
                             : Colour (LookAndFeel_E1::defaultMatrixCellOffColor));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
    }

    void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
    {
        const Node graph (graphModel, false);
        const Node srcNode (getNode (row, true));
        const Port srcPort (getPort (row, true));
        const Node dstNode (getNode (col, false));
        const Port dstPort (getPort (col, false));

        if (ev.mods.isPopupMenu())
        {
            // noop
            return;
        }

        if (! srcNode.canConnectTo (dstNode))
        {
            matrix.disconnect (row, col);
            repaint();
            return;
        }

        if (! PortType::canConnect (srcPort.getType(), dstPort.getType()))
            return;

        const ValueTree arcs (srcNode.getParentArcsNode());
        if (Node::connectionExists (arcs,
                                    srcNode.getNodeId(),
                                    srcPort.index(),
                                    dstNode.getNodeId(),
                                    dstPort.index()))
        {
            matrix.disconnect (row, col);
            disconnectPorts (srcPort, dstPort);
        }
        else
        {
            matrix.connect (row, col);
            jassert (nullptr != ViewHelpers::findContentComponent (this));

            connectPorts (srcPort, dstPort);
        }

        repaint();
    }

    void matrixBackgroundClicked (const MouseEvent& ev) override
    {
        emptyAreaClicked (ev);
    }

    int getNumRows() override { return matrix.getNumRows(); }
    int getNumColumns() override { return matrix.getNumColumns(); }

    void paint (Graphics& g) override
    {
        if (matrix.isNotEmpty())
            return PatchMatrixComponent::paint (g);
    }

    void handleNodeMenuResult (const int result, const Node& node)
    {
        switch (result)
        {
            case NodePopupMenu::RemoveNode: {
                ViewHelpers::postMessageFor (this, new RemoveNodeMessage (node));
            }
            break;

            case NodePopupMenu::Duplicate: {
                ViewHelpers::postMessageFor (this, new DuplicateNodeMessage (node));
            }
            break;
        }
    }

    void showMenuForNode (const Node& node)
    {
        auto* const world = ViewHelpers::getGlobals (this);

        NodePopupMenu menu (node);

        if (world)
            menu.addPresetsMenu (world->presets());

        const int result = menu.show();
        if (auto* message = menu.createMessageForResultCode (result))
        {
            ViewHelpers::postMessageFor (this, message);
            return;
        }
        handleNodeMenuResult (result, node);
    }

    void showMenuForNodeAndPort (const Node& n, const Port& p)
    {
        auto* const world = ViewHelpers::getGlobals (this);

        NodePopupMenu menu (n, p);

        if (world)
            menu.addPresetsMenu (world->presets());
        const int result = menu.show();
        if (auto* message = menu.createMessageForResultCode (result))
        {
            ViewHelpers::postMessageFor (this, message);
            return;
        }
        handleNodeMenuResult (result, n);
    }

    boost::signals2::signal<void()> sigSelectionChanged;

private:
    friend class ConnectionGrid;
    friend class Sources;
    friend class Destinations;

    bool useHighlighting;
    MatrixState matrix;
    ValueTree nodeModels;
    ValueTree graphModel;
    NodeArray nodes;
    PortArray ins, outs;
    Array<int> audioInIndexes, audioOutIndexes, audioInChannels, audioOutChannels,
        midiInIndexes, midiOutIndexes, midiInChannels, midiOutChannels;

    void matrixHoveredCellChanged (const int prevRow, const int prevCol, const int newRow, const int newCol) override
    {
        auto* quads = findParentComponentOfClass<QuadrantLayout>();
        if (auto* sources = dynamic_cast<ListBox*> (quads->getQauadrantComponent (QuadrantLayout::Q2)))
        {
            sources->repaintRow (prevRow);
            sources->repaintRow (newRow);
        }

        if (auto* dests = dynamic_cast<HorizontalListBox*> (quads->getQauadrantComponent (QuadrantLayout::Q4)))
        {
            dests->repaintRow (prevCol);
            dests->repaintRow (newCol);
        }
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected, bool isSource)
    {
        const int padding = 18;
        const Node node (getNode (rowNumber, isSource));
        const Port port (getPort (rowNumber, isSource));

        String text = node.getName();

        {
            String portName = port.getName();
            if (portName.isEmpty())
                portName << port.getType().getName() << " " << (1 + port.channel());
            text << " - " << portName;
        }

        Rectangle<int> r (0, 0, width, height);

        const bool hover = (isSource) ? mouseIsOverRow (rowNumber)
                                      : mouseIsOverColumn (rowNumber);
        g.setColour (rowIsSelected || hover ? Colors::elemental.withAlpha (0.4f) : Colors::widgetBackgroundColor);

        if (isSource)
            g.fillRect (0, 0, width - 1, height - 1);
        else
            g.fillRect (0, 1, width - 1, height - 1);

        g.setColour (Colours::white);

        if (isSource)
        {
            g.drawText (text, padding, 0, width - 1 - padding, height - 1, Justification::centredLeft);
        }
        else
        {
            Rectangle<int> r2 = { padding, 0, height - 1 - padding, width };
            Style::drawVerticalText (g, text, r.withY (padding), Justification::centredRight);
        }
    }

    void emptyAreaClicked (const MouseEvent& ev)
    {
        if (! ev.mods.isPopupMenu())
        {
            return;
        }

        const Node graph (graphModel);

        PluginsPopupMenu menu (this);
        if (graph.isRootGraph())
        {
            menu.addSectionHeader ("Graph I/O");
            menu.addItem (1, "Audio Inputs", true, graph.hasAudioInputNode());
            menu.addItem (2, "Audio Outputs", true, graph.hasAudioOutputNode());
            menu.addItem (3, "MIDI Input", true, graph.hasMidiInputNode());
            menu.addItem (4, "MIDI Output", true, graph.hasMidiOutputNode());
        }

        menu.addSectionHeader ("Plugins");
        menu.addPluginItems();
        const int result = menu.show();
        if (menu.isPluginResultCode (result))
        {
            bool verified = false;
            const auto desc = menu.getPluginDescription (result, verified);
            if (desc.fileOrIdentifier.isNotEmpty() && desc.pluginFormatName.isNotEmpty())
                ViewHelpers::postMessageFor (this, new AddPluginMessage (graph, desc, verified));
        }
        else
        {
            PluginDescription desc;
            desc.pluginFormatName = "Internal";
            bool hasRequestedType = false;
            bool failure = false;
            ValueTree node;

            switch (result)
            {
                case 1:
                    desc.fileOrIdentifier = "audio.input";
                    hasRequestedType = graph.hasAudioInputNode();
                    break;
                case 2:
                    desc.fileOrIdentifier = "audio.output";
                    hasRequestedType = graph.hasAudioOutputNode();
                    break;
                case 3:
                    desc.fileOrIdentifier = "midi.input";
                    hasRequestedType = graph.hasMidiInputNode();
                    break;
                case 4:
                    desc.fileOrIdentifier = "midi.output";
                    hasRequestedType = graph.hasMidiOutputNode();
                    break;
                default:
                    failure = true;
                    break;
            }

            if (failure)
            {
                DBG ("[element] unkown menu result: " << result);
            }
            else if (hasRequestedType)
            {
                const ValueTree nodeToRemove = graph.getNodesValueTree()
                                                   .getChildWithProperty (tags::identifier, desc.fileOrIdentifier);
                const Node model (nodeToRemove, false);
                ViewHelpers::postMessageFor (this, new RemoveNodeMessage (model));
            }
            else
            {
                ViewHelpers::postMessageFor (this, new LoadPluginMessage (desc, true));
            }
        }
    }

    void listBoxItemClicked (int row, const MouseEvent& ev, bool isSource)
    {
        const Node node (getNode (row, isSource));
        const Port port (getPort (row, isSource));

        if (ev.mods.isPopupMenu())
            showMenuForNodeAndPort (node, port);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent& ev, bool isSource)
    {
        const Node node (getNode (row, isSource));
        if (node.isGraph())
        {
            auto* cc = ViewHelpers::findContentComponent (this);
            cc->setCurrentNode (node);
        }
        else
        {
            ViewHelpers::presentPluginWindow (this, node);
        }
    }

    static ValueTree findArc (const ValueTree& arcs, uint32 sourceNode, int sourceChannel, uint32 destNode, int destChannel)
    {
        for (int i = arcs.getNumChildren(); --i >= 0;)
        {
            const ValueTree arc (arcs.getChild (i));
            if (sourceNode == (uint32) (int64) arc.getProperty (tags::sourceNode) && sourceChannel == (int) arc.getProperty (tags::sourceChannel) && destNode == (uint32) (int64) arc.getProperty (tags::destNode) && destChannel == (int) arc.getProperty (tags::destChannel))
            {
                return arc;
            }
        }

        return ValueTree();
    }

    static ValueTree findArc (const ValueTree& arcs, uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort)
    {
        for (int i = arcs.getNumChildren(); --i >= 0;)
        {
            const ValueTree arc (arcs.getChild (i));
            if (static_cast<int> (sourceNode) == (int) arc.getProperty (tags::sourceNode) && static_cast<int> (sourcePort) == (int) arc.getProperty (tags::sourcePort) && static_cast<int> (destNode) == (int) arc.getProperty (tags::destNode) && static_cast<int> (destPort) == (int) arc.getProperty (tags::destPort))
            {
                return arc;
            }
        }

        return ValueTree();
    }

    static ValueTree findArc (const ValueTree& arcs, const Node& sourceNode, int sourceChannel, const Node& destNode, int destChannel)
    {
        return findArc (arcs, sourceNode.getNodeId(), sourceChannel, destNode.getNodeId(), destChannel);
    }

    void resetMatrix()
    {
        const ValueTree arcs (nodeModels.getParent().getChildWithName (tags::arcs));
        //FIXME: jassert (arcs.hasType (tags::arcs));
        for (int row = 0; row < matrix.getNumRows(); ++row)
        {
            for (int col = 0; col < matrix.getNumColumns(); ++col)
            {
                const Node src = getNode (row, true);
                const Port srcPort = getPort (row, true);
                const Node dst = getNode (col, false);
                const Port dstPort = getPort (col, false);

                const ValueTree arc (findArc (arcs, src.getNodeId(), srcPort.index(), dst.getNodeId(), dstPort.index()));
                if (arc.isValid())
                    matrix.connect (row, col);
                else
                    matrix.disconnect (row, col);
            }
        }
    }

    void buildNodeArray()
    {
        nodes.clearQuick();
        for (int i = 0; i < nodeModels.getNumChildren(); ++i)
        {
            const Node node (nodeModels.getChild (i));
            nodes.add (node);
        }

        updateContent();
    }

    friend class ValueTree;
    virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged,
                                           const Identifier& property) override {}

    virtual void valueTreeChildAdded (ValueTree& parentTree,
                                      ValueTree& childWhichHasBeenAdded) override
    {
        if (parentTree == nodeModels.getParent() && childWhichHasBeenAdded.hasType (tags::nodes))
        {
            buildNodeArray();
            resetMatrix();
        }
        else if (nodeModels == parentTree && childWhichHasBeenAdded.hasType (types::Node))
        {
            buildNodeArray();
        }
        else if (childWhichHasBeenAdded.hasType (tags::ports) || childWhichHasBeenAdded.hasType (types::Port))
        {
            buildNodeArray();
        }
        else if (childWhichHasBeenAdded.hasType (tags::arcs) || childWhichHasBeenAdded.hasType (types::Arc))
        {
            buildNodeArray();
        }
    }

    virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        if (parentTree == nodeModels.getParent() && childWhichHasBeenRemoved.hasType (tags::nodes))
        {
            buildNodeArray();
            resetMatrix();
        }
        else if (nodeModels == parentTree && childWhichHasBeenRemoved.hasType (types::Node))
        {
            buildNodeArray();
        }
        else if (childWhichHasBeenRemoved.hasType (tags::ports) || childWhichHasBeenRemoved.hasType (types::Port))
        {
            buildNodeArray();
        }
        else if (childWhichHasBeenRemoved.hasType (tags::arcs) || childWhichHasBeenRemoved.hasType (types::Arc))
        {
            buildNodeArray();
        }
    }

    virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                             int oldIndex,
                                             int newIndex) override {}

    virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override
    {
        if (treeWhoseParentHasChanged.hasType (tags::nodes))
        {
            nodeModels = treeWhoseParentHasChanged;
        }
    }

    virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override
    {
        if (nodeModels != treeWhichHasBeenChanged)
        {
        }
        graphModel = nodeModels.getParent();
        buildNodeArray();
        resetMatrix();
    }
};

namespace detail {

// =============================================================================
template <class Box>
static void boxInit (Box& box)
{
    box.setMultipleSelectionEnabled (true);
    // box.setRowSelectedOnMouseDown (true);
    box.setClickingTogglesRowSelection (false);
}
} // namespace detail

// =============================================================================
class ConnectionGrid::Sources : public ListBox,
                                public ListBoxModel
{
public:
    Sources (PatchMatrix* m)
        : matrix (m)
    {
        jassert (m != nullptr);
        setRowHeight (matrix->getRowThickness());
        setModel (this);
        detail::boxInit (*this);
    }

    ~Sources() {}

    int getNumRows() override { return matrix->getNumRows(); };

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        matrix->paintListBoxItem (rowNumber, g, width, height, rowIsSelected, true);
    }

    void listWasScrolled() override
    {
        if (auto* scroll = &getVerticalScrollBar())
        {
            matrix->setOffsetY (-roundToInt (scroll->getCurrentRangeStart()));
            matrix->repaint();
        }
    }

    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        matrix->listBoxItemClicked (row, ev, true);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent& ev) override
    {
        matrix->listBoxItemDoubleClicked (row, ev, true);
    }

    void backgroundClicked (const MouseEvent& ev) override
    {
        matrix->emptyAreaClicked (ev);
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
        const Node node (matrix->getNode (lastRowSelected, true));
        ViewHelpers::postMessageFor (this, new RemoveNodeMessage (node));
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
        matrix->sigSelectionChanged();
    }

#if 0
        virtual Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                                   Component* existingComponentToUpdate);
        
        virtual void returnKeyPressed (int lastRowSelected);
        virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe);
        virtual String getTooltipForRow (int row);
        virtual MouseCursor getMouseCursorForRow (int row);
#endif

private:
    PatchMatrix* matrix;
    friend class PatchMatrix;
};

// =============================================================================
class ConnectionGrid::Destinations : public HorizontalListBox,
                                     public ListBoxModel
{
public:
    Destinations (PatchMatrix* m)
        : matrix (m)
    {
        jassert (m != nullptr);
        setRowHeight (matrix->getColumnThickness());
        setModel (this);
        detail::boxInit (*this);
    }

    int getNumRows() override { return matrix->getNumColumns(); }

    void mouseDown (const MouseEvent& ev) override
    {
        return;
    }

    void mouseUp (const MouseEvent& ev) override
    {
        return;
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        matrix->paintListBoxItem (rowNumber, g, width, height, rowIsSelected, false);
    }

    void listWasScrolled() override
    {
        if (auto* scroll = getHorizontalScrollBar())
        {
            matrix->setOffsetX (-roundToInt (scroll->getCurrentRangeStart()));
            matrix->repaint();
        }
    }

    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        matrix->listBoxItemClicked (row, ev, false);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent& ev) override
    {
        matrix->listBoxItemDoubleClicked (row, ev, false);
    }

    void backgroundClicked (const MouseEvent& ev) override
    {
        matrix->emptyAreaClicked (ev);
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
        const Node node (matrix->getNode (lastRowSelected, false));
        ViewHelpers::postMessageFor (this, new RemoveNodeMessage (node));
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
        matrix->sigSelectionChanged();
    }

private:
    PatchMatrix* matrix;
    friend class PatchMatrix;
};

// =============================================================================
class ConnectionGrid::Controls : public Component
{
public:
    Controls (PatchMatrix& m, Sources& s, Destinations& t)
        : matrix (m), sources (s), targets (t)
    {
        addAndMakeVisible (routeButton);
        routeButton.onClick = [this]() { applyRouting(); };
        selectionConn = matrix.sigSelectionChanged.connect (
            std::bind (&Controls::stabilize, this));
        stabilize();
    }

    ~Controls()
    {
        selectionConn.disconnect();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (32, 0);
        r.removeFromTop (getHeight() / 2 - routeButtonHeight / 2);
        routeButton.changeWidthToFitText (routeButtonHeight);
        routeButton.setBounds (r.removeFromTop (routeButtonHeight));
    }

    void mouseDown (const MouseEvent& ev) override
    {
        if (ev.mods.isPopupMenu())
            matrix.emptyAreaClicked (ev);
    }

private:
    PatchMatrix& matrix;
    Sources& sources;
    Destinations& targets;

    int routeButtonHeight = 26;
    TextButton routeButton { "Route", "Apply routes to selection." };
    boost::signals2::connection selectionConn;

    bool canRoute() const
    {
        return sources.getNumSelectedRows() > 0 && targets.getNumSelectedRows() > 0;
    }

    void stabilize()
    {
        routeButton.setEnabled (canRoute());
    }

    void applyRouting()
    {
        if (! canRoute())
            return;

        if (auto ctx = ViewHelpers::getGlobals (this))
        {
            auto& e = *ctx->services().find<EngineService>();

            Array<Port> srcPorts[PortType::Unknown];
            Array<Port> dstPorts[PortType::Unknown];

            for (int i = 0; i < sources.getNumSelectedRows(); ++i)
            {
                auto port = matrix.getPort (sources.getSelectedRow (i), true);
                srcPorts[port.getType()].add (port);
            }

            for (int i = 0; i < targets.getNumSelectedRows(); ++i)
            {
                auto port = matrix.getPort (targets.getSelectedRow (i), false);
                dstPorts[port.getType()].add (port);
            }

            for (auto pt : PortType::all())
            {
                auto& s = srcPorts[pt];
                auto& t = dstPorts[pt];
                if (s.size() <= 0 || t.size() <= 0)
                    continue;

                auto nc = std::max (s.size(), t.size());
                int sidx = 0, tidx = 0;

                while (--nc >= 0)
                {
                    // std::clog << "s=" << s[sidx] << " t=" << t[tidx] << std::endl;
                    auto sp = s.getUnchecked (sidx);
                    auto tp = t.getUnchecked (tidx);

                    e.connect (sp.getType(), sp.getNode(), sp.channel(), tp.getNode(), tp.channel());

                    if (++sidx >= s.size())
                        sidx = 0;
                    if (++tidx >= t.size())
                        tidx = 0;
                }
            }
        }

        if (auto grid = findParentComponentOfClass<ConnectionGrid>())
            grid->stabilizeContent();
    }
};

// =============================================================================
class ConnectionGrid::Quads : public QuadrantLayout
{
public:
    enum ResizeMode
    {
        DynamicQ1 = 0
    };

    Quads() {}
    ~Quads() {}

    void setResizeMode (const ResizeMode mode)
    {
        if (mode == resizeMode)
            return;
        resizeMode = mode;
        resized();
    }

    void updateCenter() override
    {
        switch (resizeMode)
        {
            case DynamicQ1:
                updateForDynamicQ1();
                break;
        }
    }

private:
    ResizeMode resizeMode = DynamicQ1;
    int thicknessOnOtherQuads = 190;

    // keeps q2, q3, and q4 static
    void updateForDynamicQ1()
    {
        const int w = getWidth();
        const int h = getHeight();
        const int x = (thicknessOnOtherQuads <= w) ? thicknessOnOtherQuads : 0;
        const int y = (h - thicknessOnOtherQuads >= 0) ? h - thicknessOnOtherQuads : 0;
        setCenter (x, y);
    }
};

// =============================================================================
void ConnectionGrid::PatchMatrix::updateContent()
{
    audioInIndexes.clearQuick();
    audioOutIndexes.clearQuick();
    audioInChannels.clearQuick();
    audioOutChannels.clearQuick();
    midiInIndexes.clearQuick();
    midiInChannels.clearQuick();
    midiOutIndexes.clearQuick();
    midiOutChannels.clearQuick();
    ins.clearQuick();
    outs.clearQuick();
    int newNumRows = 0, newNumCols = 0, nodeIndex = 0;

    for (const Node& node : nodes)
    {
        const ValueTree ports (node.getPortsValueTree());
        for (int i = 0; i < ports.getNumChildren(); ++i)
        {
            const Port port (ports.getChild (i));
            if (port.getType() != PortType::Audio && port.getType() != PortType::Midi)
                continue;

            Array<int>& inIndexes = port.getType() == PortType::Audio ? audioInIndexes : audioInIndexes;
            Array<int>& inChannels = port.getType() == PortType::Audio ? audioInChannels : audioInChannels;
            Array<int>& outIndexes = port.getType() == PortType::Audio ? audioOutIndexes : audioOutIndexes;
            Array<int>& outChannels = port.getType() == PortType::Audio ? audioOutChannels : audioOutChannels;

            if (port.isInput())
            {
                inIndexes.add (nodeIndex);
                inChannels.add (i);
                ins.add (port);
                ++newNumCols;
            }
            else
            {
                outIndexes.add (nodeIndex);
                outChannels.add (i);
                outs.add (port);
                ++newNumRows;
            }
        }

        ++nodeIndex;
    }

    matrix.resize (newNumRows, newNumCols);
    jassert (newNumRows == outs.size() && newNumCols == ins.size());

    resetMatrix();
    if (auto* grid = findParentComponentOfClass<ConnectionGrid>())
    {
        grid->sources->updateContent();
        grid->sources->repaint();
        grid->destinations->updateContent();
        grid->destinations->repaint();
    }

    repaint();
}

// =============================================================================
ConnectionGrid::ConnectionGrid()
{
    setName ("PatchBay"); // Don't change this

    quads = std::make_unique<Quads>();
    addAndMakeVisible (quads.get());
    quads->setQuadrantComponent (Quads::Q1, matrix = new PatchMatrix());
    quads->setQuadrantComponent (Quads::Q2, sources = new Sources (matrix));
    quads->setQuadrantComponent (Quads::Q4, destinations = new Destinations (matrix));
    quads->setQuadrantComponent (Quads::Q3, controls = new Controls (*matrix, *sources, *destinations));

    breadcrumb = std::make_unique<BreadCrumbComponent>();
    addAndMakeVisible (breadcrumb.get());
    breadcrumb->toFront (false);
    resized();
}

ConnectionGrid::~ConnectionGrid()
{
    matrix = nullptr;
    sources = nullptr;
    controls = nullptr;
    destinations = nullptr;
    quads = nullptr;
}

void ConnectionGrid::setNode (const Node& newNode)
{
    if (newNode.isGraph() && ! newNode.isRootGraph())
    {
        // noop
    }
    else
    {
        setEnabled (true);
        setInterceptsMouseClicks (true, true);
    }

    ValueTree newNodes = newNode.hasNodeType (types::Graph)
                             ? newNode.getNodesValueTree()
                             : ValueTree (tags::nodes);
    jassert (this->matrix != nullptr);
    matrix->nodeModels = newNodes;
    if (breadcrumb)
        breadcrumb->setNode (newNode);
}

void ConnectionGrid::paint (Graphics& g)
{
    g.fillAll (Colors::contentBackgroundColor);
}

void ConnectionGrid::resized()
{
    auto r = getLocalBounds();
    if (breadcrumb)
        breadcrumb->setBounds (r.removeFromTop (24));
    quads->setBounds (r);
}

void ConnectionGrid::mouseDown (const MouseEvent& ev)
{
    Component::mouseDown (ev);
}

bool ConnectionGrid::isInterestedInDragSource (const SourceDetails& sd)
{
    return sd.description.isArray() && sd.description.size() == 3 && sd.description[0].toString() == "element://dnd/plugin";
}

void ConnectionGrid::itemDropped (const SourceDetails& sd)
{
    PluginDescription desc;
    desc.pluginFormatName = sd.description[1];
    desc.fileOrIdentifier = sd.description[2];
    ViewHelpers::postMessageFor (this, new LoadPluginMessage (desc, false));
}

void ConnectionGrid::didBecomeActive()
{
    auto session = ViewHelpers::findContentComponent (this)->session();
    setNode (session->getCurrentGraph());
}
} // namespace element

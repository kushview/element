/*
    This file is part of the element modules for the JUCE Library
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"
#include "engine/GraphProcessor.h"
#include "gui/HorizontalListBox.h"
#include "gui/ViewHelpers.h"
#include "gui/ContentComponent.h"
#include "gui/GuiApp.h"
#include "gui/ViewHelpers.h"
#include "session/PluginManager.h"
#include "session/NodeModel.h"
#include "Messages.h"

#include "gui/ConnectionGrid.h"

namespace Element {
    // Spacing between each patch point
    static const int gridPadding = 1;
    
    class ConnectionGrid::PatchMatrix :  public PatchMatrixComponent,
                                         private ValueTree::Listener
    {
    public:
        PatchMatrix()
            : matrix()
        {
            setSize (300, 200);

            nodeModels = ValueTree (Tags::nodes);
            nodes.clear();
            nodeModels.addListener (this);
        }
        
        ~PatchMatrix()
        {
            nodeModels.removeListener (this);
        }
        
        
        const Node getNode (const int index, const bool isSource) const
        {
            return nodes [isSource ? audioOutIndexes [index] : audioInIndexes [index]];
        }
        
        const int getAudioChannelForIndex (const int index, const bool isSource) const
        {
            return isSource ? audioOutChannels.getUnchecked(index)
                            : audioInChannels.getUnchecked(index);
        }
        
        void updateContent();
        
        void mouseMove (const MouseEvent& ev) override {
            PatchMatrixComponent::mouseMove (ev);
            repaint();
        }
        
        void paintMatrixCell (Graphics& g, const int width, const int height,
                              const int row, const int column) override
        {
            const Node src (getNode (row, true));
            const Node dst (getNode (column, false));
            
            if (mouseIsOverCell (row, column) && ! matrix.connected (row, column))
            {
                g.setColour (Element::LookAndFeel_E1::elementBlue.withAlpha (0.3f));
                g.fillRect (0, 0, width - gridPadding, height - gridPadding);
            }
            else
            {
                g.setColour (matrix.connected (row, column) ?
                             Colour (Element::LookAndFeel_E1::elementBlue.brighter()) :
                             Colour (Element::LookAndFeel_E1::defaultMatrixCellOffColor));
        
                g.fillRect (0, 0, width - gridPadding, height - gridPadding);

            }
        }
        
        void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
        {
            const Node srcNode (getNode (row, true));
            const int srcChan (getAudioChannelForIndex (row, true));
            const Node dstNode (getNode (col, false));
            const int dstChan (getAudioChannelForIndex (col, false));
            
            if (ev.mods.isPopupMenu()) {
                PopupMenu menu;
                menu.addItem (1, "hello");
                menu.show();
                return;
            }
            
            if (! srcNode.canConnectTo (dstNode)) {
                matrix.disconnect (row, col);
                return;
            }
            
            ValueTree arcs (srcNode.getParentArcsNode());
            for (int i = arcs.getNumChildren(); --i >= 0;)
            {
                const ValueTree arc (arcs.getChild (i));
                if (srcNode.getNodeId() == (uint32)(int64)arc.getProperty ("srcNode") &&
                    srcChan == (int) arc.getProperty ("srcChannel") &&
                    dstNode.getNodeId() == (uint32)(int64)arc.getProperty ("dstNode") &&
                    dstChan == (int) arc.getProperty ("dstChannel"))
                {
                    matrix.disconnect (row, col);
                    arcs.removeChild (arc, nullptr);
                    repaint();
                    return;
                }
            }
            
            ValueTree arc (Tags::arc);
            arc.setProperty ("srcNode", (int64) srcNode.getNodeId(), nullptr)
               .setProperty ("srcChannel", srcChan, nullptr)
               .setProperty ("dstNode", (int64) dstNode.getNodeId(), nullptr)
               .setProperty ("dstChannel", dstChan, nullptr);
            jassert (arcs.hasType (Tags::arcs));
            arcs.addChild (arc, -1, nullptr);
            matrix.connect (row, col);
            repaint();
        }
        
        int getNumRows()    override { return matrix.getNumRows(); }
        int getNumColumns() override { return matrix.getNumColumns(); }
        
        void paint (Graphics& g) override
        {
            if (matrix.isNotEmpty())
                return PatchMatrixComponent::paint (g);
            
            paintEmptyMessage (g, getWidth(), getHeight());
        }
        
    private:
        friend class ConnectionGrid;
        friend class Sources;
        friend class Destinations;
        MatrixState matrix;
        ValueTree nodeModels;
        NodeArray nodes;
        Array<int> audioInIndexes, audioOutIndexes, audioInChannels, audioOutChannels;

        void paintEmptyMessage (Graphics& g, const int width, const int height) {
//            return;
//            g.fillAll (LookAndFeel_E1::widgetBackgroundColor.darker());
            g.setColour(LookAndFeel_E1::textColor);
            g.drawFittedText ("Nothing to see here...", 0, 0, width, height, Justification::centred, 2);
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                               bool rowIsSelected, bool isSource)
        {
            const Node node (getNode (rowNumber, isSource));
            const int channel = 1 + getAudioChannelForIndex (rowNumber, isSource);
            String text = node.getName();
            text << " " << channel;
            g.setColour (LookAndFeel_E1::widgetBackgroundColor);
            g.fillRect (0, 0, width - 1, height - 1);
            g.setColour (Colours::white);
            
            if (isSource)
            {
                g.drawText (text, 0, 0, width - 1, height - 1, Justification::centredLeft);
            }
            else
            {
                g.saveState();
                g.addTransform (AffineTransform::identity.rotated (
                    1.57079633f, (float)width, 0.0f));
                g.drawFittedText (text, 0, 0, height - 1, width - 1,
                                  Justification::centredRight, 1);
                g.restoreState();
            }
        }
        
        void buildNodeArray()
        {
            nodes.clearQuick();
            for (int i = 0; i < nodeModels.getNumChildren(); ++i)
            {
                const Node node (nodeModels.getChild(i));
                nodes.add (node);
            }
            
            updateContent();
        }
        
        friend class ValueTree;
        virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged,
                                               const Identifier& property) override { }
        
        virtual void valueTreeChildAdded (ValueTree& parentTree,
                                          ValueTree& childWhichHasBeenAdded) override
        {
            if (nodeModels == parentTree && childWhichHasBeenAdded.hasType(Tags::node))
                buildNodeArray();
        }
        
        virtual void valueTreeChildRemoved (ValueTree& parentTree,
                                            ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved) override
        {
            if (nodeModels == parentTree && childWhichHasBeenRemoved.hasType (Tags::node))
                buildNodeArray();
        }
        
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                                 int oldIndex, int newIndex) override { }
        
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override { }
        
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override
        {
            if (nodeModels != treeWhichHasBeenChanged)
                return;
            buildNodeArray();
        }
    };

    // MARK: Sources
    
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
        }
        
        ~Sources() { }
        
        int getNumRows() override { return matrix->getNumRows(); };
        
        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                               bool rowIsSelected) override
        {
            matrix->paintListBoxItem (rowNumber, g, width, height, rowIsSelected, true);
        }
        
        void listWasScrolled() override
        {
            if (auto *scroll = getVerticalScrollBar())
            {
                matrix->setOffsetY (-roundDoubleToInt (scroll->getCurrentRangeStart()));
                matrix->repaint();
            }
        }
        
#if 0
        virtual Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                                   Component* existingComponentToUpdate);
        virtual void listBoxItemClicked (int row, const MouseEvent&);
        virtual void listBoxItemDoubleClicked (int row, const MouseEvent&);
        virtual void backgroundClicked (const MouseEvent&);
        virtual void selectedRowsChanged (int lastRowSelected);
        virtual void deleteKeyPressed (int lastRowSelected);
        virtual void returnKeyPressed (int lastRowSelected);
        
        virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe);
        virtual String getTooltipForRow (int row);
        virtual MouseCursor getMouseCursorForRow (int row);
#endif
        
    private:
        PatchMatrix* matrix;
        friend class PatchMatrix;
    };
    
    // MARK: Controls
    
    class ConnectionGrid::Controls : public Component { };
    
    // MARK: Destinations
    
    class ConnectionGrid::Destinations : public HorizontalListBox,
                                         public ListBoxModel
    {
    public:
        Destinations (PatchMatrix* m)
            : matrix (m)
        {
            jassert(m != nullptr);
            setRowHeight (matrix->getColumnThickness());
            setModel (this);
        }
        
        int getNumRows() override { return matrix->getNumColumns(); }
        
        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                               bool rowIsSelected) override
        {
            matrix->paintListBoxItem (rowNumber, g, width, height, rowIsSelected, false);
        }
        
        void listWasScrolled() override
        {
            if (auto *scroll = getHorizontalScrollBar())
            {
                matrix->setOffsetX (-roundDoubleToInt (scroll->getCurrentRangeStart()));
                matrix->repaint();
            }
        }

    private:
        PatchMatrix* matrix;
        friend class PatchMatrix;
    };
    
    // MARK: Quads
    
    class ConnectionGrid::Quads : public QuadrantLayout
    {
    public:
        Quads() { }
        ~Quads() { }
        
        void updateCenter() override {
            QuadrantLayout::updateCenter();
        }
    };
    
    // MARK: PatchMatrix IMPL
    
    void ConnectionGrid::PatchMatrix::updateContent()
    {
        audioInIndexes.clearQuick(); audioOutIndexes.clearQuick();
        audioInChannels.clearQuick(); audioOutChannels.clearQuick();
        int newNumRows = 0, newNumCols = 0, nodeIndex = 0;
        for (const Node& node : nodes)
        {
            const int numAudioIns   = node.getNumAudioIns();
            const int numAudioOuts  = node.getNumAudioOuts();
            for (int i = 0; i < jmax (numAudioIns, numAudioOuts); ++i)
            {
                if (i < numAudioIns)
                {
                    audioInIndexes.add (nodeIndex);
                    audioInChannels.add (i);
                }
                
                if (i < numAudioOuts)
                {
                    audioOutIndexes.add (nodeIndex);
                    audioOutChannels.add (i);
                }
            }
            
            newNumRows += numAudioOuts;
            newNumCols += numAudioIns;
            ++nodeIndex;
        }
        
        matrix.resize (newNumRows, newNumCols);
        jassert(newNumRows == audioOutChannels.size() &&
                newNumCols == audioInChannels.size());
        repaint();
        if (auto* grid = findParentComponentOfClass<ConnectionGrid>())
        {
            grid->sources->updateContent();
            grid->destinations->updateContent();
        }
    }

    // MARK: Connection Grid IMPL
    
    ConnectionGrid::ConnectionGrid ()
    {
        addAndMakeVisible (quads = new Quads());
        quads->setQuadrantComponent (Quads::Q1, matrix = new PatchMatrix());
        quads->setQuadrantComponent (Quads::Q2, sources = new Sources (matrix));
        quads->setQuadrantComponent (Quads::Q3, controls = new Controls());
        quads->setQuadrantComponent (Quads::Q4, destinations = new Destinations (matrix));
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
        jassert (newNode.hasNodeType (Tags::graph)); // need a graph at the moment
        jassert (this->matrix != nullptr);
        DBG(newNode.node().toXmlString());
        matrix->nodeModels = newNode.node().getChildWithName(Tags::nodes);
    }
    
    void ConnectionGrid::paint (Graphics& g)
    {
        g.fillAll (LookAndFeel_E1::contentBackgroundColor);
    }
    
    void ConnectionGrid::resized()
    {
        quads->setBounds (getLocalBounds());
    }
    
    bool ConnectionGrid::isInterestedInDragSource (const SourceDetails& sd)
    {
        return sd.description.isArray() &&
               sd.description.size() == 3 &&
               sd.description[0].toString() == "element://dnd/plugin";
    }
    
    void ConnectionGrid::itemDropped (const SourceDetails& sd)
    {
        PluginDescription desc;
        desc.pluginFormatName = sd.description[1];
        desc.fileOrIdentifier = sd.description[2];
        ViewHelpers::postMessageFor (this, new LoadPluginMessage (desc));
    }
}

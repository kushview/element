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
        
        void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
        {
            matrix.toggleCell (row, col);
            repaint();
        }
        
        const Node getNode (const int index) const { return nodes [index]; }
        
        void updateContent();
        
        void paintMatrixCell (Graphics& g, const int width, const int height,
                              const int row, const int column) override
        {
            g.setColour (matrix.isCellToggled (row, column) ?
                         Colour (Element::LookAndFeel_E1::defaultMatrixCellOnColor) :
                         Colour (Element::LookAndFeel_E1::defaultMatrixCellOffColor));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }

        int getNumRows()    override { return nodes.size(); }
        int getNumColumns() override { return nodes.size(); }

        void updateMatrix (const MatrixState& state) {
            matrix = state;
        }
        
        void paint (Graphics& g) override
        {
            if (matrix.isNotEmpty())
                return PatchMatrixComponent::paint(g);
            
            paintEmptyMessage (g, getWidth(), getHeight());
        }
        
    private:
        friend class ConnectionGrid;
        MatrixState matrix;
        ValueTree nodeModels;
        NodeArray nodes;

        void paintEmptyMessage (Graphics& g, const int width, const int height) {
//            return;
//            g.fillAll (LookAndFeel_E1::widgetBackgroundColor.darker());
//            g.setColour(LookAndFeel_E1::textColor);
            g.drawFittedText ("Nothing to see here...", 0, 0, width, height, Justification::centred, 2);
        }
        
        void buildNodeArray()
        {
            // DBG(nodeModels.toXmlString());
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
            buildNodeArray();
        }
        
        virtual void valueTreeChildRemoved (ValueTree& parentTree,
                                            ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved) override
        {
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
    
    class ConnectionGrid::Sources : public ListBox,
                                    public ListBoxModel
    {
    public:
        Sources (PatchMatrix* m)
            : matrix(m)
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
            const Node node (matrix->getNode (rowNumber));
            g.setColour (LookAndFeel_E1::widgetBackgroundColor);
            g.fillRect (0, 0, width - 1, height - 1);
            
            g.setColour (Colours::white);
            g.drawText (node.getName(), 0, 0, width - 1, height - 1, Justification::centredLeft);
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
    
    class ConnectionGrid::Controls : public Component { };
    
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
            const Node node (matrix->getNode (rowNumber));
            g.setColour (LookAndFeel_E1::widgetBackgroundColor);
            g.fillRect (0, 0, width - 1, height - 1);
            
            // ViewHelpers::drawVerticalTextRow (node.getName(), g, width, height, rowIsSelected);
            g.saveState();
            g.addTransform (AffineTransform::identity.rotated (1.57079633f, (float)width, 0.0f));
            
//            if (selected)
//            {
//                g.setColour(LF::textColor.darker (0.6000006));
//                g.setOpacity (0.60);
//                g.fillRect (0, 0, h, w);
//            }
            
#if JUCE_MAC
            // g.setFont (Resources::normalFontSize);
#endif
            
            g.setColour (Colours::white);
            g.drawFittedText (node.getName(), 0, 0, height - 1, width - 1, Justification::centredRight, 1);
            
            g.restoreState();
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
    
    class ConnectionGrid::Quads : public QuadrantLayout
    {
    public:
        Quads() { }
        ~Quads() { }
        
        void updateCenter() override {
            QuadrantLayout::updateCenter();
        }
    };
    
    void ConnectionGrid::PatchMatrix::updateContent()
    {
        int newNumRows = nodes.size();
        int newNumCols = nodes.size();
        
        // if (auto *cc = findParentComponentOfClass<ContentComponent>())
        // {
        //     auto e = cc->getGlobals().engine();
        //     auto& g (e->graph());
        //     for (int i = 0; i < g.getNumNodes(); ++i)
        //     {
        //         GraphNodePtr node = g.getNode (i);
        //         newNumRows += node->getNumAudioInputs();
        //         newNumCols += node->getNumAudioOutputs();
        //     }
        // }
        
        matrix.resize (newNumRows, newNumCols);
        if (auto* p = findParentComponentOfClass<ConnectionGrid>())
        {
            p->sources->updateContent();
            p->destinations->updateContent();
        }
        repaint();
    }

    
    
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

/*
    This file is part of the element modules for the JUCE Library
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.
*/

#include "gui/HorizontalListBox.h"
#include "gui/ViewHelpers.h"
#include "gui/ContentComponent.h"
#include "gui/GuiApp.h"
#include "gui/ViewHelpers.h"
#include "session/PluginManager.h"
#include "Messages.h"

#include "gui/ConnectionGrid.h"

#define NUM_DUMMY_NODES 16

namespace Element {
    // Spacing between each patch point
    static const int gridPadding = 1;
    
    class ConnectionGrid::PatchMatrix :  public PatchMatrixComponent
    {
    public:
        PatchMatrix() : matrix (NUM_DUMMY_NODES, NUM_DUMMY_NODES)
        {
            for (int i = 0 ; i < NUM_DUMMY_NODES; ++i)
                matrix.connect (i, i);
            
            setSize (300, 200);
        }
        
        ~PatchMatrix() { }
        
        void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
        {
            matrix.toggleCell (row, col);
            repaint();
        }
        
        void paintMatrixCell (Graphics& g, const int width, const int height,
                              const int row, const int column) override
        {
            g.setColour (matrix.isCellToggled (row, column) ?
                         Colour (Element::LookAndFeel_E1::defaultMatrixCellOnColor) :
                         Colour (Element::LookAndFeel_E1::defaultMatrixCellOffColor));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }

        int getNumRows()    override { return matrix.getNumRows(); }
        int getNumColumns() override { return matrix.getNumColumns(); }
        
    private:
        MatrixState matrix;
    };
    
    class ConnectionGrid::Sources : public ListBox,
                                    public ListBoxModel
    {
    public:
        Sources (PatchMatrix* m)
            : matrix(m)
        {
            assert (m != nullptr);
            setRowHeight (matrix->getRowThickness());
            setModel (this);
        }
        
        ~Sources() { }
        
        int getNumRows() override { return NUM_DUMMY_NODES; };
        
        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                       bool rowIsSelected) override
        {
            g.setColour (LookAndFeel_E1::widgetBackgroundColor);
            g.fillRect (0, 0, width - 1, height - 1);
            
            g.setColour (Colours::white);
            g.drawText ("Hello There " + String(rowNumber), 0, 0, width - 1, height - 1, Justification::centredLeft);
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
    };
    
    class ConnectionGrid::Controls : public Component { };
    
    class ConnectionGrid::Destinations : public HorizontalListBox,
                                         public ListBoxModel
    {
    public:
        Destinations (PatchMatrix* m)
            : matrix (m)
        {
            assert(m != nullptr);
            setRowHeight (matrix->getColumnThickness());
            setModel (this);
        }
        
        int getNumRows() override { return NUM_DUMMY_NODES; }
        
        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                               bool rowIsSelected) override
        {
            if (rowIsSelected) {
                g.setColour (Colours::black);
                g.fillRect (0, 0, width, height - 1);
            }
            
            ViewHelpers::drawVerticalTextRow ("Hello There", g, width, height, rowIsSelected);
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
    
    ConnectionGrid::ConnectionGrid()
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

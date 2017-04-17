
#include "gui/ConnectionGrid.h"

namespace Element {
    static const int gridPadding = 1;
    
    class ConnectionGrid::PatchMatrix :  public PatchMatrixComponent
    {
    public:
        PatchMatrix() : matrix(8, 8)
        {
            for (int i = 0 ; i < 8; ++i)
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
    
    class ConnectionGrid::ViewPort : public Viewport
    {
    public:
        ViewPort()
        {
            setScrollBarsShown (false, false, true, true);
        }
        
        ~ViewPort() { }
    };
    
    class ConnectionGrid::Quads : public QuadrantLayout {
        
    };
    
    ConnectionGrid::ConnectionGrid()
    {
        view = new ViewPort();
        matrix = new PatchMatrix();
        view->setViewedComponent (matrix, true);
        
        addAndMakeVisible (quads = new Quads());
        quads->setQuadrantComponent (Quads::Q1, view);
        
        auto* c = new Component();
        c->setSize(150, 200);
        quads->setQuadrantComponent (Quads::Q2, c);
        quads->setQuadrantComponent (Quads::Q3, new Component());
        
        c = new Component();
        c->setSize(200, 200);
        quads->setQuadrantComponent (Quads::Q4, c);
    }
    
    ConnectionGrid::~ConnectionGrid()
    {
        view->setViewedComponent (nullptr);
        view = nullptr;
        matrix = nullptr;
        quads = nullptr;
    }
    
    void ConnectionGrid::resized()
    {
        quads->setBounds (getLocalBounds());
    }
}

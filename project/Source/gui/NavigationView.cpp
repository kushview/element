
#include "gui/NavigationView.h"
#include "gui/ViewHelpers.h"

namespace Element {

class NavigationList : public ListBox,
                       public ListBoxModel
{
public:
    enum RootType {
        instrumentsItem = 0,
        pluginsItem,
        sessionItem,
        numRootTypes
    };
    
    NavigationList()
    {
        setModel (this);
        updateContent();
    }
    
    ~NavigationList()
    {
        setModel (nullptr);
    }
    
    static const String& getRootItemName (const int t)
    {
        jassert (t < numRootTypes);
        static const String _names [numRootTypes] = {
            "Instruments", "Plugins", "Session" };
        return _names [t];
    }
    
    int getNumRows() override
    {
        return numRootTypes;
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height,
                           bool selected) override
    {
        const String& name (getRootItemName (row));
        ViewHelpers::drawBasicTextRow(name, g, width, height, selected);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                       Component* existingComponentToUpdate) override
    {
        return ListBoxModel::refreshComponentForRow(rowNumber, isRowSelected, existingComponentToUpdate);
    }

    void listBoxItemClicked (int row, const MouseEvent&) override { }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { }

    void backgroundClicked (const MouseEvent&) override
    {
    
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
    
    }

    void deleteKeyPressed (int lastRowSelected) override
    {
    
    }
    
    void returnKeyPressed (int lastRowSelected) override
    {
    
    }

    void listWasScrolled() override
    {
    
    }

    virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe) override
    {
        return var::null;
    }

    String getTooltipForRow (int row) override
    {
        return "Tip";
    }

};

class NavigationTree
{
public:
    NavigationTree()
    {
    }
    
    ~NavigationTree() { }
};


NavigationView::NavigationView()
{
    addAndMakeVisible (navList = new NavigationList());
    navList->updateContent();
    
}

NavigationView::~NavigationView()
{
}

void NavigationView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel_E1::backgroundColor);
}

void NavigationView::resized()
{
    navList->setBounds (getLocalBounds());
}

}

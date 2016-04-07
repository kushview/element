#include "gui/NavigationView.h"
#include "gui/TreeViewBase.h"
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
    
    NavigationList (NavigationView* v)
        : view (v)
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
        ViewHelpers::drawBasicTextRow (name, g, width, height, selected);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                       Component* existingComponentToUpdate) override
    {
        return ListBoxModel::refreshComponentForRow(rowNumber, isRowSelected, existingComponentToUpdate);
    }

    void listBoxItemClicked (int row, const MouseEvent&) override
    {
        jassert (isPositiveAndBelow (row, (int) numRootTypes));
    }

    void listBoxItemDoubleClicked (int, const MouseEvent&) override { }
    void backgroundClicked (const MouseEvent&) override { }

    void selectedRowsChanged (int lastRowSelected) override
    {
    
    }

    void deleteKeyPressed (int) override { }
    void returnKeyPressed (int) override { }
    void listWasScrolled() override { }

    virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe) override
    {
        return var::null;
    }

    String getTooltipForRow (int row) override
    {
        return "Tip";
    }

private:
    NavigationView* view;
};

class NavigationTree :  public TreePanelBase
{
public:
    NavigationTree (NavigationView* v)
        : TreePanelBase ("navigation"),
          view (v)
    {
        setEmptyTreeMessage ("Empty...");
        setRoot (nullptr);
    }
    
    ~NavigationTree() { }
    
private:
    NavigationView* view;
};


NavigationView::NavigationView()
{
    addAndMakeVisible (navList = new NavigationList (this));
    addAndMakeVisible (navBar = new StretchableLayoutResizerBar (&layout, 1, true));
    addAndMakeVisible (navTree = new NavigationTree (this));
    updateLayout();
    resized();
}

NavigationView::~NavigationView()
{
    navTree = nullptr;
    navBar = nullptr;
    navList = nullptr;
}

void NavigationView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel_E1::backgroundColor);
}

void NavigationView::resized()
{
    Component* comps[] = { navList.get(), navBar.get(), navTree.get() };
    layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), false, true);
}

void NavigationView::updateLayout()
{
    layout.setItemLayout (0, 50.0, 200.0, 100.0);
    layout.setItemLayout (1, 3, 3, 3);
    layout.setItemLayout (2, 50.0, 200.0, 100.0);
}

}

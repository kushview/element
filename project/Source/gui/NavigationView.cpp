
#include "gui/ContentComponent.h"
#include "gui/GuiApp.h"
#include "gui/NavigationView.h"
#include "gui/TreeViewBase.h"
#include "gui/ViewHelpers.h"
#include "session/PluginManager.h"

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
        view->setRootItem (row);
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

class PluginTreeItem : public TreeItemBase
{
public:
    PluginTreeItem (const PluginDescription& d) : desc(d) { }
    ~PluginTreeItem() { }
    bool mightContainSubItems() override { return false; }
    virtual String getRenamingName() const override { return desc.name; }
    virtual String getDisplayName()  const override { return desc.name; }
    virtual void setName (const String&)  override {  }
    virtual bool isMissing() override { return false; }
    virtual Icon getIcon() const override { return Icon(getIcons().document, LookAndFeel_E1::elementBlue); }
    
    var getDragSourceDescription() override
    {
        var dd;
        dd.append ("element://dnd/plugin");
        dd.append (desc.pluginFormatName);
        dd.append (desc.fileOrIdentifier);
        
        return dd;
    }
    
    const PluginDescription desc;
};

class PluginsNavigationItem : public TreeItemBase
{
public:
    PluginsNavigationItem() { }
    ~PluginsNavigationItem() { }
    bool mightContainSubItems() { return true; }
    String getRenamingName() const { return "Plugins"; }
    String getDisplayName() const { return "Plugins"; }
    virtual void setName (const String&) {  }
    virtual bool isMissing() { return false; }
    virtual Icon getIcon() const { return Icon(getIcons().document, LookAndFeel_E1::elementBlue); }
    virtual void addSubItems()
    {
        ContentComponent* cc = getOwnerView()->findParentComponentOfClass<ContentComponent>();
        if (! cc)
            return;
            
        PluginManager& plugins (cc->app().globals().plugins());
        KnownPluginList& known (plugins.availablePlugins());

        for (int i = 0; i < known.getNumTypes(); ++i)
            addSubItem (new PluginTreeItem (*known.getType (i)));
    }
    
    void itemOpennessChanged (bool isOpen)
    {
        if (isOpen)
            addSubItems();
    }
};

class NavigationTree :  public TreePanelBase
{
public:
    NavigationTree (NavigationView* v)
        : TreePanelBase ("navigation"),
          view (v)
    {
        setEmptyTreeMessage ("Empty...");
        auto* item = new PluginsNavigationItem();
        setRoot (item);
        item->setOpen (true);
    }
    
    void rootItemChanged (int item) {
        if (item == rootItem)
            return;
        
        switch (item)
        {
            case NavigationList::pluginsItem:
                setRoot (new PluginsNavigationItem());
                break;
            default:
                setRoot(nullptr);
                break;
        }
        
        rootItem = item;
    }
    
    ~NavigationTree() { }
    
private:
    NavigationView* view;
    int rootItem;
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

void NavigationView::setRootItem (int item)
{
    if (navList->getSelectedRow() != item)
        return navList->selectRow (item);
    navTree->rootItemChanged (item);
}

void NavigationView::updateLayout()
{
    layout.setItemLayout (0, 50.0, 200.0, 100.0);
    layout.setItemLayout (1, 3, 3, 3);
    layout.setItemLayout (2, 50.0, 200.0, 100.0);
}

}

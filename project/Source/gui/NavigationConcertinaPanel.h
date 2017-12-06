
#pragma once

#include "gui/GuiCommon.h"
#include "DataPath.h"



namespace Element {
    
class ElementsNavigationPanel : public SessionGraphsListBox
{
public:
    ElementsNavigationPanel() {
        
    }
    bool keyPressed(const KeyPress& kp) override
    {
        // Allows menu command to pass through, maybe a better way
        // to do this.
        if (kp.getKeyCode() == KeyPress::backspaceKey)
            return Component::keyPressed (kp);
            return ListBox::keyPressed (kp);
    }
    
    void paintListBoxItem (int, Graphics&, int, int, bool) override { }
    
    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        if (ev.mods.isPopupMenu())
            return;
        const Node graph (getSelectedGraph());
        
        if (auto* cc = ViewHelpers::findContentComponent (this))
        {
            auto session (cc->getSession());
            if (row != session->getActiveGraphIndex())
            {
                auto graphs = graph.getValueTree().getParent();
                graphs.setProperty ("active", graphs.indexOf(graph.node()), nullptr);
                if (auto* ec = cc->getAppController().findChild<EngineController>())
                    ec->setRootNode (graph);
                    cc->stabilize();
                    }
        }
    }
    
    Component* refreshComponentForRow (int row, bool isSelected, Component* c) override
    {
        Row* r = (nullptr == c) ? new Row (*this) : dynamic_cast<Row*> (c);
        jassert(r);
        r->updateContent (getGraph(row), row, isSelected);
        return r;
    }
        
    private:
        class Row : public Component,
                            public Label::Listener,
                        public ButtonListener
        {
        public:
            Row (ElementsNavigationPanel& _owner) : owner (_owner)
            {
                addAndMakeVisible (text);
                text.setJustificationType (Justification::centredLeft);
                text.setInterceptsMouseClicks (false, false);
                text.setColour (Label::textWhenEditingColourId, LookAndFeel::textColor.darker());
                text.setColour (Label::backgroundWhenEditingColourId, Colours::black);
                text.addListener (this);
                
                // TODO: conf button
                // addAndMakeVisible (conf);
                conf.setButtonText ("=");
                conf.addListener (this);
            }
            
            ~Row() {
                text.removeListener (this);
            }
            
            void updateContent (const Node& _node, int _row, bool _selected)
            {
                node        = _node;
                row         = _row;
                selected    = _selected;
                if (node.isValid())
                    text.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
            }
            
            void mouseDown (const MouseEvent& ev) override
            {
                owner.selectRow (row);
                
                if (ev.getNumberOfClicks() == 2)
                {
                    if (! text.isBeingEdited())
                    {
                        text.showEditor();
                    }
                }
                else
                {
                    if (! text.isBeingEdited())
                    {
                        owner.listBoxItemClicked (row, ev);
                    }
                }
            }
            
            void paint (Graphics& g) override
            {
                if (text.isBeingEdited())
                    g.fillAll (Colours::black);
                else
                    ViewHelpers::drawBasicTextRow ("", g, getWidth(), getHeight(), selected);
            }
            
            void resized() override
            {
                auto r (getLocalBounds());
                r.removeFromRight (4);
                
                if (conf.isVisible())
                {
                    conf.setBounds (r.removeFromRight (20).withSizeKeepingCentre (16, 16));
                    r.removeFromRight (4);
                }
                
                r.removeFromLeft (10);
                text.setBounds (r);
            }
            
            void labelTextChanged (Label*) override {}
            void editorShown (Label*, TextEditor&) override
            {
                savedText = text.getText();
                text.setInterceptsMouseClicks (true, true);
                repaint();
            }
            
            void editorHidden (Label*, TextEditor&) override
            {
                if (text.getText().isEmpty())
                    text.setText (savedText.isNotEmpty() ? savedText : "Untitled", dontSendNotification);
                text.setInterceptsMouseClicks (false, false);
                repaint (0, 0, 20, getHeight());
            }
            
            void buttonClicked (Button*) override
            {
                owner.selectRow (row);
                ViewHelpers::invokeDirectly (this, Commands::showGraphConfig, false);
            }
            
        private:
            ElementsNavigationPanel& owner;
            Node node;
            Label text;
            String savedText;
            SettingButton conf;
            int row = 0;
            bool selected = false;
        };
};

class DataPathTreeComponent : public Component,
                              public FileBrowserListener,
                              private Timer
{
public:
    DataPathTreeComponent()
        : thread ("EL_DataPath"),
          renameWindow ("Rename","Enter a new file name.", AlertWindow::NoIcon)
    {
        thread.startThread();
        list = new DirectoryContentsList (0, thread);
        list->setDirectory (DataPath::defaultLocation(), true, true);
        addAndMakeVisible (tree = new FileTreeComponent (*list));
        tree->addListener (this);
        
        renameWindow.addButton (TRANS ("Save"),   1, KeyPress (KeyPress::returnKey));
        renameWindow.addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));
        renameWindow.addTextEditor ("filename", "", "Filename");
        
        setSize (300, 800);
    }
    
    ~DataPathTreeComponent()
    {
        tree->removeListener (this);
    }
    
    void resized() override
    {
        tree->setBounds (getLocalBounds().reduced(2));
    }
    
    FileTreeComponent& getFileTreeComponent() {  jassert(tree != nullptr); return *tree; }
    File getSelectedFile() { return getFileTreeComponent().getSelectedFile(); }
    File getDirectory() { return (list) ? list->getDirectory() : File(); }
    void refresh()
    {
        ScopedPointer<XmlElement> state = tree->getOpennessState (true);
        getFileTreeComponent().refresh();
        if (state)
            tree->restoreOpennessState (*state, true);
    }

    virtual void selectionChanged() override { }
    virtual void fileClicked (const File& file, const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && ! file.isDirectory())
            runFileMenu (file);
    }
    
    virtual void fileDoubleClicked (const File& file) override { }
    virtual void browserRootChanged (const File& newFile) override { ignoreUnused (newFile); }
    
private:
    ScopedPointer<FileTreeComponent> tree;
    ScopedPointer<DirectoryContentsList> list;
    TimeSliceThread thread;
    
    AlertWindow renameWindow;
    
    friend class Timer;
    void timerCallback() override { }
    
    void deleteSelectedFile()
    {
        const auto file (getSelectedFile());
        if (! file.existsAsFile())
            return;
        
        #if JUCE_WINDOWS
        String message ("Would you like to move this file to the Recycle Bin?\n");
        #else
        String message ("Would you like to move this file to the trash?\n\n");
        #endif
        
        message << file.getFullPathName();
        if (! AlertWindow::showOkCancelBox (AlertWindow::QuestionIcon, "Delete file", message))
            return;
        
        if (! file.deleteFile()) {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Delete file", "Could not delete");
        } else {
            refresh();
        }
    }
    
    void renameSelectedFile()
    {
        const auto file (getSelectedFile());
        renameWindow.getTextEditor("filename")->setText(getSelectedFile().getFileNameWithoutExtension());
        renameWindow.setAlwaysOnTop (true);
        renameWindow.centreAroundComponent (ViewHelpers::findContentComponent(this),
                                            renameWindow.getWidth(), renameWindow.getHeight());
        renameWindow.enterModalState (true, ModalCallbackFunction::forComponent (renameFileCallback, this),
                                      false);
    }
    
    void closeRenameWindow()
    {
        if (renameWindow.isCurrentlyModal())
            renameWindow.exitModalState (0);
        renameWindow.setVisible (false);
    }
    
    static void renameFileCallback (const int res, DataPathTreeComponent* t) { if (t) t->handleRenameFile (res); }
    void handleRenameFile (const int result)
    {
        const String newBaseName = renameWindow.getTextEditorContents ("filename");
        
        if (result == 0)
        {
        
        }
        else
        {
            auto file = getSelectedFile();
            auto newFile = file.getParentDirectory().getChildFile(newBaseName).withFileExtension(file.getFileExtension());
            if (file.moveFileTo (newFile))
            {
                refresh();
                tree->setSelectedFile (newFile);
            }
            else
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "File rename", "Could not rename this file.");
            }
        }
        
        closeRenameWindow();
    }
    
    static void fileMenuCallback (const int res, DataPathTreeComponent* t) { if (t) t->handleFileMenu (res); }
    void handleFileMenu (const int res)
    {
        switch (res)
        {
            case 0: break;
            case 1: renameSelectedFile(); break;
            case 2: deleteSelectedFile(); break;
        }
    }
    
    
    void runFileMenu (const File& file)
    {
        PopupMenu menu;
        menu.addItem (1, "Rename");
        menu.addItem (2, "Delete");

        auto* callback = ModalCallbackFunction::forComponent (fileMenuCallback, this);
        menu.showMenuAsync (PopupMenu::Options(), callback);
    }
};

class NavigationConcertinaPanel : public ConcertinaPanel
{
public:
    NavigationConcertinaPanel (Globals& g)
        : globals (g), headerHeight (30),
    defaultPanelHeight (80)
    {
        setLookAndFeel (&lookAndFeel);
        updateContent();
    }
    
    ~NavigationConcertinaPanel()
    {
        setLookAndFeel (nullptr);
    }
    
    int getIndexOfPanel (Component* panel)
    {
        if (nullptr == panel)
            return -1;
        for (int i = 0; i < getNumPanels(); ++i)
            if (auto* p = getPanel (i))
                if (p == panel)
                    return i;
        return -1;
    }
    
    template<class T> T* findPanel()
    {
        for (int i = getNumPanels(); --i >= 0;)
            if (T* panel = dynamic_cast<T*> (getPanel (i)))
                return panel;
        return nullptr;
    }
    
    void clearPanels()
    {
        for (int i = 0; i < comps.size(); ++i)
            removePanel (comps.getUnchecked (i));
        comps.clearQuick (true);
    }
    
    void updateContent()
    {
        clearPanels();
        Component* c = nullptr;
        c = new ElementsNavigationPanel();
        auto *h = new ElementsHeader (*this, *c);
        addPanelInternal (-1, c, "Elements", h);
        
       #if EL_USE_DATA_PATH_TREE
        auto * dp = new DataPathTreeComponent();
        dp->getFileTreeComponent().setDragAndDropDescription ("ccNavConcertinaPanel");
        addPanelInternal (-1, dp, "User Data Path", new UserDataPathHeader (*this, *dp));
       #endif
    }
    
    AudioIOPanelView* getAudioIOPanel() { return findPanel<AudioIOPanelView>(); }
    PluginsPanelView* getPluginsPanel() { return findPanel<PluginsPanelView>(); }
    SessionGraphsListBox* getSessionPanel() { return findPanel<SessionGraphsListBox>(); }
    
    const StringArray& getNames() const { return names; }
    const int getHeaderHeight() const { return headerHeight; }
    void setHeaderHeight (const int newHeight)
    {
        jassert (newHeight > 0);
        headerHeight = newHeight;
        updateContent();
    }
    
private:
    typedef Element::LookAndFeel ELF;
    Globals& globals;
    int headerHeight;
    int defaultPanelHeight;
    
    StringArray names;
    OwnedArray<Component> comps;
    void addPanelInternal (const int index, Component* comp, const String& name = String(),
                           Component* header = nullptr)
    {
        jassert(comp);
        if (name.isNotEmpty())
            comp->setName (name);
        addPanel (index, comps.insert(index, comp), false);
        setPanelHeaderSize (comp, headerHeight);
        
        if (nullptr == header)
            header = new Header (*this, *comp);
        setCustomPanelHeader (comp, header, true);
    }
    
    class Header : public Component
    {
    public:
        Header (NavigationConcertinaPanel& _parent, Component& _panel)
        : parent(_parent), panel(_panel)
        {
            setInterceptsMouseClicks (false, true);
            addAndMakeVisible (text);
            text.setColour (Label::textColourId, ELF::textColor);
            text.setInterceptsMouseClicks (false, true);
        }
        
        virtual ~Header() { }
        
        virtual void resized() override
        {
            text.setBounds (4, 1, 100, getHeight() - 2);
        }
        
        virtual void paint (Graphics& g) override
        {
            getLookAndFeel().drawConcertinaPanelHeader (
                                                        g, getLocalBounds(), false, false, parent, panel);
        }
        
    protected:
        NavigationConcertinaPanel& parent;
        Component& panel;
        Label text;
    };
    
    class ElementsHeader : public Header,
    public ButtonListener
    {
    public:
        ElementsHeader (NavigationConcertinaPanel& _parent, Component& _panel)
        : Header (_parent, _panel)
        {
            addAndMakeVisible (addButton);
            addButton.setButtonText ("+");
            addButton.addListener (this);
            setInterceptsMouseClicks (false, true);
        }
        
        void resized() override
        {
            const int padding = 4;
            const int buttonSize = getHeight() - (padding * 2);
            addButton.setBounds (getWidth() - padding - buttonSize,
                                 padding, buttonSize, buttonSize);
        }
        
        void buttonClicked (Button*) override
        {
            if (auto* cc = findParentComponentOfClass<ContentComponent>())
                cc->getGlobals().getCommandManager().invokeDirectly (
                                                                     (int)Commands::sessionAddGraph, true);
                }
        
    private:
        TextButton addButton;
    };
    
    class UserDataPathHeader : public Header,
                               public ButtonListener
    {
    public:
        UserDataPathHeader (NavigationConcertinaPanel& _parent, DataPathTreeComponent& _panel)
            : Header (_parent, _panel), tree (_panel)
        {
            addAndMakeVisible (addButton);
            addButton.setButtonText ("+");
            addButton.addListener (this);
            addButton.setTriggeredOnMouseDown (true);
            setInterceptsMouseClicks (false, true);
        }
        
        void resized() override
        {
            const int padding = 4;
            const int buttonSize = getHeight() - (padding * 2);
            addButton.setBounds (getWidth() - padding - buttonSize,
                                 padding, buttonSize, buttonSize);
        }
        
        void buttonClicked (Button*) override
        {
            PopupMenu menu;
            menu.addItem (1, "Refresh...");
            menu.addSeparator();
            #if JUCE_MAC
            String name = "Show in Finder";
            #else
            String name = "Show in Exlorer"
            #endif
            menu.addItem (2, name);
            const int res = menu.show();
            if (res == 1)
            {
                tree.refresh();
            }
            else if (res == 2)
            {
                tree.getDirectory().revealToUser();
            }
        }
        
    private:
        DataPathTreeComponent& tree;
        TextButton addButton;
    };
    
    class LookAndFeel : public Element::LookAndFeel
    {
    public:
        LookAndFeel() { }
        ~LookAndFeel() { }
        
        void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                        bool isMouseOver, bool isMouseDown,
                                        ConcertinaPanel& panel, Component& comp)
        {
            ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
            g.setColour (Colours::white);
            Rectangle<int> r (area.withTrimmedLeft (20));
            g.drawText (comp.getName(), 20, 0, r.getWidth(), r.getHeight(),
                        Justification::centredLeft);
        }
    } lookAndFeel;
};
}

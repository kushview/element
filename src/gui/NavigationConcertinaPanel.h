/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "gui/GuiCommon.h"
#include "gui/views/ControllerDevicesView.h"
#include "gui/views/ControllerMapsView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/views/NodeEditorContentView.h"
#include "gui/widgets/SessionGraphsListBox.h"
#include "session/Session.h"
#include "session/Node.h"
#include "DataPath.h"
#include "Globals.h"

namespace Element {

#if 1
class ElementsNavigationPanel : public SessionGraphsListBox
{
public:
    ElementsNavigationPanel() { setRowHeight (20); }

    bool keyPressed (const KeyPress& kp) override
    {
        // Allows menu command to pass through, maybe a better way
        // to do this.
        if (kp.getKeyCode() == KeyPress::backspaceKey)
            return Component::keyPressed (kp);
        return ListBox::keyPressed (kp);
    }

    void paintListBoxItem (int, Graphics&, int, int, bool) override {}

    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        const Node graph (getSelectedGraph());

        if (auto* cc = ViewHelpers::findContentComponent (this))
        {
            auto session (cc->getSession());
            if (row != session->getActiveGraphIndex())
            {
                auto graphs = graph.getValueTree().getParent();
                graphs.setProperty (Tags::active, graphs.indexOf (graph.node()), nullptr);
                if (auto* ec = cc->getAppController().findChild<EngineController>())
                    ec->setRootNode (graph);
                cc->stabilize();
            }
        }
    }

    Component* refreshComponentForRow (int row, bool isSelected, Component* c) override
    {
        Row* r = (nullptr == c) ? new Row (*this) : dynamic_cast<Row*> (c);
        jassert (r);
        r->updateContent (getGraph (row), row, isSelected);
        return r;
    }

private:
    class Row : public Component,
                public Label::Listener,
                public Button::Listener
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

        ~Row() noexcept
        {
            text.removeListener (this);
        }

        void updateContent (const Node& _node, int _row, bool _selected)
        {
            node = _node;
            row = _row;
            selected = _selected;
            if (node.isValid())
                text.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
        }

        void mouseDown (const MouseEvent& ev) override
        {
            owner.selectRow (row);

            if (ev.mods.isPopupMenu() && ! text.isBeingEdited())
            {
                owner.listBoxItemClicked (row, ev);

                PopupMenu menu;
                menu.addItem (2, "Rename");
                menu.addItem (6, "Duplicate");
                menu.addItem (3, "Delete");
                menu.addSeparator();
                menu.addItem (4, "Edit...");
                menu.addItem (5, "Settings...");

                const auto res = menu.show();

                switch (res)
                {
                    case 0:
                        break;
                    case 2:
                        text.showEditor();
                        break;
                    case 3:
                        ViewHelpers::invokeDirectly (this, Commands::sessionDeleteGraph, true);
                        break;
                    case 4:
                        ViewHelpers::invokeDirectly (this, Commands::showGraphEditor, true);
                        break;
                    case 5:
                        ViewHelpers::invokeDirectly (this, Commands::showGraphConfig, true);
                        break;
                    case 6:
                        ViewHelpers::invokeDirectly (this, Commands::sessionDuplicateGraph, true);
                        break;
                }
            }
            else if (ev.getNumberOfClicks() == 2)
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

            if (node.isRootGraph())
            {
                const auto h = getHeight();
                const auto w = h;
                const int p = 1 + (int) node.getProperty (Tags::midiProgram, -1);
                if (p > 0)
                    g.drawText (String (p), getWidth() - h, 0, w, h, Justification::centred);
            }
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
#endif

class DataPathTreeComponent : public Component,
                              public FileBrowserListener,
                              private Timer
{
public:
    DataPathTreeComponent()
        : thread ("EL_DataPath"),
          renameWindow ("Rename", "Enter a new file name.", AlertWindow::NoIcon)
    {
        thread.startThread();
        list.reset (new DirectoryContentsList (&filter, thread));
        list->setDirectory (DataPath::defaultLocation(), true, true);

        tree.reset (new FileTreeComponent (*list));
        addAndMakeVisible (tree.get());
        tree->addListener (this);
        tree->setItemHeight (20);
        tree->setIndentSize (10);

        renameWindow.addButton (TRANS ("Save"), 1, KeyPress (KeyPress::returnKey));
        renameWindow.addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));
        renameWindow.addTextEditor ("filename", "", "Filename");

        setSize (300, 800);
    }

    ~DataPathTreeComponent()
    {
        tree->removeListener (this);
        renameWindow.setVisible (false);
        tree.reset();
        list.reset();
    }

    void resized() override
    {
        tree->setBounds (getLocalBounds().reduced (2));
    }

    FileTreeComponent& getFileTreeComponent()
    {
        jassert (tree != nullptr);
        return *tree;
    }
    File getSelectedFile() { return getFileTreeComponent().getSelectedFile(); }
    File getDirectory() { return (list) ? list->getDirectory() : File(); }

    void refresh()
    {
        auto state = tree->getOpennessState (true);
        getFileTreeComponent().refresh();
        if (state)
            tree->restoreOpennessState (*state, true);
    }

    virtual void selectionChanged() override {}

    virtual void fileClicked (const File& file, const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && ! file.isDirectory())
            runFileMenu (file);
    }

    void fileDoubleClicked (const File& file) override
    {
        auto session = ViewHelpers::getSession (this);
        auto* const cc = ViewHelpers::findContentComponent (this);

        if (session && cc)
        {
            if (file.getFileExtension().toLowerCase() == ".elg" || file.getFileExtension().toLowerCase() == ".els")
            {
                cc->post (new OpenSessionMessage (file));
            }
            else if (file.getFileExtension().toLowerCase() == ".elpreset")
            {
                const Node node (Node::parse (file), false);
                if (node.isValid())
                    cc->post (new AddNodeMessage (node, session->getActiveGraph()));
            }
        }
    }

    virtual void browserRootChanged (const File& newFile) override { ignoreUnused (newFile); }

private:
    class Filter : public FileFilter
    {
    public:
        Filter() : FileFilter ("UserDataPath") {}

        bool isFileSuitable (const File& file) const override { return true; }
        bool isDirectorySuitable (const File& file) const override
        {
            if (file.getFileName().toLowerCase() == "cache")
                return false;
            return true;
        }
    } filter;

    std::unique_ptr<FileTreeComponent> tree;
    std::unique_ptr<DirectoryContentsList> list;
    TimeSliceThread thread;

    AlertWindow renameWindow;

    friend class Timer;
    void timerCallback() override {}

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

        if (! file.deleteFile())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Delete file", "Could not delete");
        }
        else
        {
            refresh();
        }
    }

    void renameSelectedFile()
    {
        const auto file (getSelectedFile());
        renameWindow.getTextEditor ("filename")->setText (getSelectedFile().getFileNameWithoutExtension());
        renameWindow.setAlwaysOnTop (true);
        renameWindow.centreAroundComponent (ViewHelpers::findContentComponent (this),
                                            renameWindow.getWidth(),
                                            renameWindow.getHeight());
        renameWindow.enterModalState (true, ModalCallbackFunction::forComponent (renameFileCallback, this), false);
    }

    void closeRenameWindow()
    {
        if (renameWindow.isCurrentlyModal())
            renameWindow.exitModalState (0);
        renameWindow.setVisible (false);
    }

    static void renameFileCallback (const int res, DataPathTreeComponent* t)
    {
        if (t)
            t->handleRenameFile (res);
    }
    void handleRenameFile (const int result)
    {
        const String newBaseName = renameWindow.getTextEditorContents ("filename");

        if (result == 0)
        {
        }
        else
        {
            auto file = getSelectedFile();
            auto newFile = file.getParentDirectory().getChildFile (newBaseName).withFileExtension (file.getFileExtension());
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

    static void fileMenuCallback (const int res, DataPathTreeComponent* t)
    {
        if (t)
            t->handleFileMenu (res);
    }
    void handleFileMenu (const int res)
    {
        switch (res)
        {
            case 0:
                break;
            case 1:
                renameSelectedFile();
                break;
            case 2:
                deleteSelectedFile();
                break;
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
        : globals (g), headerHeight (30), defaultPanelHeight (80)
    {
        setLookAndFeel (&lookAndFeel);
    }

    ~NavigationConcertinaPanel()
    {
        clearPanels();
        setLookAndFeel (nullptr);
    }

    inline Component* findPanelByName (const String& name)
    {
        for (int i = 0; i < getNumPanels(); ++i)
            if (getPanel (i)->getName() == name)
                return getPanel (i);
        return 0;
    }

    inline void saveState (PropertiesFile* props)
    {
        ValueTree state (Tags::state);

        for (int i = 0; i < getNumPanels(); ++i)
        {
            ValueTree item ("item");
            auto* const panel = getPanel (i);
            item.setProperty ("index", i, 0)
                .setProperty ("name", panel->getName(), 0)
                .setProperty ("h", panel->getHeight(), 0);

            if (auto* ned = dynamic_cast<NodeEditorContentView*> (panel))
                item.setProperty ("sticky", ned->isSticky(), nullptr);

            state.addChild (item, -1, 0);
        }

        if (auto xml = state.createXml())
            props->setValue ("ccNavPanel", xml.get());
    }

    void restoreState (PropertiesFile* props)
    {
        if (auto xml = props->getXmlValue ("ccNavPanel"))
        {
            ValueTree state = ValueTree::fromXml (*xml);
            for (int i = 0; i < state.getNumChildren(); ++i)
            {
                auto item (state.getChild (i));
                if (auto* c = findPanelByName (item["name"].toString().trim()))
                {
                    setPanelSize (c, jmax (10, (int) item["h"]), false);
                    if (auto* ned = dynamic_cast<NodeEditorContentView*> (c))
                        ned->setSticky ((bool) item.getProperty ("sticky", ned->isSticky()));
                }
            }
        }
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

    template <class T>
    T* findPanel()
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

#ifdef EL_PRO
        auto* sess = new SessionTreePanel();
        sess->setName ("Session");
        sess->setComponentID ("Session");
        addPanelInternal (-1, sess, "Session", new ElementsHeader (*this, *sess));
#endif

        auto* gv = new GraphSettingsView();
        gv->setName ("Graph");
        gv->setComponentID ("Graph");
        gv->setGraphButtonVisible (false);
        gv->setUpdateOnActiveGraphChange (true);
        gv->setPropertyPanelHeaderVisible (false);
        addPanelInternal (-1, gv, "Graph", nullptr);

        auto* nv = new NodeEditorContentView();
        nv->setName ("Node");
        nv->setComponentID ("Node");
        addPanelInternal (-1, nv, "Node", nullptr);

        auto* mv = new NodeMidiContentView();
        mv->setName ("MIDI");
        mv->setComponentID ("MIDI");
        addPanelInternal (-1, mv, "MIDI", nullptr);

        auto* pv = new PluginsPanelView (ViewHelpers::getGlobals (this)->getPluginManager());
        pv->setName ("Plugins");
        pv->setComponentID ("Plugins");
        addPanelInternal (-1, pv, "Plugins", 0);

        auto* dp = new DataPathTreeComponent();
        dp->setName ("UserDataPath");
        dp->setComponentID ("UserDataPath");
        dp->getFileTreeComponent().setDragAndDropDescription ("ccNavConcertinaPanel");
        addPanelInternal (-1, dp, "User Data Path", new UserDataPathHeader (*this, *dp));
    }

    AudioIOPanelView* getAudioIOPanel() { return findPanel<AudioIOPanelView>(); }
    PluginsPanelView* getPluginsPanel() { return findPanel<PluginsPanelView>(); }
    DataPathTreeComponent* getUserDataPathPanel() { return findPanel<DataPathTreeComponent>(); }
    SessionTreePanel* getSessionPanel() { return findPanel<SessionTreePanel>(); }

    const StringArray& getNames() const { return names; }
    const int getHeaderHeight() const { return headerHeight; }

    void setHeaderHeight (const int newHeight)
    {
        if (newHeight == headerHeight)
            return;
        jassert (newHeight > 0);
        headerHeight = newHeight;
        for (auto* c : comps)
            setPanelHeaderSize (c, headerHeight);
    }

private:
    typedef Element::LookAndFeel ELF;
    Globals& globals;
    int headerHeight;
    int defaultPanelHeight;

    StringArray names;
    OwnedArray<Component> comps;
    void addPanelInternal (const int index, Component* comp, const String& name = String(), Component* header = nullptr)
    {
        jassert (comp);
        if (name.isNotEmpty())
            comp->setName (name);
        addPanel (index, comps.insert (index, comp), false);
        setPanelHeaderSize (comp, headerHeight);

        if (nullptr == header)
            header = new Header (*this, *comp);
        setCustomPanelHeader (comp, header, true);
    }

    class Header : public Component
    {
    public:
        Header (NavigationConcertinaPanel& _parent, Component& _panel)
            : parent (_parent), panel (_panel)
        {
            setInterceptsMouseClicks (false, true);
            addAndMakeVisible (text);
            text.setColour (Label::textColourId, ELF::textColor);
            text.setInterceptsMouseClicks (false, true);
        }

        virtual ~Header() {}

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
                           public Button::Listener
    {
    public:
        ElementsHeader (NavigationConcertinaPanel& _parent, Component& _panel)
            : Header (_parent, _panel)
        {
            addAndMakeVisible (addButton);
            addButton.setIcon (Icon (getIcons().falBars, Colours::white));
            addButton.setTriggeredOnMouseDown (true);
            addButton.addListener (this);
            setInterceptsMouseClicks (false, true);
        }

        void resized() override
        {
            const int padding = 5;
            const int buttonSize = getHeight() - (padding * 2);
            addButton.setBounds (getWidth() - padding - buttonSize,
                                 padding,
                                 buttonSize,
                                 buttonSize);
        }

        static void menuInvocationCallback (int chosenItemID, ElementsHeader* header)
        {
            if (chosenItemID > 0 && header)
                header->menuCallback (chosenItemID);
        }

        void menuCallback (int menuId)
        {
            if (1 == menuId)
            {
                ViewHelpers::invokeDirectly (this, Commands::showSessionConfig, true);
            }
            else if (2 == menuId)
            {
                ViewHelpers::invokeDirectly (this, Commands::sessionAddGraph, false);
                if (auto* p = dynamic_cast<SessionTreePanel*> (&panel))
                    p->refresh();
            }
        }

        void buttonClicked (Button*) override
        {
            PopupMenu menu;
            menu.addItem (1, "Session Settings...");
            menu.addSeparator();
            menu.addItem (2, "Add Graph");
            menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&addButton),
                                ModalCallbackFunction::forComponent (menuInvocationCallback, this));
        }

    private:
        IconButton addButton;
    };

    class UserDataPathHeader : public Header,
                               public Button::Listener
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
                                 padding,
                                 buttonSize,
                                 buttonSize);
        }

        void buttonClicked (Button*) override
        {
            PopupMenu menu;
            menu.addItem (1, "Refresh...");
            menu.addSeparator();
#if JUCE_MAC
            String name = "Show in Finder";
#else
            String name = "Show in Explorer";
#endif
            menu.addItem (2, name);
            const int res = menu.show();
            if (res == 1)
            {
                tree.refresh();
            }
            else if (res == 2)
            {
                File file = tree.getSelectedFile();
                if (! file.exists())
                    file = file.getParentDirectory();
                if (! file.exists())
                    file = tree.getDirectory();
                if (file.exists())
                    file.revealToUser();
            }
        }

    private:
        DataPathTreeComponent& tree;
        TextButton addButton;
    };

    class LookAndFeel : public Element::LookAndFeel
    {
    public:
        LookAndFeel() {}
        ~LookAndFeel() {}

        void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area, bool isMouseOver, bool isMouseDown, ConcertinaPanel& panel, Component& comp)
        {
            ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
            g.setColour (Colours::white);
            Rectangle<int> r (area.withTrimmedLeft (20));
            g.drawText (comp.getName(), 20, 0, r.getWidth(), r.getHeight(), Justification::centredLeft);
        }
    } lookAndFeel;
};
} // namespace Element

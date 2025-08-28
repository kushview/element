// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>
#include <element/ui/content.hpp>
#include <element/nodefactory.hpp>

#include "ui/filetreeview.hpp"
#include "ui/viewhelpers.hpp"
#include "datapath.hpp"
#include "filesystemwatcher.hpp"
#include "messages.hpp"

namespace element {

class DataPathTreeComponent : public juce::Component,
                              public juce::FileBrowserListener,
                              public FileSystemWatcher::Listener
{
public:
    DataPathTreeComponent()
        : thread ("el.thread.DataPathBrowser"),
          renameWindow ("Rename", "Enter a new file name.", AlertWindow::NoIcon)
    {
        thread.startThread();
        list.reset (new DirectoryContentsList (&filter, thread));
        list->setDirectory (DataPath::defaultLocation(), true, true);

#if JUCE_MAC || JUCE_WINDOWS
        watcher.addFolder (list->getDirectory());
        watcher.addListener (this);
#endif

        tree.reset (new FileTreeView (*list));
        addAndMakeVisible (tree.get());
        tree->setDragNativeFiles (true);
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
#if JUCE_MAC || JUCE_WINDOWS
        watcher.removeListener (this);
        watcher.removeAllFolders();
#endif
        list->removeAllChangeListeners();
        tree->removeListener (this);
        renameWindow.setVisible (false);
        tree.reset();
        list.reset();
    }

    void resized() override
    {
        tree->setBounds (getLocalBounds().reduced (2));
    }

    FileTreeView& getFileTree()
    {
        jassert (tree != nullptr);
        return *tree;
    }
    File getSelectedFile() { return getFileTree().getSelectedFile(); }
    File getDirectory() { return (list) ? list->getDirectory() : File(); }

    void refresh()
    {
        auto state = tree->getOpennessState (true);
        getFileTree().refresh();
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
            else if (file.hasFileExtension ("eln;elpreset"))
            {
                const Node node (Node::parse (file), false);
                if (node.isValid())
                    cc->post (new AddNodeMessage (node, session->getActiveGraph()));
            }
            else if (file.hasFileExtension ("lua"))
            {
                Node node (types::Node);
                node.setProperty (tags::name, file.getFileNameWithoutExtension());
                node.setProperty (tags::format, "Element");
                node.setProperty (tags::identifier, EL_NODE_ID_SCRIPT);
                cc->post (new AddNodeMessage (node, session->getActiveGraph(), file));
            }
        }
    }

    virtual void browserRootChanged (const File& newFile) override { ignoreUnused (newFile); }
    void folderChanged (const juce::File&) override { refresh(); }
    void fileChanged (const juce::File&, FileSystemWatcher::FileSystemEvent) override { refresh(); }

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

    std::unique_ptr<FileTreeView> tree;
    std::unique_ptr<DirectoryContentsList> list;
    TimeSliceThread thread;

    AlertWindow renameWindow;
#if JUCE_MAC || JUCE_WINDOWS
    FileSystemWatcher watcher;
#endif
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
        // menu.addItem (1, "Rename");
        menu.addItem (2, "Delete");

        auto* callback = ModalCallbackFunction::forComponent (fileMenuCallback, this);
        menu.showMenuAsync (PopupMenu::Options(), callback);
    }
};
} // namespace element

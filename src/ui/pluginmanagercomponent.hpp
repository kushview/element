// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce.hpp>
#include <element/ui/content.hpp>

#define EL_VIEW_PLUGIN_MANAGER "PluginManagerView"

namespace element {

class PluginManager;
class Settings;

//==============================================================================
/**
     A component displaying a list of plugins, with options to scan for them,
     add, remove and sort them.
 */
class PluginListComponent : public Component,
                            public FileDragAndDropTarget,
                            private ChangeListener,
                            private Button::Listener
{
public:
    //==============================================================================
    /**
     Creates the list component.
     
     For info about the deadMansPedalFile, see the PluginDirectoryScanner constructor.
     The properties file, if supplied, is used to store the user's last search paths.
     */
    PluginListComponent (PluginManager&, PropertiesFile* props = nullptr, bool allowPluginsWhichRequireAsynchronousInstantiation = false);

    /** Destructor. */
    ~PluginListComponent();

    /** Changes the text in the panel's options button. */
    void setOptionsButtonText (const String& newText);

    /** Changes the text in the progress dialog box that is shown when scanning. */
    void setScanDialogText (const String& textForProgressWindowTitle,
                            const String& textForProgressWindowDescription);

    /** Sets how many threads to simultaneously scan for plugins.
     If this is 0, then all scanning happens on the message thread (this is the default when
     allowPluginsWhichRequireAsynchronousInstantiation is false). If
     allowPluginsWhichRequireAsynchronousInstantiation is true then numThreads must not
     be zero (it is one by default). */
    void setNumberOfThreadsForScanning (int numThreads);

    /** Returns the last search path stored in a given properties file for the specified format. */
    static FileSearchPath getLastSearchPath (PropertiesFile&, AudioPluginFormat&);

    /** Stores a search path in a properties file for the given format. */
    static void setLastSearchPath (PropertiesFile&, AudioPluginFormat&, const FileSearchPath&);

    /** Triggers an asynchronous scan for the given format. */
    void scanFor (AudioPluginFormat&);

    /** Scan for all third party types */
    void scanAll();

    /** Returns true if there's currently a scan in progress. */
    bool isScanning() const noexcept;

    /** Removes the plugins currently selected in the table. */
    void removeSelectedPlugins();

    /** Sets a custom table model to be used.
     This will take ownership of the model and delete it when no longer needed.
     */
    void setTableModel (TableListBoxModel* model);

    /** Returns the table used to display the plugin list. */
    TableListBox& getTableListBox() noexcept { return table; }

private:
    PluginManager& plugins;
    AudioPluginFormatManager& formatManager;
    KnownPluginList& list;
    File deadMansPedalFile;
    TableListBox table;
    TextButton optionsButton, closeButton, scanButton;
    PropertiesFile* propertiesToUse;
    String dialogTitle, dialogText;
    bool allowAsync;
    int numThreads;

    class TableModel;
    std::unique_ptr<TableListBoxModel> tableModel;

    class Scanner;
    friend class Scanner;
    friend struct ContainerDeletePolicy<Scanner>;
    std::unique_ptr<Scanner> currentScanner;

    OwnedArray<TextButton> formatButtons;

    void scanFinished (const StringArray&);
    static void optionsMenuStaticCallback (int, PluginListComponent*);
    void optionsMenuCallback (int);
    void updateList();
    void showSelectedFolder();
    bool canShowSelectedFolder() const;
    void removeMissingPlugins();
    void removePluginItem (int index);

    void resized() override;
    bool isInterestedInFileDrag (const StringArray&) override;
    void filesDropped (const StringArray&, int, int) override;
    void buttonClicked (Button*) override;
    void changeListenerCallback (ChangeBroadcaster*) override;

    void scanWithBackgroundScanner();

    void editPluginPath (const String& format);
    void saveListToSettings();
    bool isPluginVersion();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListComponent)
};

class PluginManagerContentView : public ContentView
{
public:
    PluginManagerContentView();
    ~PluginManagerContentView();
    void resized() override;
    void didBecomeActive() override;
    void willBeRemoved() override;

private:
    std::unique_ptr<PluginListComponent> pluginList;
    Settings* settings();
    void saveSettings();
    void restoreSettings();
};
} // namespace element

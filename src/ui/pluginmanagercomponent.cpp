// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/plugins.hpp>
#include <element/context.hpp>
#include <element/settings.hpp>
#include <element/nodefactory.hpp>

#include "ui/guicommon.hpp"
#include "ui/pluginmanagercomponent.hpp"
#include "utils.hpp"

namespace element {

static void removeNonElementPlugins (KnownPluginList& list);

//==============================================================================
class PluginListComponent::Scanner : private Timer,
                                     public PluginScanner::Listener
{
public:
    Scanner (PluginListComponent& plc, AudioPluginFormat& format, PropertiesFile* properties, bool allowPluginsWhichRequireAsynchronousInstantiation, int threads, const String& title, const String& text)
        : owner (plc),
          formatToScan (format),
          propertiesToUse (properties),
          pathChooserWindow (TRANS ("Select folders to scan..."), String(), AlertWindow::NoIcon),
          progressWindow (title, text, AlertWindow::NoIcon),
          progress (0.0),
          numThreads (threads),
          allowAsync (allowPluginsWhichRequireAsynchronousInstantiation),
          finished (false),
          useBackgroundScanner (true)
    {
        jassert (properties != nullptr);

        FileSearchPath path (getLastSearchPath (*properties, formatToScan));
        pathList.setPath (path); // set this so it is ALWAYS up-to-date in ::startScan()
        scanner.setNonOwned (owner.plugins.getBackgroundAudioPluginScanner());

        const StringArray withPath { "VST", "VST3", "CLAP" };
        if (path.getNumPaths() <= 0 && withPath.contains (formatToScan.getName()))
        {
            pathList.setSize (500, 300);
            pathList.setPath (path);

            pathChooserWindow.addCustomComponent (&pathList);
            pathChooserWindow.addButton (TRANS ("Scan"), 1, KeyPress (KeyPress::returnKey));
            pathChooserWindow.addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));

            pathChooserWindow.enterModalState (true,
                                               ModalCallbackFunction::forComponent (startScanCallback, &pathChooserWindow, this),
                                               false);
        }
        else
        {
            startScan();
        }
    }

    Scanner (PluginListComponent& plc,
             PluginManager& plugins,
             const String& title,
             const String& text)
        : owner (plc),
          formatToScan (*plugins.getAudioPluginFormat ("Element")),
          propertiesToUse (nullptr),
          pathChooserWindow (TRANS ("Select folders to scan..."), String(), AlertWindow::NoIcon),
          progressWindow (title, text, AlertWindow::NoIcon),
          progress (0.0),
          numThreads (0),
          allowAsync (false),
          finished (false),
          useBackgroundScanner (true)
    {
        scanner.setNonOwned (owner.plugins.getBackgroundAudioPluginScanner());
        startScan();
    }

    Scanner (PluginListComponent& plc, PluginManager& plugins, const StringArray& formats, const String& title, const String& text)
        : owner (plc),
          formatToScan (*plugins.getAudioPluginFormat ("Element")),
          propertiesToUse (nullptr),
          pathChooserWindow (TRANS ("Select folders to scan..."), String(), AlertWindow::NoIcon),
          progressWindow (title, text, AlertWindow::NoIcon),
          progress (0.0),
          numThreads (0),
          allowAsync (false),
          finished (false),
          useBackgroundScanner (true)
    {
        scanner.setNonOwned (owner.plugins.getBackgroundAudioPluginScanner());
        formatsToScan = formats;
        startScan();
    }

    ~Scanner()
    {
        stopTimer();

        if (scanner)
            scanner->removeListener (this);

        if (pool != nullptr)
        {
            pool->removeAllJobs (true, 60000);
            pool = nullptr;
        }

        scanner.clear();
    }

private:
    PluginListComponent& owner;
    AudioPluginFormat& formatToScan;
    PropertiesFile* propertiesToUse;
    OptionalScopedPointer<PluginScanner> scanner;

    AlertWindow pathChooserWindow, progressWindow;
    FileSearchPathListComponent pathList;
    String pluginBeingScanned;
    double progress;
    [[maybe_unused]] int numThreads;
    [[maybe_unused]] bool allowAsync, finished;
    std::unique_ptr<ThreadPool> pool;
    [[maybe_unused]] bool useBackgroundScanner = false;
    StringArray formatsToScan;

    static void startScanCallback (int result, AlertWindow* alert, Scanner* scanner)
    {
        if (alert != nullptr && scanner != nullptr)
        {
            if (result != 0)
                scanner->warnUserAboutStupidPaths();
            else
                scanner->finishedScan();
        }
    }

    // Try to dissuade people from to scanning their entire C: drive, or other system folders.
    void warnUserAboutStupidPaths()
    {
        for (int i = 0; i < pathList.getPath().getNumPaths(); ++i)
        {
            const File f (pathList.getPath()[i]);

            if (isStupidPath (f))
            {
                AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
                                              TRANS ("Plugin Scanning"),
                                              TRANS ("If you choose to scan folders that contain non-plugin files, "
                                                     "then scanning may take a long time, and can cause crashes when "
                                                     "attempting to load unsuitable files.")
                                                  + newLine
                                                  + TRANS ("Are you sure you want to scan the folder \"XYZ\"?")
                                                        .replace ("XYZ", f.getFullPathName()),
                                              TRANS ("Scan"),
                                              String(),
                                              nullptr,
                                              ModalCallbackFunction::create (warnAboutStupidPathsCallback, this));
                return;
            }
        }

        owner.setLastSearchPath (*propertiesToUse, formatToScan, pathList.getPath());
        startScan();
    }

    static bool isStupidPath (const File& f)
    {
        Array<File> roots;
        File::findFileSystemRoots (roots);

        if (roots.contains (f))
            return true;

        File::SpecialLocationType pathsThatWouldBeStupidToScan[] = { File::globalApplicationsDirectory,
                                                                     File::userHomeDirectory,
                                                                     File::userDocumentsDirectory,
                                                                     File::userDesktopDirectory,
                                                                     File::tempDirectory,
                                                                     File::userMusicDirectory,
                                                                     File::userMoviesDirectory,
                                                                     File::userPicturesDirectory };

        for (int i = 0; i < numElementsInArray (pathsThatWouldBeStupidToScan); ++i)
        {
            const File sillyFolder (File::getSpecialLocation (pathsThatWouldBeStupidToScan[i]));

            if (f == sillyFolder || sillyFolder.isAChildOf (f))
                return true;
        }

        return false;
    }

    static void warnAboutStupidPathsCallback (int result, Scanner* scanner)
    {
        if (result != 0)
            scanner->startScan();
        else
            scanner->finishedScan();
    }

    void startScan()
    {
        pathChooserWindow.setVisible (false);

        if (propertiesToUse != nullptr)
        {
            setLastSearchPath (*propertiesToUse, formatToScan, pathList.getPath());
            propertiesToUse->saveIfNeeded();
        }

        progressWindow.addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));
        progressWindow.getButton (TRANS ("Cancel"))->onClick = [this] {
            scanner->cancel();
        };
        progressWindow.addProgressBarComponent (progress);
        progress = 0.0;
        scanner->addListener (this);
        finished = false;

        startTimer (20);
        progressWindow.setVisible (true);

        if (! scanner->isScanning())
        {
            if (formatsToScan.size() > 0)
                scanner->scanForAudioPlugins (formatsToScan);
            else
                scanner->scanForAudioPlugins (formatToScan.getName());
        }
        else
        {
            finishedScan();
        }
    }

    void finishedScan()
    {
        progressWindow.getButton (TRANS ("Cancel"))->onClick = nullptr;
        stopTimer();

        if (scanner)
        {
            scanner->removeListener (this);
            scanner.reset();
        }

        progressWindow.exitModalState();
        progressWindow.setVisible (false);
        progressWindow.removeFromDesktop();

        MessageManager::getInstance()->runDispatchLoopUntil (14);
        StringArray failedFiles; // TODO

        owner.scanFinished (failedFiles);
    }

    void timerCallback() override
    {
        if (! finished)
        {
            progressWindow.setMessage (pluginBeingScanned);
        }
    }

    void audioPluginScanFinished() override
    {
        finished = true;
        finishedScan();
    }

    void audioPluginScanStarted (const String& pluginName) override
    {
        pluginBeingScanned = File::createFileWithoutCheckingPath (pluginName.trim())
                                 .getFileName();
        MessageManager::getInstance()->runDispatchLoopUntil (14);
    }

    void audioPluginScanProgress (const float reportedProgress) override
    {
        this->progress = reportedProgress;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scanner)
};

//==============================================================================
class PluginListComponent::TableModel : public TableListBoxModel
{
public:
    TableModel (PluginListComponent& c, KnownPluginList& l) : owner (c), list (l) 
    {
        refreshCache();
    }

    void refreshCache()
    {
        cachedTypes = list.getTypes();
    }

    const Array<PluginDescription>& getCachedTypes() const
    {
        return cachedTypes;
    }

    int getNumRows() override
    {
        return cachedTypes.size() + list.getBlacklistedFiles().size();
    }

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        ViewHelpers::drawBasicTextRow (String(), g, width, height, rowIsSelected);
    }

    enum
    {
        nameCol = 1,
        typeCol = 2,
        categoryCol = 3,
        manufacturerCol = 4,
        descCol = 5,
        ioCol = 6
    };

    String getPluginIO (const PluginDescription& desc)
    {
        String result (desc.numInputChannels);
        result << " / " << desc.numOutputChannels;
        return result;
    }

    void paintCell (Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected) override
    {
        String text;
        bool isBlacklisted = row >= cachedTypes.size();

        if (isBlacklisted)
        {
            const auto blacklisted = list.getBlacklistedFiles();
            const int blacklistIndex = row - cachedTypes.size();
            if (!isPositiveAndBelow (blacklistIndex, blacklisted.size()))
                return;
            
            if (columnId == nameCol)
                text = blacklisted[blacklistIndex];
            else if (columnId == descCol)
                text = TRANS ("Deactivated after failing to initialise correctly");
        }
        else if (isPositiveAndBelow (row, cachedTypes.size()))
        {
            const auto& desc = cachedTypes.getReference (row);
            switch (columnId)
            {
                case nameCol:
                    text = desc.name;
                    break;
                case typeCol:
                    text = desc.pluginFormatName;
                    break;
                case categoryCol:
                    text = desc.category.isNotEmpty() ? desc.category : "-";
                    break;
                case manufacturerCol:
                    text = desc.manufacturerName;
                    break;
                case descCol:
                    text = getPluginDescription (desc);
                    break;
                case ioCol:
                    text = getPluginIO (desc);
                    break;
                default:
                    jassertfalse;
                    break;
            }
        }

        if (text.isNotEmpty())
        {
            g.setColour (isBlacklisted         ? Colours::red
                         : columnId == nameCol ? Colors::textColor
                                               : Colours::grey);
            g.setFont (Font (FontOptions (height * 0.7f)));
            g.drawFittedText (text, 4, 0, width - 6, height, Justification::centredLeft, 1, 0.9f);
        }
    }

    void cellPopup (const int result)
    {
        switch (result)
        {
            case 0:
                break;
            case 1:
                removeNonElementPlugins (owner.list);
                owner.saveListToSettings();
                refreshCache();
                break;
            case 2:
                owner.removeSelectedPlugins();
                break;
            default: {
            }
            break;
        }
    }

    void cellClicked (int row, int col, const MouseEvent& ev) override
    {
        if (ev.mods.isPopupMenu())
        {
            PopupMenu menu;
            menu.addItem (1, "Clear list");
            menu.addItem (2, "Remove selected");
            cellPopup (menu.show());
        }
    }

    void deleteKeyPressed (int) override
    {
        owner.removeSelectedPlugins();
    }

    void sortOrderChanged (int newSortColumnId, bool isForwards) override
    {
        switch (newSortColumnId)
        {
            case nameCol:
                list.sort (KnownPluginList::sortAlphabetically, isForwards);
                break;
            case typeCol:
                list.sort (KnownPluginList::sortByFormat, isForwards);
                break;
            case categoryCol:
                list.sort (KnownPluginList::sortByCategory, isForwards);
                break;
            case manufacturerCol:
                list.sort (KnownPluginList::sortByManufacturer, isForwards);
                break;

            case ioCol:
            case descCol:
                break;

            default:
                jassertfalse;
                break;
        }
        
        refreshCache();
    }

    static String getPluginDescription (const PluginDescription& desc)
    {
        StringArray items;

        if (desc.descriptiveName != desc.name)
            items.add (desc.descriptiveName);

        items.add (desc.version);

        items.removeEmptyStrings();
        return items.joinIntoString (" - ");
    }

    PluginListComponent& owner;
    KnownPluginList& list;
    Array<PluginDescription> cachedTypes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableModel)
};

//==============================================================================
PluginListComponent::PluginListComponent (PluginManager& p, PropertiesFile* props, bool allowPluginsWhichRequireAsynchronousInstantiation)
    : plugins (p),
      formatManager (p.getAudioPluginFormats()),
      list (p.getKnownPlugins()),
      deadMansPedalFile (p.getDeadAudioPluginsFile()),
      optionsButton ("Options..."),
      propertiesToUse (props),
      allowAsync (allowPluginsWhichRequireAsynchronousInstantiation),
      numThreads (allowAsync ? 1 : 0)
{
    tableModel.reset (new TableModel (*this, list));

    TableHeaderComponent& header = table.getHeader();

    header.addColumn (TRANS ("Name"), TableModel::nameCol, 200, 100, 700, TableHeaderComponent::defaultFlags | TableHeaderComponent::sortedForwards);
    header.addColumn (TRANS ("Format"), TableModel::typeCol, 80, 80, 80, TableHeaderComponent::notResizable);
    header.addColumn (TRANS ("Category"), TableModel::categoryCol, 100, 100, 200);
    header.addColumn (TRANS ("Manufacturer"), TableModel::manufacturerCol, 200, 100, 300);
    header.addColumn (TRANS ("Description"), TableModel::descCol, 300, 100, 500, TableHeaderComponent::notSortable);
    header.addColumn (TRANS ("IO"), TableModel::ioCol, 80, 80, 80, TableHeaderComponent::notSortable);
    table.setHeaderHeight (22);
    table.setRowHeight (20);
    table.setModel (tableModel.get());
    table.setMultipleSelectionEnabled (true);
    addAndMakeVisible (table);

    addAndMakeVisible (optionsButton);
    optionsButton.addListener (this);
    optionsButton.setTriggeredOnMouseDown (true);

    addAndMakeVisible (closeButton);
    closeButton.setButtonText ("Close");
    closeButton.addListener (this);

    addAndMakeVisible (scanButton);
    scanButton.setButtonText ("Scan");
    scanButton.addListener (this);

    setSize (400, 600);
    list.addChangeListener (this);

    updateList();
    table.getHeader().reSortTable();

    if (plugins.isScanningAudioPlugins())
    {
        plugins.addChangeListener (this);
        scanWithBackgroundScanner();
    }
}

PluginListComponent::~PluginListComponent()
{
    list.removeChangeListener (this);
}

void PluginListComponent::setOptionsButtonText (const String& newText)
{
    optionsButton.setButtonText (newText);
    resized();
}

void PluginListComponent::setScanDialogText (const String& title, const String& content)
{
    dialogTitle = title;
    dialogText = content;
}

void PluginListComponent::setNumberOfThreadsForScanning (int num)
{
    numThreads = num;
}

void PluginListComponent::resized()
{
    Rectangle<int> r (getLocalBounds().reduced (2));
    auto r2 = r.removeFromTop (24).reduced (0, 2);

    scanButton.changeWidthToFitText (r2.getHeight());
    scanButton.setBounds (r2.removeFromLeft (scanButton.getWidth()));
    r2.removeFromLeft (4);
    optionsButton.setBounds (r2);
    optionsButton.changeWidthToFitText (r2.getHeight());
    r2.removeFromRight (2);
    closeButton.changeWidthToFitText (r2.getHeight());
    closeButton.setBounds (r2.removeFromRight (closeButton.getWidth()));
    r.removeFromTop (3);
    r.removeFromBottom (3);
    table.setBounds (r);
}

void PluginListComponent::changeListenerCallback (ChangeBroadcaster* cb)
{
    table.getHeader().reSortTable();
    updateList();

    if (cb == &plugins)
    {
        currentScanner = nullptr;
    }
}

void PluginListComponent::updateList()
{
    if (auto* model = dynamic_cast<TableModel*>(tableModel.get()))
        model->refreshCache();
    table.updateContent();
    table.repaint();
}

void PluginListComponent::removeSelectedPlugins()
{
    const SparseSet<int> selected (table.getSelectedRows());

    for (int i = table.getNumRows(); --i >= 0;)
        if (selected.contains (i))
            removePluginItem (i);
}

void PluginListComponent::setTableModel (TableListBoxModel* model)
{
    table.setModel (nullptr);
    tableModel.reset (model);
    table.setModel (tableModel.get());

    table.getHeader().reSortTable();
    table.updateContent();
    table.repaint();
}

bool PluginListComponent::canShowSelectedFolder() const
{
    if (auto* model = dynamic_cast<TableModel*>(tableModel.get()))
    {
        const auto& types = model->getCachedTypes();
        if (isPositiveAndBelow (table.getSelectedRow(), types.size()))
            return File::createFileWithoutCheckingPath (
                       types.getReference (table.getSelectedRow()).fileOrIdentifier)
                .exists();
    }

    return false;
}

void PluginListComponent::showSelectedFolder()
{
    if (! canShowSelectedFolder())
        return;

    if (auto* model = dynamic_cast<TableModel*>(tableModel.get()))
    {
        const auto& types = model->getCachedTypes();
        if (isPositiveAndBelow (table.getSelectedRow(), types.size()))
        {
            const auto& type = types.getReference (table.getSelectedRow());
            File (type.fileOrIdentifier).getParentDirectory().startAsProcess();
        }
    }
}

void PluginListComponent::removeMissingPlugins()
{
    if (auto* model = dynamic_cast<TableModel*>(tableModel.get()))
    {
        const auto& types = model->getCachedTypes();
        for (int i = types.size(); --i >= 0;)
            if (! formatManager.doesPluginStillExist (types.getReference (i)))
                list.removeType (types.getReference (i));
    }
}

void PluginListComponent::removePluginItem (int index)
{
    if (auto* model = dynamic_cast<TableModel*>(tableModel.get()))
    {
        const auto& types = model->getCachedTypes();
        if (! isPositiveAndBelow (index, types.size()))
        {
            const auto blacklisted = list.getBlacklistedFiles();
            const int blacklistIndex = index - types.size();
            if (isPositiveAndBelow (blacklistIndex, blacklisted.size()))
                list.removeFromBlacklist (blacklisted[blacklistIndex]);
        }
        else
        {
            list.removeType (index);
        }
        model->refreshCache();
    }
}

void PluginListComponent::optionsMenuStaticCallback (int result, PluginListComponent* pluginList)
{
    if (pluginList != nullptr)
        pluginList->optionsMenuCallback (result);
}

static void removeNonElementPlugins (KnownPluginList& list)
{
    const auto types = list.getTypes();
    for (int i = types.size(); --i >= 0;)
        if (types.getReference (i).pluginFormatName != "Element")
            list.removeType (types.getReference (i));
}

static void saveSettings (Component* c, const bool saveUserPlugins = true)
{
    if (auto* g = ViewHelpers::getGlobals (c))
    {
        if (saveUserPlugins)
            g->plugins().saveUserPlugins (g->settings());
        g->settings().saveIfNeeded();
    }
}

void PluginListComponent::editPluginPath (const String& f)
{
    jassert (propertiesToUse);

    FileSearchPathListComponent pathList;
    pathList.setSize (400, 260);

    if (auto* const fmt = plugins.getAudioPluginFormat (f))
    {
        pathList.setPath (getLastSearchPath (*propertiesToUse, *fmt));
    }
    else if (auto format = plugins.getProvider (f))
    {
        pathList.setPath (getLastSearchPath (*propertiesToUse, *format));
    }

    String message (f);
    message << TRANS (" plugin path");
    AlertWindow window (message, String(), AlertWindow::NoIcon);

    window.addCustomComponent (&pathList);
    window.addButton (TRANS ("Save"), 1, KeyPress (KeyPress::returnKey));
    window.addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));

    const int result = window.runModalLoop();

    if (1 == result)
    {
        if (auto* const fmt = plugins.getAudioPluginFormat (f))
            setLastSearchPath (*propertiesToUse, *fmt, pathList.getPath());
        else if (auto format = plugins.getProvider (f))
            setLastSearchPath (*propertiesToUse, *format, pathList.getPath());
    }
}

void PluginListComponent::optionsMenuCallback (int result)
{
    switch (result)
    {
        case 0:
            break;
        case 1:
            removeNonElementPlugins (list);
            saveSettings (this);
            break;
        case 2:
            removeSelectedPlugins();
            saveSettings (this);
            break;
        case 3:
            showSelectedFolder();
            break;
        case 4:
            removeMissingPlugins();
            saveSettings (this);
            break;

        case 99:
            editPluginPath ("CLAP");
            saveSettings (this, true);
            break;
        case 100:
            editPluginPath ("VST");
            saveSettings (this, true);
            break;
        case 101:
            editPluginPath ("VST3");
            saveSettings (this, true);
            break;

        case 8: {
            if (auto* world = ViewHelpers::getGlobals (this))
                plugins.saveUserPlugins (world->settings());

            currentScanner.reset (new Scanner (*this,
                                               plugins,
                                               StringArray ("CLAP"),
                                               TRANS ("Scanning for CLAP plug-ins..."),
                                               TRANS ("Searching for all possible plug-in files...")));

            break;
        }

        case 9: {
            if (auto* world = ViewHelpers::getGlobals (this))
                plugins.saveUserPlugins (world->settings());

            currentScanner.reset (new Scanner (*this,
                                               plugins,
                                               StringArray ("LV2"),
                                               TRANS ("Scanning for LV2 plug-ins..."),
                                               TRANS ("Searching for all possible plug-in URIs...")));

            break;
        }

        default: {
            if (AudioPluginFormat* format = formatManager.getFormat (result - 10))
                scanFor (*format);
            break;
        }
    }
}

void PluginListComponent::buttonClicked (Button* button)
{
    if (button == &optionsButton)
    {
        PopupMenu menu;

        menu.addItem (1, TRANS ("Clear list"));
        menu.addSeparator();

        PopupMenu paths;
        if (plugins.isAudioPluginFormatSupported ("CLAP"))
            paths.addItem (99, TRANS ("CLAP Path"));
        if (plugins.isAudioPluginFormatSupported ("VST"))
            paths.addItem (100, TRANS ("VST Path"));
        if (plugins.isAudioPluginFormatSupported ("VST3"))
            paths.addItem (101, TRANS ("VST3 Path"));
        if (paths.getNumItems() > 0)
        {
            menu.addSubMenu ("Search Paths", paths);
            menu.addSeparator();
        }

        menu.addItem (2, TRANS ("Remove selected plug-in from list"), table.getNumSelectedRows() > 0);
        menu.addItem (3, TRANS ("Show folder containing selected plug-in"), canShowSelectedFolder());
        menu.addItem (4, TRANS ("Remove any plug-ins whose files no longer exist"));
        menu.addSeparator();
        menu.addItem (8, "Scan for new or updated CLAP plugins");
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* const format = formatManager.getFormat (i);
            if (format->canScanForPlugins())
                menu.addItem (10 + i, "Scan for new or updated " + format->getName() + " plugins");
        }

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&optionsButton),
                            ModalCallbackFunction::forComponent (optionsMenuStaticCallback, this));
    }
    else if (button == &closeButton)
    {
        ViewHelpers::invokeDirectly (this, Commands::showLastContentView, true);
        saveListToSettings();
    }
    else if (button == &scanButton)
    {
        scanAll();
    }
}

bool PluginListComponent::isInterestedInFileDrag (const StringArray& /*files*/)
{
    return true;
}

void PluginListComponent::filesDropped (const StringArray& files, int, int)
{
    if (currentScanner || plugins.isScanningAudioPlugins())
        return;
    OwnedArray<PluginDescription> typesFound;
    list.scanAndAddDragAndDroppedFiles (formatManager, files, typesFound);
}

FileSearchPath PluginListComponent::getLastSearchPath (PropertiesFile& properties, AudioPluginFormat& format)
{
    return FileSearchPath (properties.getValue (Settings::lastPluginScanPathPrefix + format.getName(),
                                                format.getDefaultLocationsToSearch().toString()));
}

FileSearchPath PluginListComponent::getLastSearchPath (PropertiesFile& properties, NodeProvider& format)
{
    return FileSearchPath (properties.getValue (Settings::lastPluginScanPathPrefix + format.format(),
                                                format.defaultSearchPath().toString()));
}

void PluginListComponent::setLastSearchPath (PropertiesFile& properties, AudioPluginFormat& format, const FileSearchPath& newPath)
{
    properties.setValue (Settings::lastPluginScanPathPrefix + format.getName(),
                         newPath.toString());
    properties.saveIfNeeded();
}

void PluginListComponent::setLastSearchPath (PropertiesFile& properties, NodeProvider& format, const FileSearchPath& newPath)
{
    properties.setValue (Settings::lastPluginScanPathPrefix + format.format(),
                         newPath.toString());
    properties.saveIfNeeded();
}

// MARK: Scanner
static StringArray scanAllFormats (PluginManager& p)
{
    const StringArray supported (Util::compiledAudioPluginFormats());
    StringArray filtered;
    filtered.add ("LV2");
    filtered.add ("CLAP");
    for (int i = 0; i < p.getAudioPluginFormats().getNumFormats(); ++i)
    {
        auto* f = p.getAudioPluginFormats().getFormat (i);
        if (supported.contains (f->getName()))
            filtered.add (f->getName());
    }

    filtered.removeDuplicates (false);
    filtered.removeEmptyStrings();
    return filtered;
}

void PluginListComponent::scanAll()
{
    plugins.scanInternalPlugins();

    if (auto* world = ViewHelpers::getGlobals (this))
        plugins.saveUserPlugins (world->settings());
    currentScanner.reset (new Scanner (*this,
                                       plugins,
                                       scanAllFormats (plugins),
                                       TRANS ("Scanning for plug-ins..."),
                                       TRANS ("Searching for all possible plug-in files...")));
}

void PluginListComponent::scanFor (AudioPluginFormat& format)
{
    if (format.getName() == "Element")
    {
        return;
    }

    if (auto* world = ViewHelpers::getGlobals (this))
        plugins.saveUserPlugins (world->settings());
    // clang-format off
    currentScanner.reset (new Scanner (*this, format, propertiesToUse, allowAsync, numThreads, 
        dialogTitle.isNotEmpty() ? dialogTitle : TRANS ("Scanning for plug-ins..."), dialogText.isNotEmpty() 
                                    ? dialogText  : TRANS ("Searching for all possible plug-in files...")));
    // clang-format on
}

bool PluginListComponent::isScanning() const noexcept
{
    return (currentScanner != nullptr || plugins.isScanningAudioPlugins());
}

void PluginListComponent::saveListToSettings()
{
    if (auto* world = ViewHelpers::getGlobals (this))
        plugins.saveUserPlugins (world->settings());
}

void PluginListComponent::scanFinished (const StringArray& failedFiles)
{
    StringArray shortNames;

    for (int i = 0; i < failedFiles.size(); ++i)
        shortNames.add (File::createFileWithoutCheckingPath (failedFiles[i]).getFileName());

    currentScanner = nullptr; // mustn't delete this before using the failed files array

    if (shortNames.size() > 0)
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                          TRANS ("Scan complete"),
                                          TRANS ("Note that the following files appeared to be plugin files, but failed to load correctly")
                                              + ":\n\n"
                                              + shortNames.joinIntoString (", "));
}

PluginManagerContentView::PluginManagerContentView()
{
    setName (EL_VIEW_PLUGIN_MANAGER);
    setEscapeTriggersClose (true);
}

PluginManagerContentView::~PluginManagerContentView() {}

Settings* PluginManagerContentView::settings()
{
    if (auto world = ViewHelpers::getGlobals (this))
        return &world->settings();
    return nullptr;
}

void PluginManagerContentView::didBecomeActive()
{
    jassert (ViewHelpers::getGlobals (this));
    auto& world (*ViewHelpers::getGlobals (this));
    auto& plugins (world.plugins());
    auto& settings (world.settings());

    if (pluginList)
        pluginList.reset();

    pluginList = std::make_unique<element::PluginListComponent> (plugins, settings.getUserSettings());
    addAndMakeVisible (pluginList.get());
    resized();

    restoreSettings();
    grabKeyboardFocus();
}

void PluginManagerContentView::willBeRemoved()
{
    saveSettings();
}

void PluginManagerContentView::resized()
{
    if (pluginList)
        pluginList->setBounds (getLocalBounds().reduced (2));
}

void PluginManagerContentView::saveSettings()
{
    if (nullptr == pluginList)
        return;
    auto& header = pluginList->getTableListBox().getHeader();
    if (auto s = settings())
        if (auto props = s->getUserSettings())
            props->setValue (Settings::pluginListHeaderKey, header.toString());
}

void PluginManagerContentView::restoreSettings()
{
    if (nullptr == pluginList)
        return;
    auto& header = pluginList->getTableListBox().getHeader();
    if (auto s = settings())
        if (auto props = s->getUserSettings())
            header.restoreFromString (props->getValue (Settings::pluginListHeaderKey));
}

void PluginListComponent::scanWithBackgroundScanner()
{
    if (currentScanner)
    {
        currentScanner = nullptr;
    }
    currentScanner.reset (new Scanner (*this, plugins, "Scanning for plugins", "Looking for new or updated plugins"));
}

bool PluginListComponent::isPluginVersion()
{
    if (auto* cc = ViewHelpers::findContentComponent (this))
        return cc->services().getRunMode() == RunMode::Plugin;
    return false;
}

} // namespace element

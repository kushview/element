
#include "gui/GuiCommon.h"
#include "gui/PluginManagerComponent.h"
#include "session/PluginManager.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {

class PluginListComponent::TableModel  : public TableListBoxModel
{
public:
    TableModel (PluginListComponent& c, KnownPluginList& l)  : owner (c), list (l) {}
    
    int getNumRows() override
    {
        return list.getNumTypes() + list.getBlacklistedFiles().size();
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
        descCol = 5
    };
    
    void paintCell (Graphics& g, int row, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        String text;
        bool isBlacklisted = row >= list.getNumTypes();
        
        if (isBlacklisted)
        {
            if (columnId == nameCol)
                text = list.getBlacklistedFiles() [row - list.getNumTypes()];
            else if (columnId == descCol)
                text = TRANS("Deactivated after failing to initialise correctly");
        }
        else if (const PluginDescription* const desc = list.getType (row))
        {
            switch (columnId)
            {
                case nameCol:         text = desc->name; break;
                case typeCol:         text = desc->pluginFormatName; break;
                case categoryCol:     text = desc->category.isNotEmpty() ? desc->category : "-"; break;
                case manufacturerCol: text = desc->manufacturerName; break;
                case descCol:         text = getPluginDescription (*desc); break;
                    
                default: jassertfalse; break;
            }
        }
        
        if (text.isNotEmpty())
        {
            g.setColour (isBlacklisted ? Colours::red
                        : columnId == nameCol ? LookAndFeel_KV1::textColor
                        : Colours::grey);
            g.setFont (Font (height * 0.7f));
            g.drawFittedText (text, 4, 0, width - 6, height, Justification::centredLeft, 1, 0.9f);
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
            case nameCol:         list.sort (KnownPluginList::sortAlphabetically, isForwards); break;
            case typeCol:         list.sort (KnownPluginList::sortByFormat, isForwards); break;
            case categoryCol:     list.sort (KnownPluginList::sortByCategory, isForwards); break;
            case manufacturerCol: list.sort (KnownPluginList::sortByManufacturer, isForwards); break;
            case descCol:         break;
                
            default: jassertfalse; break;
        }
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
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableModel)
};

// MARK: Plugin LIst Component

PluginListComponent::PluginListComponent (PluginManager& p, PropertiesFile* props,
                                          bool allowPluginsWhichRequireAsynchronousInstantiation)
    : plugins(p),
      formatManager (p.getAudioPluginFormats()),
      list (p.availablePlugins()),
      deadMansPedalFile (p.getDeadAudioPluginsFile()),
      optionsButton ("Options..."),
      propertiesToUse (props),
      allowAsync (allowPluginsWhichRequireAsynchronousInstantiation),
      numThreads (allowAsync ? 1 : 0)
{
    tableModel = new TableModel (*this, list);
    
    TableHeaderComponent& header = table.getHeader();
    
    header.addColumn (TRANS("Name"),         TableModel::nameCol,         200, 100, 700, TableHeaderComponent::defaultFlags | TableHeaderComponent::sortedForwards);
    header.addColumn (TRANS("Format"),       TableModel::typeCol,         80, 80, 80,    TableHeaderComponent::notResizable);
    header.addColumn (TRANS("Category"),     TableModel::categoryCol,     100, 100, 200);
    header.addColumn (TRANS("Manufacturer"), TableModel::manufacturerCol, 200, 100, 300);
    header.addColumn (TRANS("Description"),  TableModel::descCol,         300, 100, 500, TableHeaderComponent::notSortable);
    
    table.setHeaderHeight (22);
    table.setRowHeight (20);
    table.setModel (tableModel);
    table.setMultipleSelectionEnabled (true);
    addAndMakeVisible (table);
    
    addAndMakeVisible (optionsButton);
    optionsButton.addListener (this);
    optionsButton.setTriggeredOnMouseDown (true);
    
    addAndMakeVisible (closeButton);
    closeButton.setButtonText ("Close");
    closeButton.addListener (this);
    
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        const auto name = formatManager.getFormat(i)->getName();
        const bool canScan = formatManager.getFormat(i)->canScanForPlugins();
        if (name == "Element" || name == "Internal" || !canScan)
            continue;
        
        auto* button = formatButtons.add (new TextButton (name));
        button->setButtonText ("Scan " + name);
        button->addListener (this);
        addAndMakeVisible (button);
    }
    
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
    
    for (auto* b : formatButtons)
    {
        b->changeWidthToFitText (r2.getHeight());
        b->setBounds (r2.removeFromLeft (b->getWidth()));
        r2.removeFromLeft (4);
    }
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
    
    if (cb == &plugins) {
        currentScanner = nullptr;
    }
}

void PluginListComponent::updateList()
{
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
    tableModel = model;
    table.setModel (tableModel);
    
    table.getHeader().reSortTable();
    table.updateContent();
    table.repaint();
}

bool PluginListComponent::canShowSelectedFolder() const
{
    if (const PluginDescription* const desc = list.getType (table.getSelectedRow()))
        return File::createFileWithoutCheckingPath (desc->fileOrIdentifier).exists();
    
    return false;
}

void PluginListComponent::showSelectedFolder()
{
    if (canShowSelectedFolder())
        if (const PluginDescription* const desc = list.getType (table.getSelectedRow()))
            File (desc->fileOrIdentifier).getParentDirectory().startAsProcess();
}

void PluginListComponent::removeMissingPlugins()
{
    for (int i = list.getNumTypes(); --i >= 0;)
        if (! formatManager.doesPluginStillExist (*list.getType (i)))
            list.removeType (i);
}

void PluginListComponent::removePluginItem (int index)
{
    if (const auto* type = list.getType(index))
        if (type->pluginFormatName == "Element")
            return;
    
    if (index < list.getNumTypes())
        list.removeType (index);
    else
        list.removeFromBlacklist (list.getBlacklistedFiles() [index - list.getNumTypes()]);
}

void PluginListComponent::optionsMenuStaticCallback (int result, PluginListComponent* pluginList)
{
    if (pluginList != nullptr)
        pluginList->optionsMenuCallback (result);
}

static void removeNonElementPlugins (KnownPluginList& list)
{
    for (int i = list.getNumTypes(); --i >= 0;) {
        if (list.getType(i)->pluginFormatName != "Element")
            list.removeType (i);
    }
}

static void saveSettings (Component* c)
{
    if (auto* g = ViewHelpers::getGlobals (c))
        g->getPluginManager().saveUserPlugins (g->getSettings());
}

void PluginListComponent::optionsMenuCallback (int result)
{
    switch (result)
    {
        case 0:   break;
        case 1:   removeNonElementPlugins (list);  saveSettings(this); break;
        case 2:   removeSelectedPlugins();         saveSettings(this); break;
        case 3:   showSelectedFolder();   break;
        case 4:   removeMissingPlugins(); break;
            
        default:
            if (AudioPluginFormat* format = formatManager.getFormat (result - 10))
                scanFor (*format);
            
            break;
    }
}

void PluginListComponent::buttonClicked (Button* button)
{
    if (button == &optionsButton)
    {
        PopupMenu menu;
        
        menu.addItem (1, TRANS("Clear list"));
        menu.addItem (2, TRANS("Remove selected plug-in from list"), table.getNumSelectedRows() > 0);
        menu.addItem (3, TRANS("Show folder containing selected plug-in"), canShowSelectedFolder());
        menu.addItem (4, TRANS("Remove any plug-ins whose files no longer exist"));
        menu.addSeparator();
        
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            AudioPluginFormat* const format = formatManager.getFormat (i);
                
            if (format->canScanForPlugins())
                menu.addItem (10 + i, "Scan for new or updated " + format->getName() + " plugins");
        }
        
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&optionsButton),
                            ModalCallbackFunction::forComponent (optionsMenuStaticCallback, this));
    }
    else if (button == &closeButton)
    {
        ViewHelpers::invokeDirectly (this, Commands::showLastContentView, true);
    }
    else
    {
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            AudioPluginFormat* const format = formatManager.getFormat (i);
            if (format->getName() == button->getName())
            {
                scanFor (*format);
                break;
            }
        }
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

void PluginListComponent::setLastSearchPath (PropertiesFile& properties, AudioPluginFormat& format,
                                             const FileSearchPath& newPath)
{
    properties.setValue (Settings::lastPluginScanPathPrefix + format.getName(),
                         newPath.toString());
}

// MARK: Scanner

class PluginListComponent::Scanner : private Timer,
                                     public PluginScanner::Listener
{
public:
    Scanner (PluginListComponent& plc, AudioPluginFormat& format, PropertiesFile* properties,
             bool allowPluginsWhichRequireAsynchronousInstantiation, int threads,
             const String& title, const String& text)
    : owner (plc), formatToScan (format), propertiesToUse (properties),
      pathChooserWindow (TRANS("Select folders to scan..."), String(), AlertWindow::NoIcon),
      progressWindow (title, text, AlertWindow::NoIcon),
      progress (0.0), numThreads (threads),
      allowAsync (allowPluginsWhichRequireAsynchronousInstantiation),
      finished (false), useBackgroundScanner (false)
    {
        FileSearchPath path (formatToScan.getDefaultLocationsToSearch());
        
        // You need to use at least one thread when scanning plug-ins asynchronously
        jassert (! allowAsync || (numThreads > 0));
        
        if (path.getNumPaths() > 0 )
        {
           #if ! JUCE_IOS
            if (propertiesToUse != nullptr)
                path = getLastSearchPath (*propertiesToUse, formatToScan);
           #endif
            pathList.setSize (500, 300);
            pathList.setPath (path);
            
            pathChooserWindow.addCustomComponent (&pathList);
            pathChooserWindow.addButton (TRANS("Scan"),   1, KeyPress (KeyPress::returnKey));
            pathChooserWindow.addButton (TRANS("Cancel"), 0, KeyPress (KeyPress::escapeKey));
            
            pathChooserWindow.enterModalState (true,
                ModalCallbackFunction::forComponent (startScanCallback, &pathChooserWindow, this),
                false);
        }
        else
        {
            startScan();
        }
    }
        
    Scanner (PluginListComponent& plc, PluginManager& plugins, const String& title, const String& text)
        : owner (plc), formatToScan (*plugins.getAudioPluginFormat("Element")), propertiesToUse (nullptr),
          pathChooserWindow (TRANS("Select folders to scan..."), String(), AlertWindow::NoIcon),
          progressWindow (title, text, AlertWindow::NoIcon),
            progress (0.0), numThreads (0), allowAsync (false), finished (false),
      useBackgroundScanner (true)
    {
        startScan();
    }
    
    ~Scanner()
    {
        if (pool != nullptr)
        {
            pool->removeAllJobs (true, 60000);
            pool = nullptr;
        }
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
    int numThreads;
    bool allowAsync, finished;
    ScopedPointer<ThreadPool> pool;
    bool useBackgroundScanner = false;
    
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
                                              TRANS("Plugin Scanning"),
                                              TRANS("If you choose to scan folders that contain non-plugin files, "
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
        
        startScan();
    }
    
    static bool isStupidPath (const File& f)
    {
        Array<File> roots;
        File::findFileSystemRoots (roots);
        
        if (roots.contains (f))
            return true;
        
        File::SpecialLocationType pathsThatWouldBeStupidToScan[]
        = { File::globalApplicationsDirectory,
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
        
        if (useBackgroundScanner)
            scanner.setNonOwned (owner.plugins.getBackgroundAudioPluginScanner());
        else
            scanner.setOwned (owner.plugins.createAudioPluginScanner());
        
        if (propertiesToUse != nullptr)
        {
            setLastSearchPath (*propertiesToUse, formatToScan, pathList.getPath());
            propertiesToUse->saveIfNeeded();
        }

        progressWindow.addButton (TRANS("Cancel"), 0, KeyPress (KeyPress::escapeKey));
        
        
        progressWindow.addProgressBarComponent (progress);
        progress = -1.0;
        progressWindow.enterModalState();
        
        scanner->addListener (this);
        if (! useBackgroundScanner)
            scanner->scanForAudioPlugins (formatToScan.getName());
        startTimer (20);
    }
    
    void finishedScan()
    {
        if (scanner)
            scanner->removeListener (this);
        owner.scanFinished (scanner != nullptr ? scanner->getFailedFiles()
                                               : StringArray());
    }
    
    void timerCallback() override
    {
        if (doNextScan())
            startTimer (20);
        
        if (! progressWindow.isCurrentlyModal())
            finished = true;
        
        if (finished)
            finishedScan();
        else
            progressWindow.setMessage (pluginBeingScanned);
    }
    
    bool doNextScan()
    {
        return true;
    }
    
    void audioPluginScanFinished() override
    {
        finished = true;
    }
    
    void audioPluginScanStarted (const String& pluginName) override
    {
        pluginBeingScanned = File::createFileWithoutCheckingPath(pluginName).getFileName();
        DBG("[EL] scanning: " << pluginBeingScanned);
    }
    
    void audioPluginScanProgress (const float reportedProgress) override
    {
        this->progress = reportedProgress;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scanner)
};

void PluginListComponent::scanFor (AudioPluginFormat& format)
{
    if (format.getName() == "Element")
    {
        plugins.scanInternalPlugins();
        return;
    }
    
   #if EL_RUNNING_AS_PLUGIN
    AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "Plugin Scanner",
                                     "Scanning for plugins is currently not possible in the plugin version.\n\nPlease scan plugins in the application first.");
   #else
    if (auto* world = ViewHelpers::getGlobals (this))
        plugins.saveUserPlugins (world->getSettings());
    currentScanner = new Scanner (*this, format, propertiesToUse, allowAsync, numThreads,
                                  dialogTitle.isNotEmpty() ? dialogTitle : TRANS("Scanning for plug-ins..."),
                                  dialogText.isNotEmpty()  ? dialogText  : TRANS("Searching for all possible plug-in files..."));
   #endif
}

bool PluginListComponent::isScanning() const noexcept
{
    return (currentScanner != nullptr || plugins.isScanningAudioPlugins());
}

void PluginListComponent::scanFinished (const StringArray& failedFiles)
{
    StringArray shortNames;
    
    if (auto* world = ViewHelpers::getGlobals (this))
    {
        const auto file = PluginScanner::getSlavePluginListFile();
        plugins.restoreAudioPlugins (file);
        file.deleteFile();
    }
    
    for (int i = 0; i < failedFiles.size(); ++i)
        shortNames.add (File::createFileWithoutCheckingPath (failedFiles[i]).getFileName());
    
    currentScanner = nullptr; // mustn't delete this before using the failed files array
    
    if (shortNames.size() > 0)
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                          TRANS("Scan complete"),
                                          TRANS("Note that the following files appeared to be plugin files, but failed to load correctly")
                                          + ":\n\n"
                                          + shortNames.joinIntoString (", "));
}
    
    
PluginManagerContentView::PluginManagerContentView()
{
    setEscapeTriggersClose (true);
}

PluginManagerContentView::~PluginManagerContentView() { }

void PluginManagerContentView::didBecomeActive()
{
    jassert (ViewHelpers::getGlobals (this));
    auto& world (*ViewHelpers::getGlobals (this));
    auto& plugins (world.getPluginManager());
    auto& settings (world.getSettings());

    if (pluginList)
        pluginList = nullptr;
    
    pluginList = new Element::PluginListComponent (plugins, settings.getUserSettings());
    addAndMakeVisible (pluginList);
    resized();
    
    grabKeyboardFocus();
}

void PluginManagerContentView::resized()
{
    if (pluginList)
        pluginList->setBounds (getLocalBounds().reduced (2));
}
    
void PluginListComponent::scanWithBackgroundScanner()
{
    if (currentScanner) {
        currentScanner = nullptr;
    }
    currentScanner = new Scanner (*this, plugins, "Scanning for plugins", "Looking for new or updated plugins");
}

}

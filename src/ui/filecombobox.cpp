// Copyright 2020 Raw Material Software Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// Copied from JUCE

#include "ui/filecombobox.hpp"

namespace element {
using namespace juce;

FileComboBox::FileComboBox (const String& name,
                            const File& currentFile,
                            bool canEditFilename,
                            bool isDirectory,
                            bool isForSaving,
                            const String& fileBrowserWildcard,
                            const String& suffix,
                            const String& textWhenNothingSelected)
    : Component (name),
      isDir (isDirectory),
      isSaving (isForSaving),
      wildcard (fileBrowserWildcard),
      enforcedSuffix (suffix)
{
    addAndMakeVisible (filenameBox);
    filenameBox.setEditableText (canEditFilename);
    filenameBox.setTextWhenNothingSelected (textWhenNothingSelected);
    filenameBox.setTextWhenNoChoicesAvailable (TRANS ("(no recently selected files)"));
    filenameBox.onChange = [this] { setCurrentFile (getCurrentFile(), true); };

    setBrowseButtonText ("...");

    setCurrentFile (currentFile, true, dontSendNotification);
}

FileComboBox::~FileComboBox()
{
}

//==============================================================================
void FileComboBox::paintOverChildren (Graphics& g)
{
    if (isFileDragOver)
    {
        g.setColour (Colours::red.withAlpha (0.2f));
        g.drawRect (getLocalBounds(), 3);
    }
}

void FileComboBox::setShowFullPathName (bool showFullPath)
{
    if (showFullPathNames == showFullPath)
        return;
    showFullPathNames = showFullPath;
    auto savedRecents = recentFiles;
    recentFiles.clearQuick();
    setRecentlyUsedFilenames (savedRecents);
}

static void layoutFileComboBox (FileComboBox& filenameComp,
                                ComboBox* filenameBox,
                                Button* browseButton)
{
    browseButton->setSize (80, filenameComp.getHeight());

    if (auto* tb = dynamic_cast<TextButton*> (browseButton))
        tb->changeWidthToFitText();

    browseButton->setTopRightPosition (filenameComp.getWidth(), 0);

    filenameBox->setBounds (0, 0, browseButton->getX(), filenameComp.getHeight());
}

void FileComboBox::resized()
{
    // getLookAndFeel().layoutFilenameComponent (*this, &filenameBox, browseButton.get());
    layoutFileComboBox (*this, &filenameBox, browseButton.get());
}

std::unique_ptr<ComponentTraverser> FileComboBox::createFocusTraverser()
{
    // This prevents the sub-components from grabbing focus if the
    // FileComboBox has been set to refuse focus.
    return getWantsKeyboardFocus() ? Component::createFocusTraverser() : nullptr;
}

void FileComboBox::setBrowseButtonText (const String& newBrowseButtonText)
{
    browseButtonText = newBrowseButtonText;
    lookAndFeelChanged();
}

void FileComboBox::lookAndFeelChanged()
{
    browseButton.reset();
    browseButton.reset (getLookAndFeel().createFilenameComponentBrowseButton (browseButtonText));
    addAndMakeVisible (browseButton.get());
    browseButton->setConnectedEdges (Button::ConnectedOnLeft);
    browseButton->onClick = [this] { showChooser(); };
    resized();
}

void FileComboBox::setTooltip (const String& newTooltip)
{
    SettableTooltipClient::setTooltip (newTooltip);
    filenameBox.setTooltip (newTooltip);
}

void FileComboBox::setDefaultBrowseTarget (const File& newDefaultDirectory)
{
    defaultBrowseFile = newDefaultDirectory;
}

File FileComboBox::getLocationToBrowse()
{
    if (lastFilename.isEmpty() && defaultBrowseFile != File())
        return defaultBrowseFile;

    return getCurrentFile();
}

void FileComboBox::showChooser()
{
    int flags = isDir
                    ? FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories
                    : (isSaving
                           ? FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                           : FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles);

    chooser = std::make_unique<FileChooser> (
        isDir ? TRANS ("Choose a new directory") : TRANS ("Choose a new file"),
        getLocationToBrowse(),
        wildcard);

    auto safeThis = Component::SafePointer<FileComboBox> (this);
    chooser->launchAsync (flags, [safeThis] (const FileChooser& fc) {
        if (safeThis != nullptr && fc.getResults().size() > 0)
            safeThis->setCurrentFile (fc.getResult(), true);
    });
}

bool FileComboBox::isInterestedInFileDrag (const StringArray&)
{
    return true;
}

void FileComboBox::filesDropped (const StringArray& filenames, int, int)
{
    isFileDragOver = false;
    repaint();

    const File f (filenames[0]);

    if (f.exists() && (f.isDirectory() == isDir))
        setCurrentFile (f, true);
}

void FileComboBox::fileDragEnter (const StringArray&, int, int)
{
    isFileDragOver = true;
    repaint();
}

void FileComboBox::fileDragExit (const StringArray&)
{
    isFileDragOver = false;
    repaint();
}

//==============================================================================
String FileComboBox::getCurrentFileText() const
{
    return filenameBox.getText();
}

File FileComboBox::getCurrentFile() const
{
    // auto f = File::getCurrentWorkingDirectory().getChildFile (getCurrentFileText());
    File f;
    if (isPositiveAndBelow (filenameBox.getSelectedItemIndex(), recentFiles.size()))
        f = File (recentFiles[filenameBox.getSelectedItemIndex()]);
    if (enforcedSuffix.isNotEmpty())
        f = f.withFileExtension (enforcedSuffix);

    return f;
}

void FileComboBox::setCurrentFile (File newFile,
                                   const bool addToRecentlyUsedList,
                                   NotificationType notification)
{
    if (enforcedSuffix.isNotEmpty())
        newFile = newFile.withFileExtension (enforcedSuffix);

    if (newFile.getFullPathName() != lastFilename)
    {
        lastFilename = newFile.getFullPathName();

        if (addToRecentlyUsedList)
            addRecentlyUsedFile (newFile);

        auto text = showFullPathNames ? lastFilename : File (lastFilename).getFileName();
        filenameBox.setText (text, dontSendNotification);

        if (notification != dontSendNotification)
        {
            triggerAsyncUpdate();

            if (notification == sendNotificationSync)
                handleUpdateNowIfNeeded();
        }
    }
}

void FileComboBox::setFilenameIsEditable (const bool shouldBeEditable)
{
    filenameBox.setEditableText (shouldBeEditable);
}

StringArray FileComboBox::getRecentlyUsedFilenames() const
{
    return recentFiles;
}

void FileComboBox::setRecentlyUsedFilenames (const StringArray& filenames)
{
    if (filenames != getRecentlyUsedFilenames())
    {
        filenameBox.clear();

        for (int i = 0; i < jmin (filenames.size(), maxRecentFiles); ++i)
        {
            if (! File::isAbsolutePath (filenames[i]))
                continue;
            filenameBox.addItem (showFullPathNames ? filenames[i]
                                                   : File (filenames[i]).getFileName(),
                                 i + 1);
        }

        recentFiles = filenames;
    }
}

void FileComboBox::setMaxNumberOfRecentFiles (const int newMaximum)
{
    maxRecentFiles = jmax (1, newMaximum);

    setRecentlyUsedFilenames (getRecentlyUsedFilenames());
}

void FileComboBox::addRecentlyUsedFile (const File& file)
{
    auto files = getRecentlyUsedFilenames();

    if (file.getFullPathName().isNotEmpty())
    {
        files.removeString (file.getFullPathName(), true);
        files.insert (0, file.getFullPathName());

        setRecentlyUsedFilenames (files);
    }
}

//==============================================================================
void FileComboBox::addListener (FileComboBoxListener* const listener)
{
    listeners.add (listener);
}

void FileComboBox::removeListener (FileComboBoxListener* const listener)
{
    listeners.remove (listener);
}

void FileComboBox::handleAsyncUpdate()
{
    Component::BailOutChecker checker (this);
    listeners.callChecked (checker, [this] (FileComboBoxListener& l) { l.fileComboBoxChanged (this); });
}

} // namespace element

/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "gui/FileComboBox.h"

namespace element {

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
#if JUCE_MODAL_LOOPS_PERMITTED
    FileChooser fc (isDir ? TRANS ("Choose a new directory")
                          : TRANS ("Choose a new file"),
                    getLocationToBrowse(),
                    wildcard);

    if (isDir ? fc.browseForDirectory()
              : (isSaving ? fc.browseForFileToSave (false)
                          : fc.browseForFileToOpen()))
    {
        setCurrentFile (fc.getResult(), true);
    }
#else
    ignoreUnused (isSaving);
    jassertfalse; // needs rewriting to deal with non-modal environments
#endif
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

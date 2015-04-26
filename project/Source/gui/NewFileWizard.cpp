/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "NewFileWizard.h"

//==============================================================================
namespace {
    const int fileWizardBaseMenu = 0x12d83f0;
}

namespace Element {
namespace Gui {


class NewPatternWizard  : public NewFileWizard::Type
{
public:

    NewPatternWizard() {}

    String getName()  { return "Pattern"; }

    void createNewFile (AssetTree::Item parent)
    {
        const File newFile (askUserToChooseNewFile ("New Pattern.btpt", "*.btpt", parent));

        if (newFile != File::nonexistent)
            create (parent, newFile);
    }

    static bool create (AssetTree::Item parent, const File& newFile)
    {
#if 0
        if (XmlElement* e = Pattern::create()->node().createXml())
        {
            bool written = e->writeToFile (newFile, String::empty, "UTF-8", 80);
            if (written)
                parent.addFile (newFile, 0, true);

            delete e;
            return true;
        }
#endif

        showFailedToWriteMessage (newFile);
        return false;
    }

};


void NewFileWizard::Type::showFailedToWriteMessage (const File& file)
{
    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                 "Failed to Create File!",
                                 "Couldn't write to the file: " + file.getFullPathName());
}

File
NewFileWizard::Type::askUserToChooseNewFile (const String& suggestedFilename, const String& wildcard,
                                             const AssetTree::Item& projectGroupToAddTo)
{
    FileChooser fc ("Select File to Create",
                    projectGroupToAddTo.determineGroupFolder()
                                       .getChildFile (suggestedFilename)
                                       .getNonexistentSibling(),
                    wildcard, false);

    if (fc.browseForFileToSave (true))
        return fc.getResult();

    return File::nonexistent;
}

//==============================================================================
NewFileWizard::NewFileWizard()
{
    registerWizard (new NewPatternWizard ());
}

NewFileWizard::~NewFileWizard()
{
}

void NewFileWizard::addWizardsToMenu (PopupMenu& m) const
{
    for (int i = 0; i < wizards.size(); ++i)
        m.addItem (fileWizardBaseMenu + i, "Add New " + wizards.getUnchecked(i)->getName() + "...");
}

bool NewFileWizard::runWizardFromMenu (int chosenMenuItemID, const AssetTree::Item& projectGroupToAddTo) const
{
    if (Type* wiz = wizards [chosenMenuItemID - fileWizardBaseMenu])
    {
        wiz->createNewFile (projectGroupToAddTo);
        return true;
    }

    return false;
}

void NewFileWizard::registerWizard (Type* newWizard)
{
    wizards.add (newWizard);
}

}}

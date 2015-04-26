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

#ifndef ELEMENT_NEW_FILE_WIZARD_H
#define ELEMENT_NEW_FILE_WIZARD_H

#include "../session/Session.h"
#include "GuiCommon.h"

namespace Element {
namespace Gui {

class NewFileWizard : private DeletedAtShutdown
{
public:

    NewFileWizard();
    ~NewFileWizard();

    class Type
    {
    public:

        Type() { }
        virtual ~Type()  { }

        virtual String getName() = 0;
        virtual void createNewFile (AssetTree::Item group) = 0;

    protected:
        File askUserToChooseNewFile (const String& suggestedFilename, const String& wildcard,
                                     const AssetTree::Item& projectGroupToAddTo);
        static void showFailedToWriteMessage (const File& file);
    };

    void addWizardsToMenu (PopupMenu& m) const;
    bool runWizardFromMenu (int chosenMenuItemID, const AssetTree::Item& group) const;
    void registerWizard (Type* newWizard);

private:
    OwnedArray <Type> wizards;
};


}}

#endif   // ELEMENT_NEW_FILE_WIZARD_H

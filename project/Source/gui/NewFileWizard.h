#ifndef ELEMENT_NEW_FILE_WIZARD_H
#define ELEMENT_NEW_FILE_WIZARD_H

#include "session/Session.h"
#include "gui/GuiCommon.h"

namespace Element {

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


}

#endif   // ELEMENT_NEW_FILE_WIZARD_H

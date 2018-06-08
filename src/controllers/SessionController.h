
#pragma once

#include "controllers/AppController.h"
#include "gui/SessionDocument.h"
#include "session/Session.h"

namespace Element {
class SessionController : public AppController::Child
{
public:
    SessionController() { }
    ~SessionController() { }
    
    void activate() override;
    void deactivate() override;
    
    void openFile (const File& file);
    void closeSession();
    void saveSession (const bool saveAs = false);
    void newSession();
    bool hasSessionChanged() { return (document) ? document->hasChangedSinceSaved() : false; }

    inline void setChangesFrozen (const bool areNowFrozen)
    {
        if (! currentSession)
            return;
        currentSession->freezeChangeNotification = areNowFrozen;
        if (document)
            document->setChangedFlag (false);
    }

    inline void resetChanges()
    {
        if (! document)
            return;
        const bool wasFrozen = currentSession->freezeChangeNotification;
        currentSession->freezeChangeNotification = true;
        const File file = (document) ? document->getLastDocumentOpened() : File();
        document = new SessionDocument (currentSession);
        document->setFile (file);
        document->setChangedFlag (false);
        currentSession->freezeChangeNotification = wasFrozen;
        jassert(! hasSessionChanged());
    }
    
    void exportGraph (const Node& node, const File& targetFile);
    void importGraph (const File& file);
    
private:
    SessionPtr currentSession;
    ScopedPointer<SessionDocument> document;
};

}

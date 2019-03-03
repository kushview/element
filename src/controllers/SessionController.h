
#pragma once

#include "controllers/AppController.h"
#include "documents/SessionDocument.h"
#include "session/Session.h"
#include "Signals.h"

namespace Element {
class SessionController : public AppController::Child
{
public:
    SessionController() { }
    ~SessionController() { }
    
    void activate() override;
    void deactivate() override;
    
    void openDefaultSession();
    void openFile (const File& file);
    const File getSessionFile() const { return document != nullptr ? document->getFile() : File(); }
    void closeSession();
    void saveSession (const bool saveAs = false);
    void newSession();
    bool hasSessionChanged() { return (document) ? document->hasChangedSinceSaved() : false; }

    void resetChanges (const bool clearDocumentFile = false);
    
    void exportGraph (const Node& node, const File& targetFile);
    void importGraph (const File& file);
    
    Signal<void()> sessionLoaded;
private:
    SessionPtr currentSession;
    ScopedPointer<SessionDocument> document;
    void loadNewSessionData();
    void refreshOtherControllers();
};

}

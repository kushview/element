
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
    
    void exportGraph (const Node& node, const File& targetFile);
    void importGraph (const File& file);
    
private:
    SessionPtr currentSession;
    ScopedPointer<SessionDocument> document;
};

}

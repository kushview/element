
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/SessionController.h"
#include "Globals.h"

namespace Element {
    void SessionController::activate()
    {
        auto* app = dynamic_cast<AppController*> (getRoot());
        auto* ec  = dynamic_cast<EngineController*> (app->findChild<EngineController>());
        currentSession = app->getWorld().getSession();
        document = new SessionDocument (currentSession);
    }
    
    void SessionController::deactivate()
    {
        document = nullptr;
        currentSession = nullptr;
    }
    
    void SessionController::openSession()
    {
        if (document->loadFromUserSpecifiedFile (true))
        {
            DBG("[SC] open session");
        }
    }
    void SessionController::closeSession()
    {
        DBG("[SC] close session");
    }
    
    void SessionController::saveSession (const bool saveAs)
    {
        if (! document)
            return;
        
        if (! saveAs) {
            document->save (true, true);
        } else {
            document->saveAs (File::nonexistent, true, true, true);
        }
    }
    
    void SessionController::newSession()
    {
        jassert (document && currentSession);
        if (! document->hasChangedSinceSaved())
            return;
        
        // - 0 if the third button was pressed ('cancel')
        // - 1 if the first button was pressed ('yes')
        // - 2 if the middle button was pressed ('no')
        int res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon, "Save Session?",
                                                   "The current session has changes. Would you like to save it?",
                                                   "Save Session", "Don't Save", "Cancel");
        if (res == 1)
            document->save (true, true);
        
        if (res == 1 || res == 2)
        {
            document->setFile (File::nonexistent);
            document->setChangedFlag (false);
        }
    }
}

#pragma once

#include "controllers/AppController.h"
#include "documents/SessionDocument.h"
#include "session/Session.h"

namespace Element {

class ScriptingController : public AppController::Child
{
public:
    ScriptingController();
    ~ScriptingController();
    void activate() override;
    void deactivate() override;
};

}

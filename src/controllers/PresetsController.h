
#pragma once

#include "controllers/AppController.h"
#include "gui/SessionDocument.h"
#include "session/Session.h"

namespace Element {

class PresetsController : public AppController::Child
{
public:
    PresetsController();
    ~PresetsController();
    void activate() override;
    void deactivate() override;

    void refresh();
    void add (const Node& Node, const String& presetName = String());

private:
    friend struct Pimpl; struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}


#pragma once

#include "controllers/AppController.h"
#include "gui/GraphDocument.h"

namespace Element {

class GraphController : public AppController::Child
{
public:
    GraphController() = default;
    ~GraphController() = default;
    
    void activate() override;
    void deactivate() override;

    bool hasGraphChanged() const { return document.hasChangedSinceSaved(); }

    void openGraph (const File& file);
    void newGraph();
    void saveGraph (const bool saveAs);

private:
    GraphDocument document;
    void refreshOtherControllers();
};

}

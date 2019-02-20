
#pragma once

#include "controllers/AppController.h"
#include "gui/GraphDocument.h"

namespace Element {

/** Responsible for creating new, opening, and saving graph files in
    Element Lite and Solo */
class GraphController final : public AppController::Child
{
public:
    GraphController() = default;
    ~GraphController() = default;
    
    void activate() override;
    void deactivate() override;

    bool hasGraphChanged() const { return document.hasChangedSinceSaved(); }

    void openDefaultGraph();
    void openGraph (const File& file);
    void newGraph();
    void saveGraph (const bool saveAs);

private:
    GraphDocument document;
    void refreshOtherControllers();
};

}


#pragma once

#include "controllers/AppController.h"
#include "documents/GraphDocument.h"
#include "Signals.h"

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

    bool hasGraphChanged() const        { return document.hasChangedSinceSaved(); }
    const File getGraphFile() const     { return document.getFile(); }
    Node getGraph() const               { return document.getGraph(); }

    void openDefaultGraph();
    void openGraph (const File& file);
    void newGraph();
    void saveGraph (const bool saveAs);
    void loadGraph (const Node& graph);
    Signal<void()> graphChanged;

private:
    GraphDocument document;
    std::unique_ptr<Component> wizard;
    void refreshOtherControllers();
};

}

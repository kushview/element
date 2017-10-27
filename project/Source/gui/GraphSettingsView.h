
#pragma once

#include "gui/ContentComponent.h"

namespace Element {
    class GraphPropertyPanel;
    class GraphSettingsView : public ContentView {
    public:
        GraphSettingsView();
        ~GraphSettingsView();
        
        void resized() override;
        void didBecomeActive() override;
        void paint (Graphics& g) override;
        void stabilizeContent() override;
    private:
        ScopedPointer<GraphPropertyPanel> props;
    };
    
}


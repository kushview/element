
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class GraphMixerView : public ContentView
{
public:
    GraphMixerView();
    ~GraphMixerView();

    void resized() override;
    void stabilizeContent() override;
    void initializeView (AppController&) override;
    
private:
    class Content; friend class Content;
    std::unique_ptr<Content> content;
};

}

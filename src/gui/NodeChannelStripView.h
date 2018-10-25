
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class NodeChannelStripView : public ContentView
{
public:
    NodeChannelStripView();
    ~NodeChannelStripView();

    void resized() override;
    void stabilizeContent() override;
    void initializeView (AppController&) override;
    
private:
    class Content; friend class Content;
    std::unique_ptr<Content> content;
};

}

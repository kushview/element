
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class SessionTreePanel;

class SessionTreeContentView : public ContentView
{
public:
    SessionTreeContentView();
    ~SessionTreeContentView();

    void didBecomeActive() override;
    void stabilizeContent() override;
    void resized() override;

private:
    std::unique_ptr<SessionTreePanel> tree;
};

}

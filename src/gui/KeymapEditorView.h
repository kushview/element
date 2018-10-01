
#pragma once

#include "ElementApp.h"
#include "gui/ContentComponent.h"

namespace Element {
class KeymapEditorView : public ContentView, public Button::Listener
{
public:
    KeymapEditorView();
    virtual ~KeymapEditorView() { }

    void resized() override;
    
    inline void willBecomeActive() override { }
    inline void didBecomeActive()  override { stabilizeContent(); }
    void stabilizeContent() override;

    void buttonClicked (Button*) override;
private:
    ScopedPointer<KeyMappingEditorComponent> editor;
    TextButton closeButton;

    void saveMappings();
};
}

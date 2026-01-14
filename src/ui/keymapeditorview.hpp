// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"
#include <element/ui/content.hpp>

#define EL_VIEW_KEYMAP_EDITOR "KeymapEditorView"

namespace element {
class KeymapEditorView : public ContentView, public Button::Listener
{
public:
    KeymapEditorView();
    virtual ~KeymapEditorView() {}

    void resized() override;

    inline void willBecomeActive() override {}
    inline void didBecomeActive() override { stabilizeContent(); }
    void stabilizeContent() override;

    void buttonClicked (Button*) override;

private:
    std::unique_ptr<KeyMappingEditorComponent> editor;
    TextButton closeButton;

    void saveMappings();
};
} // namespace element

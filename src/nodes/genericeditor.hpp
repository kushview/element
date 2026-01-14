// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>

namespace element {

class GenericNodeEditor : public NodeEditor
{
public:
    GenericNodeEditor (const Node&);
    ~GenericNodeEditor() override;
    void resized() override;
    void paint (Graphics&) override;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace element

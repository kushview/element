// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class MidiFilterNode : public Processor
{
public:
    virtual ~MidiFilterNode();

protected:
    MidiFilterNode (uint32 nodeId);
    inline bool wantsMidiPipe() const override { return true; }
};

} // namespace element

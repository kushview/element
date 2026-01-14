// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class MidiFilterNode : public Processor
{
public:
    virtual ~MidiFilterNode();

protected:
    MidiFilterNode (uint32 nodeId);
    inline bool wantsContext() const noexcept override { return true; }
};

} // namespace element

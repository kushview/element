// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nodes/midifilter.hpp"

namespace element {

MidiFilterNode::MidiFilterNode (uint32 nodeId)
    : Processor (nodeId) {}
MidiFilterNode::~MidiFilterNode() {}

} // namespace element

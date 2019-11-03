/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "engine/nodes/MidiMonitorNode.h"
#include "gui/LookAndFeel.h"
#include "Utils.h"

namespace Element {

MidiMonitorNode::MidiMonitorNode()
    : MidiFilterNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_MIDI_MONITOR, nullptr);

    setSize (width, height);
}

MidiMonitorNode::~MidiMonitorNode() { }

void MidiMonitorNode::clear()
{
}

void MidiMonitorNode::prepareToRender (double sampleRate, int maxBufferSize)
{
}

void MidiMonitorNode::releaseResources() { }

void MidiMonitorNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
}

}

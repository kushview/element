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

#pragma once

#include "JuceHeader.h"
#include "session/controllerdevice.hpp"
#include "signals.hpp"

namespace element {

class ControllerMapHandler;
class ControllerMapInput;
class NodeObject;
class MidiEngine;

class MappingEngine
{
public:
    using CapturedEventSignal = Signal<void()>;

    MappingEngine();
    ~MappingEngine();

    bool addInput (const ControllerDevice&, MidiEngine&);
    bool addHandler (const ControllerDevice::Control&, const Node&, const int);

    bool removeInput (const ControllerDevice&);
    bool refreshInput (const ControllerDevice&);
    void clear();
    void startMapping();
    void stopMapping();

    void capture (const bool start = true) { capturedEvent.capture.set (start); }
    MidiMessage getCapturedMidiMessage() const { return capturedEvent.message; }
    ControllerDevice::Control getCapturedControl() const { return capturedEvent.control; }
    CapturedEventSignal& capturedSignal() { return capturedEvent.callback; }

private:
    friend class ControllerMapInput;
    class Inputs;
    std::unique_ptr<Inputs> inputs;

    class CapturedEvent : public AsyncUpdater
    {
    public:
        CapturedEvent() { capture.set (false); }
        ~CapturedEvent() {}
        inline void handleAsyncUpdate() override
        {
            capture.set (false);
            callback();
        }

    private:
        friend class MappingEngine;
        Atomic<bool> capture;
        ControllerDevice::Control control;
        MidiMessage message;
        CapturedEventSignal callback;
    } capturedEvent;

    bool captureNextEvent (ControllerMapInput&, const ControllerDevice::Control&, const MidiMessage&);
};

} // namespace element

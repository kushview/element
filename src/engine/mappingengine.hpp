// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>
#include <element/controller.hpp>
#include <element/signals.hpp>

namespace element {

class ControllerMapHandler;
class ControllerMapInput;
class Processor;
class Node;
class MidiEngine;

class MappingEngine
{
public:
    using CapturedEventSignal = Signal<void()>;

    MappingEngine();
    ~MappingEngine();

    bool addInput (const Controller&, MidiEngine&);
    bool addHandler (const Control&, const Node&, const int);

    bool removeInput (const Controller&);
    bool refreshInput (const Controller&);
    void clear();
    void startMapping();
    void stopMapping();

    void capture (const bool start = true) { capturedEvent.capture.set (start); }
    MidiMessage getCapturedMidiMessage() const { return capturedEvent.message; }
    Control getCapturedControl() const { return capturedEvent.control; }
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
        Control control;
        MidiMessage message;
        CapturedEventSignal callback;
    } capturedEvent;

    bool captureNextEvent (ControllerMapInput&, const Control&, const MidiMessage&);
};

} // namespace element


#pragma once

#include "JuceHeader.h"
#include "session/ControllerDevice.h"
#include "Signals.h"

namespace Element {

class ControllerMapHandler;
class ControllerMapInput;
class GraphNode;

class MappingEngine
{
public:
    using CapturedEventSignal = Signal<void()>;
    
    MappingEngine();
    ~MappingEngine();

    bool addInput (const ControllerDevice&);
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
    class Inputs; std::unique_ptr<Inputs> inputs;

    class CapturedEvent : public AsyncUpdater
    {
    public:
        CapturedEvent() { capture.set (false); }
        ~CapturedEvent() { }
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

}

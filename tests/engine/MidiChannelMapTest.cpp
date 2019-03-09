#include "Tests.h"
#include "engine/MidiChannelMap.h"
#include "engine/nodes/MidiChannelMapProcessor.h"

using namespace Element;

namespace Element {

class MidiChannelMapTest : public UnitTestBase
{
public:
    explicit MidiChannelMapTest (const String& name = "MidiChannelMap") 
        : UnitTestBase (name, "engine", "midiChannelMap") { }
    virtual ~MidiChannelMapTest() { }

    void initialise() override {
        MessageManager::getInstance();
    }

    void runTest() override
    {
        testPortTypes();
        testPortConfig();
        testReset();
        testOutputCorrect();
        testProcess();
    }

private:
    void testProcess()
    {
        beginTest ("process()");
        MidiChannelMap chmap;
        MidiMessage note (MidiMessage::noteOn (5, 100, 1.f));
        chmap.set (5, 6);
        chmap.process (note);
        expect (chmap.get (5) == 6);
    }

    void testReset()
    {
        beginTest ("reset()");
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        chmap.reset();
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get(ch) == ch);
    }

    void testOutputCorrect()
    {
        beginTest ("get() and set()");
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get(ch) == ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get (ch) == 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 12);
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get (ch) == 12);
    }

    void testPortTypes()
    {
        beginTest ("port types");
        const PortType midi (String ("midi"));
        expect (midi.isMidi());
        const PortType control (String ("control"));
        expect (control.isControl());
    }

    void testPortConfig()
    {
        beginTest ("port config");
        GraphProcessor proc;
        proc.setPlayConfigDetails (2, 2, 44100.f, 1024);
        proc.prepareToPlay (44100.0, 512);
        auto* processor = new MidiChannelMapProcessor();
        expect (processor->getTotalNumInputChannels() == 0);
        expect (processor->getTotalNumOutputChannels() == 0);
        
        GraphNodePtr node = proc.addNode (processor, 0);
        MessageManager::getInstance()->runDispatchLoopUntil (10);
        expect (processor->getTotalNumInputChannels() == 0);
        expect (processor->getTotalNumOutputChannels() == 0);
        
        expect (node != nullptr);

        expect (node->getNumAudioInputs() == 0);
        expect (node->getNumAudioOutputs() == 0);
        expect (node->getNumPorts (PortType::Audio, true) == 0);
        expect (node->getNumPorts (PortType::Audio, false) == 0);
        expect (node->getNumPorts (PortType::Control, true) == 16);
        expect (node->getNumPorts (PortType::Control, false) == 0);
        expect (node->getNumPorts (PortType::Midi, true) == 1);
        expect (node->getNumPorts (PortType::Midi, false) == 1);
        expect (node->getNumPorts() == 18);

        for (int i = 0; i < 16; ++i)
        {
            expect (node->getPortType(i) == PortType::Control);
            expect (node->isPortInput (i));
        }

        expect (node->getPortType (16) == PortType::Midi);
        expect (node->isPortInput (16));
        expect (node->getMidiInputPort() == 16);
        expect (node->getPortType (17) == PortType::Midi);
        expect (! node->isPortInput (17));
        expect (node->getMidiOutputPort() == 17);

        expect (node->getPortType (18) == PortType::Unknown);

        GraphNodePtr midiIn = proc.addNode (new Element::GraphProcessor::AudioGraphIOProcessor (
            GraphProcessor::AudioGraphIOProcessor::midiInputNode));

        beginTest ("connectivity");
        expect (midiIn->getNumPorts() == 1);
        expect (midiIn->getPortType (0) == PortType::Midi);
        expect (midiIn->isPortInput (0) == false);
        expect (proc.addConnection (midiIn->nodeId, midiIn->getMidiOutputPort(),
                                    node->nodeId, node->getMidiInputPort()));

        node = nullptr;
        midiIn = nullptr;
        proc.releaseResources();
        proc.clear();
    }
};

static MidiChannelMapTest sMidiChannelMapTest;

}

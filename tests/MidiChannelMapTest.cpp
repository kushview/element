#include "Tests.h"
#include "engine/MidiChannelMap.h"

using namespace Element;

namespace Element {

class MidiChannelMapTest : public UnitTestBase
{
public:
    explicit MidiChannelMapTest (const String& name = "MidiChannelMap") 
        : UnitTestBase (name, "dsp", "midiChannelMap") { }
    virtual ~MidiChannelMapTest() { }

    void runTest() override
    {
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
};

static MidiChannelMapTest sMidiChannelMapTest;

}

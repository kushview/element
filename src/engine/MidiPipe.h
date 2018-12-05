
#pragma once

#include "JuceHeader.h"

namespace Element {

class MidiPipe
{
public:
    MidiPipe();
    MidiPipe (MidiBuffer** buffers, int numBuffers);
    MidiPipe (const OwnedArray<MidiBuffer>& buffers, const Array<int>& channels);
    ~MidiPipe();

    int getNumBuffers() const { return size; }
    const MidiBuffer* const getReadBuffer (const int index) const;
    MidiBuffer* const getWriteBuffer (const int index) const;

    void clear();
    void clear (int startSample, int numSamples);
    void clear (int channel, int startSample, int numSamples);

private:
    enum { maxReferencedBuffers = 32 };
    int size = 0;
    MidiBuffer* referencedBuffers [maxReferencedBuffers];
};

}

#include "engine/AudioEngine.h"
#include "engine/ClipFactory.h"
#include "engine/ClipSource.h"
#include "engine/MidiSequenceProcessor.h"
#include "engine/Transport.h"
#include "session/NoteSequence.h"
#include "gui/MidiEditorComponent.h"

namespace Element {

class MidiSequenceEditor : public AudioProcessorEditor
{
public:
    MidiSequenceEditor (MidiSequenceProcessor* p, const MidiClip& clip)
        : AudioProcessorEditor (p), proc (p)
    {
        addAndMakeVisible (ed = new MidiEditorComponent (keyboard));
        const NoteSequence notes (clip.node().getChildWithName ("notes"));
        ed->setNoteSequence (notes);
        setSize (600, 300);
    }
    
    ~MidiSequenceEditor() noexcept
    {
        proc->editorBeingDeleted (this);
    }
    
    void resized() override
    {
        ed->setBounds (getLocalBounds());
    }
    
private:
    MidiSequenceProcessor* proc;
    ScopedPointer<MidiEditorComponent> ed;
    MidiKeyboardState keyboard;
};

MidiSequenceProcessor::MidiSequenceProcessor()
{
    setPlayConfigDetails (0, 0, 44100.0, 1024);
}

void MidiSequenceProcessor::prepareToPlay (double sampleRate, int estimatedBlockSize)
{
    setPlayConfigDetails (0, 0, sampleRate, estimatedBlockSize);
#if 0
    if (ClipSource* s = engine.clips().createSource (clip))
    {
        source = s;
        source->prepareToPlay (estimatedBlockSize, sampleRate);
    }
    else
    {
        source = nullptr;
        DBG("could not create midi clip source");
    }
#endif
    player.prepareToPlay (sampleRate, estimatedBlockSize);
}

void MidiSequenceProcessor::releaseResources()
{
    seq.clear();
    source = nullptr;
}

void MidiSequenceProcessor::processBlock (AudioSampleBuffer& audio, MidiBuffer& midi)
{
    midi.clear();
    #if 0
    if (source == nullptr)
        return;
    
    if (Transport* playhead = dynamic_cast<Transport*> (getPlayHead())) {
        AudioPlayHead::CurrentPositionInfo pos;
        if (playhead->getCurrentPosition (pos))
        {
            if (pos.isPlaying)
            {
                const ClipData* data = source->getClipData();
                Midi::renderSequence (midi, data->midi, playhead->getTimeScale(),
                                        static_cast<int32> (pos.timeInSamples), 
                                        audio.getNumSamples());
            }
        }
    }
    #endif
}

void MidiSequenceProcessor::getStateInformation (MemoryBlock& block)
{
    const ValueTree data = clip.node();
    MemoryOutputStream output (block, false);
    data.writeToStream (output);
}

void MidiSequenceProcessor::setStateInformation (const void* data, int size)
{
    MemoryInputStream input (data, static_cast<size_t> (size), false);
    const ValueTree newData (ValueTree::readFromStream (input));
    if (newData.isValid())
    {
#if 0
        clip.setData (newData);
        if (auto* s = engine.clips().createSource (clip)) {
            source = s;
        }
        else
        {
            source = nullptr;
        }
        #endif
    }
}

AudioProcessorEditor* MidiSequenceProcessor::createEditor()
{
    return new MidiSequenceEditor (this, clip);
}

}

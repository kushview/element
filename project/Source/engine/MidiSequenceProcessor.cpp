#include "engine/MidiSequenceProcessor.h"
#include "engine/Transport.h"
#include "gui/MidiEditorComponent.h"

namespace Element {
    
    class MidiSequenceEditor : public AudioProcessorEditor {
    public:
        MidiSequenceEditor (MidiSequenceProcessor* p) : AudioProcessorEditor(p), proc(p)
        {
            addAndMakeVisible (ed = new MidiEditorComponent (keyboard));
            setSize(300, 300);
        }
        
        ~MidiSequenceEditor()
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
        
        for (int i = 0; i < 8; ++i)
        {
            MidiMessage msg = MidiMessage::noteOn (1, 72, 1.0f);
            MidiMessage off = MidiMessage::noteOff (1, 72);
            const double timestamp = static_cast<double> (Shuttle::PPQ * i);
            msg.setTimeStamp(timestamp);
            off.setTimeStamp(timestamp + static_cast<double> (Shuttle::PPQ * 0.5));
            seq.addEvent (msg); seq.addEvent (off);
            seq.updateMatchedPairs();
        }
    }
    
    void MidiSequenceProcessor::prepareToPlay (double sampleRate, int estimatedBlockSize)
    {
        setPlayConfigDetails (0, 0, sampleRate, estimatedBlockSize);
        player.prepareToPlay (sampleRate, estimatedBlockSize);
    }
    
    void MidiSequenceProcessor::releaseResources() { }
    
    void MidiSequenceProcessor::processBlock (AudioSampleBuffer& audio, MidiBuffer& midi)
    {
        midi.clear();
        if (Transport* playhead = dynamic_cast<Transport*> (getPlayHead())) {
            AudioPlayHead::CurrentPositionInfo pos;
            if (playhead->getCurrentPosition (pos)) {
                if (pos.isPlaying) {
                    Midi::renderSequence(midi, seq, playhead->getTimeScale(),
                                         pos.timeInSamples, audio.getNumSamples());
                }
            }
        }
    }
    
    AudioProcessorEditor* MidiSequenceProcessor::createEditor()
    {
        return new MidiSequenceEditor (this);
    }
}

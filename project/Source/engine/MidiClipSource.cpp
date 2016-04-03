
#include "engine/ClipFactory.h"
#include "engine/ClipSource.h"
#include "engine/Engine.h"
#include "engine/MidiClipSource.h"
#include "session/MidiClip.h"
#include "session/Note.h"
#include "session/NoteSequence.h"

namespace Element {

class MidiClipData :  public ClipData,
                      public AtomicLock,
                      public ValueTree::Listener
{
public:
    MidiClipData (const ClipModel& model)
    {
        eventMap.set (0, nullptr);
        state.addListener (this);
        clipModelChanged (model);
    }
    
    ~MidiClipData ()
    {
        state.removeListener (this);
        eventMap.clear();
        midi.clear();
        state = ValueTree::invalid;
    }
    
    void clipModelChanged (const ClipModel& model) override
    {
        state = model.node().getChildWithName ("notes");
    }
    
    bool addNote (const Note& note)
    {
        MidiMessage msgOn, msgOff;
        note.getMidi (msgOn, msgOff);
        
        lock();
        EventHolder* event = midi.addEvent (msgOn);
        midi.updateMatchedPairs();
        unlock();
        
        int32 eventId = note.eventId();
        
        if (event != nullptr)
        {
            lock();
            midi.addEvent (msgOff);
            midi.updateMatchedPairs();
            unlock();
            
            if (eventId <= 0)
                eventId = nextId();
            
            eventMap.set (eventId, event);
            note.setEventId (eventId);
        }
        else
        {
            note.setEventId (0);
        }
        
        return note.eventId() > 0;
    }
    
    void buildHashTable (ValueTree&) { }
    
    bool editNote (const Note& note)
    {
        if (note.eventId() <= 0)
            return false;
        
        EventHolder* down (eventMap [note.eventId()]);
        if (! down) {
            return false;
        }
        
        MidiMessage* noteOn = &down->message;
        if (! noteOn->isNoteOn())
            return false;
        
        EventHolder* up = down->noteOffObject;
        if (! up)
            return false;
        
        MidiMessage* noteOff = &up->message;
        if (! noteOff)
            return false;
        
        lock();
        if (note.keyId() != noteOn->getNoteNumber())
        {
            noteOn->setNoteNumber (note.keyId());
            noteOff->setNoteNumber (noteOn->getNoteNumber());
        }
        
        if (note.channel() != noteOn->getChannel())
        {
            noteOn->setChannel (note.channel());
            noteOff->setChannel (noteOn->getChannel());
        }
        unlock();
        
        bool editResult = true;
        
        if (note.tickStart() != noteOn->getTimeStamp() ||
            note.tickEnd() != noteOff->getTimeStamp())
        {
            editResult = false;
            
            lock();
            noteOn->setTimeStamp (note.tickStart());
            noteOff->setTimeStamp (note.tickEnd());
            midi.updateMatchedPairs();
            midi.sort();
            unlock();
            
            editResult = true;
        }
        
        return editResult;
    }
    
    bool expireNote (const Note& note)
    {
        const int32 evid = note.eventId();
        bool res = true;
        
        if (evid > 0)
        {
            if (eventMap.contains (evid))
            {
                res = removeNote (note);
                if (res)
                    expiredIds.addIfNotAlreadyThere (evid);
            }
        } else {
            
        }
        
        return res;
    }
    
    int32 nextId()
    {
        if (expiredIds.size() > 0)
        {
            int32 res = expiredIds.getLast();
            expiredIds.removeLast();
            return res;
        }
        
        return ++lastEventId;
    }
    
    
    bool removeNote (const Note& note)
    {
        const int32 evid = note.eventId();
        if (EventHolder* ev = eventMap [evid])
        {
            const int evIndex = midi.getIndexOf (ev);
            eventMap.remove (evid);
            
            lock();
            midi.deleteEvent (evIndex, true);
            midi.updateMatchedPairs();
            unlock();
            
            return true;
        }
        
        return false;
    }

    void valueTreePropertyChanged (ValueTree& parent, const Identifier& prop) override
    {
        if (parent.hasType (Slugs::note))
        {
            const Note note (parent);
            editNote (note);
        }
    }
    
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override
    {
        if (parent == state && parent.hasType("notes") && child.hasType(Slugs::note))
        {
            const Note note (child);
            addNote (note);
        }
    }
    
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override
    {
        if (parent == state && parent.hasType("notes") && child.hasType(Slugs::note))
        {
            const Note note (child);
            removeNote (note);
            expireNote (note);
        }
    }

    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override
    {
        if (parent == state)
        {
            lock();
            this->midi.sort();
            unlock();
        }
    }
    
    void valueTreeParentChanged (ValueTree& child) override { }
    void valueTreeRedirected (ValueTree& data) override
    {
        lock();
        midi.clear();
        unlock();
        
        for (int i = 0; i < data.getNumChildren(); ++i)
        {
            const ValueTree child (data.getChild(i));
            if (! child.hasType(Slugs::note))
                continue;
            const Note note (child);
            addNote (note);
        }
    }
    
private:
    typedef MidiMessageSequence::MidiEventHolder EventHolder;
    typedef HashMap<int32, EventHolder*> EventMap;
    int32 lastEventId;
    Array<int32> expiredIds;
    EventMap eventMap;
    ValueTree state;
    
    JUCE_LEAK_DETECTOR (MidiClipData);
};

class MidiClipSource :  public ClipSource
{
public:
    MidiClipSource()
    {
        blockSize = 0;
    }
    
    ~MidiClipSource() { }
    
    void openClip (int, double)
    {
        DBG ("MidiClipSource::open()");
    }
    
    void closeClip()
    {
        DBG ("MidiClipSource::close()");
    }
    
    void renderClip (const Position& pos, AudioSourceChannelInfo&)
    {
        DBG ("render: " << pos.timeInSeconds);
    }
    
    void seekLocalFrame (const int64& frame) {
        DBG("Dummy Clip Seek: " << frame);
    }
    
    void prepareToPlay (int block, double) { blockSize = block; }
    void releaseResources() { }
    
    void getNextAudioBlock (const AudioSourceChannelInfo &)
    {
        DBG("clip pos: " << getNextReadPosition() << "-" << getNextReadPosition() + blockSize << " len: " << frameLength());
    }
    
private:
    int32 blockSize;
    
    JUCE_LEAK_DETECTOR (MidiClipSource);
};

    
class MidiClipType :  public ClipType
{
public:
    MidiClipType() { }
    ~MidiClipType() { }
    
    ClipData* createClipData (Engine&, const ClipModel& model)
    {
        return new MidiClipData (model);
    }
    
    bool canCreateFrom (const ClipModel& model)
    {
        if (const MidiClip* mc = dynamic_cast<const MidiClip*> (&model))
            { (void)mc; return true; }
        return model.getTypeString() == "midi";
    }
    
    bool canCreateFrom (const File&) { return false; }
    ClipSource* createSource (Engine&, const File&) { return nullptr; }
    ClipSource* createSource (Engine&, const ClipModel&) { return new MidiClipSource(); }
    
private:
    JUCE_LEAK_DETECTOR (MidiClipType);
};

ClipType* createMidiClipType()
{
    return new MidiClipType();
}
    
}

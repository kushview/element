/*
    AudioEngine.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_AUDIO_ENGINE_H
#define ELEMENT_AUDIO_ENGINE_H

#include "ElementApp.h"
#include "engine/Engine.h"

namespace Element {

class Globals;
class ClipFactory;
class EngineControl;
class GraphProcessor;
class PatternInterface;
class Pattern;
class Transport;

class AudioEngine : public Engine
{
public:
    AudioEngine (Globals&);
    ~AudioEngine();

    void activate();
    void deactivate();

    ValueTree createGraphTree();
    void restoreFromGraphTree (const ValueTree&);
    
    JUCE_DEPRECATED(Shared<EngineControl> controller());

    // Member access
    ClipFactory& clips();
    Globals& globals();
    GraphProcessor& graph();
    Transport* transport();

    // Engine methods
    AudioIODeviceCallback& getAudioIODeviceCallback() override;
    MidiInputCallback& getMidiInputCallback() override;
    
private:
    class Callback;
    Callback* cb;
    class Private;
    ScopedPointer<Private> priv;
    Globals& world;
};

typedef ReferenceCountedObjectPtr<AudioEngine> AudioEnginePtr;

}

#endif // ELEMENT_AUDIO_ENGINE_H

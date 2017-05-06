/*
    EngineControl.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_ENGINECONTROL_H
#define ELEMENT_ENGINECONTROL_H

#include "controllers/GraphController.h"
#include "engine/InternalFormat.h"
#include "session/Session.h"

namespace Element {
    class AudioEngine;
    class Globals;
    class Instrument;
    class InternalFormat;
    class Pattern;
    class Sequencer;

    class EngineControl :  public  GraphController,
                           public  ValueTree::Listener
    {
    public:
        typedef GraphProcessor::AudioGraphIOProcessor IOProcessor;

        ~EngineControl();

        virtual void setPlaying (bool p);
        virtual void setRecording (bool r);
        virtual void setTempo (double newTempo);

        bool open (Session& s);
        bool close();

        inline bool isOpen() const { return session.get() != nullptr; }

        GraphController* createSequenceController ();

    protected:
        friend class AudioEngine;
        EngineControl (AudioEngine& engine);

    private:
        /** Adds a plugin to the root graph */
        GraphNodePtr addRootPlugin (IOProcessor::IODeviceType ioType);
        GraphNodePtr addRootPlugin (InternalFormat::ID internal);
        InternalFormat* internals() const;
        Sequencer* sequencer() const;

        Globals& world;
        AudioEngine& engine;
        SessionRef session;

        Array<NodePtr> ioNodes;

        GraphNodePtr metro, audioOut, audioIn, midiOut, midiIn;
        GraphNodePtr seqNode;

        bool validateRequiredNodes();

        friend class ValueTree;
        void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
        void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
        void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int) override;
        void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int) override;
        void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override;
        void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override;
    };
}

#endif // ELEMENT_ENGINECONTROL_H

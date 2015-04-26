/*
    EngineControl.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ELEMENT_ENGINECONTROL_H
#define ELEMENT_ENGINECONTROL_H

#include "engine/InternalFormat.h"
#include "session/Session.h"

namespace Element {

    class AudioEngine;
    class Globals;
    class Instrument;
    class InternalFormat;
    class Pattern;

    class EngineControl :  public  GraphController,
                           public  ValueTree::Listener
    {
    public:

        typedef GraphProcessor::AudioGraphIOProcessor IOProcessor;
        typedef GraphProcessor::Node::Ptr NodePtr;

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
        NodePtr addRootPlugin (IOProcessor::IODeviceType ioType);
        NodePtr addRootPlugin (InternalFormat::ID internal);
        InternalFormat* internals() const;
        Sequencer* sequencer() const;

        Globals& world;
        AudioEngine& engine;
        SessionRef session;

        Array<NodePtr> ioNodes;

        NodePtr metro, audioOut, audioIn, midiOut, midiIn;
        NodePtr seqNode;

        bool validateRequiredNodes();

        friend class ValueTree;
        virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property);
        virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded);
        virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved);
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved);
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged);
    };

}

#endif // ELEMENT_ENGINECONTROL_H

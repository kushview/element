/*
    ClipFactory.cpp - This file is part of Element
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

#include "engine/Engine.h"
#include "engine/ClipFactory.h"
#include "engine/ClipSource.h"
#include "session/ClipModel.h"

namespace Element {

// a dummy clip that can be used for testing purposes
class DummyClip :  public ClipSource
{
    int32 blockSize;

public:

    DummyClip()
    {
        DBG ("DummyClip(): created");
        blockSize = 0;
    }

    ~DummyClip() { }

    void openClip (int, double) { Logger::writeToLog ("DummyClip::open()"); }
    void closeClip() { Logger::writeToLog ("DummyClip::close()"); }

    void renderClip (const Position& pos, AudioSourceChannelInfo&)
    {
        std::clog << "render: " << pos.timeInSeconds << std::endl;
    }

    void seekLocalFrame (const int64& frame) {
        std::clog << "Dummy Clip Seek: " << frame << std::endl;
    }

    void prepareToPlay (int block, double) { blockSize = block; }
    void releaseResources() { }

    void getNextAudioBlock (const AudioSourceChannelInfo &)
    {
        std::clog << "clip pos: " << getNextReadPosition() << "-" << getNextReadPosition() + blockSize << " len: " << frameLength() << std::endl;
    }

    class Type :  public ClipType
    {
    public:
        Type() { }
        ~Type() { }

        ClipData* createClipData (Engine&, const ClipModel&)
        {
            return new ClipData();
        }

        bool canCreateFrom (const ClipModel& model)
        {
            return model.getProperty (Slugs::type, String::empty) == String("dummy");
        }
        
        bool canCreateFrom (const File&) { return false; }
        ClipSource* createSource (Engine&, const File&) { return new DummyClip(); }
        ClipSource* createSource (Engine&, const ClipModel&) { return new DummyClip(); }
    };
};

class ClipFactory::Impl
{
public:

    Impl (ClipFactory& owner, Engine& e)
        : engine (e), factory (owner)
    {
        sampleRate.setValue ((double) 48000.f);
        randomHash.setSeedRandomly();
    }

    ~Impl()
    {
        sources.clear();
        data.clear();
        types.clear();
    }

    void attachSourceData (ClipType* type, ClipSource* source)
    {
        const ClipModel model (source->getModel());
        jassert (model.isValid());
        int64 hash = model.hashCode64();
        if (hash == 0) {
            hash = randomHash.nextInt64();
        }

        if (data.contains (hash))
        {
            while (! source->setData (data [hash])) { }
        }
        else if (ClipData* cd = type->createClipData (engine, model))
        {
            Shared<ClipData> sdata (cd);
            sdata->hash = hash;
            sdata->prepare (41000.0f, 1024);
            while (! source->setData (sdata)) { }
            data.set (hash, sdata);
        }
        else
        {
            assert (false);
        }
    }

    Engine&       engine;
    ClipFactory&  factory;

    OwnedArray<ClipSource> sources;
    OwnedArray<ClipType>   types;
    HashMap<int64, Shared<ClipData> > data;
    Value sampleRate;
    Random randomHash;
};


ClipFactory::ClipFactory (Engine& e)
{
    impl = new Impl (*this, e);
   #ifdef EL_USE_DUMMY_CLIPS
    registerType (new DummyClip::Type());
   #endif
}

ClipFactory::~ClipFactory()
{
    impl = nullptr;
}

ClipSource* ClipFactory::createSource (const ClipModel& model)
{
    ClipType* type = nullptr;
	for (int i = 0; i < impl->types.size(); ++i)
		if (ClipType* t = impl->types.getUnchecked(i))
			if (t->canCreateFrom (model))
				{ type = t; break; }

    ClipSource* source = type != nullptr ? type->createSource (impl->engine, model)
                                         : nullptr;
    if (source)
    {
        impl->sources.addIfNotAlreadyThere (source);
        source->parentRate.referTo (impl->sampleRate);
        source->setModel (model);
        impl->attachSourceData (type, source);
    }

    return source;
}

void ClipFactory::setSampleRate (const double rate)
{
    impl->sampleRate = rate;
}

ClipType* ClipFactory::getType (const int32 t) { return impl->types.getUnchecked (t); }
void ClipFactory::registerType (ClipType* type) { impl->types.addIfNotAlreadyThere (type); }
int32 ClipFactory::numTypes() const { return impl->types.size(); }

}

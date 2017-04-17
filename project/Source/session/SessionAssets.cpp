/*
    SessionAssets.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include "session/Session.h"
#include "session/MediaManager.h"
#include "EngineControl.h"
#include "Globals.h"

#include "session/SessionAssets.h"

namespace Element {

    SessionAssets::SessionAssets (Session& s)
        : AssetTree (s.node(), "Session Assets", "assets", nullptr),
          session (s), related (s, *this, s.globals().media())
    { }

    SessionAssets::~SessionAssets() { }

    void
    SessionAssets::assetAdded (const AssetTree::Item& item)
    {
        if (item.isGroup())
            return;

        MediaManager& media (session.globals().media());

        if (! media.canOpenFile (item.getFile()))
        {
            AssetTree::Item ritem (item);
            ritem.removeFromTree(); // a non supported file was added somehow
            return;
        }

        MediaPtr object;

        if (MediaManager::Document* doc = media.openFile (&session, item.getFile()))
        {
            if (MediaPtr m = doc->getMediaObject ())
            {
                related.mediaToLoad.add (m);
                //related.triggerAsyncUpdate();
                related.handleAsyncUpdate();
                object = m;
            }
        }
    }

    void
    SessionAssets::assetRemoved (const AssetTree::Item& item)
    {
    }



    SessionAssets::RelatedFileQueue::RelatedFileQueue (Session&s,
                                                       SessionAssets &o,
                                                       MediaManager& m)
        : session (s),
          assets (o),
          media (m)
    { }

    SessionAssets::RelatedFileQueue::~RelatedFileQueue() { }

    void
    SessionAssets::RelatedFileQueue::handleAsyncUpdate()
    {
        for (const MediaPtr& ptr : mediaToLoad)
            ptr->getRelatedFiles (filesToOpen);

        if (filesToOpen.size() > 0)
        {
            MediaPtr object;

            for (const File& file : filesToOpen)
            {
                if (MediaManager::Document* doc = media.openFile (&session, file))
                    object = doc->getMediaObject();
                
                if (! object)
                    continue;
#if 0
                if (InstrumentPtr i = dynamicPtrCast<Instrument> (object))
                {
                    assets.instruments.add (i);
                }
                else if (PatternPtr p = dynamicPtrCast<Pattern> (object))
                {
                    assets.patterns.add (p);
                }
#endif
            }
        }

        for (const MediaPtr& ptr : mediaToLoad)
        {
            ptr.get();
#if 0
            if (PatternPtr pat = dynamicPtrCast<Pattern> (ptr))
            {
                for (int t = 0; t < 16; ++t)
                {
                    File ifile = pat->getInstrumentFile (t);
                    if (ifile == File::nonexistent)
                        continue;

                    for (int i = assets.instruments.size(); --i >= 0;)
                    {
                        bool foundIt = false;
                        InstrumentPtr inst = assets.instruments.getUnchecked (i);
                        if (inst->getFile() == ifile) {
                            pat->setInstrument (t, inst);
                            foundIt = true;
                        }

                        if (foundIt)
                            break;
                    }
                }
            }
#endif
        }

        filesToOpen.clearQuick();
        mediaToLoad.clearQuick();
    }

}

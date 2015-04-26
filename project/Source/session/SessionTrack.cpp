/*
    SessionTrack.cpp - This file is part of Element
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


#include "../session/Session.h"
#include "../MediaManager.h"


namespace Element {

    ClipModel
    Session::Track::addClip (const File &file, double startTime)
    {
        ClipModel invalid (ValueTree::invalid);

        if (! supportsFile (file))
            return invalid;

        AssetItem root (session->assets().root());

        if (! root.findItemForFile (file).isValid())
        {
            if (! root.addFile (file, -1, false)) {
                std::clog << "couldn't create asset for file during clip addition\n";
                return invalid;
            }
        }

        AssetItem item (root.findItemForFile (file));

    #if 0
        if (PatternPtr pat = session->getPatternObject (item))
        {
            ClipModel clip (startTime, pat->totalBars() * 4 * Shuttle::PPQ);
            clip.node().setProperty ("media", mediaType(), nullptr);
            clip.node().setProperty (Slugs::assetId, item.getId(), nullptr);
            trackData.addChild (clip.node(), -1, session->undoManager());
            return clip;
        }
    #endif

        return invalid;
    }

    Session::Track Session::Track::next() const
    {
        return Track (session.get(), session->sequenceNode().getChild (
                          session->sequenceNode().indexOf (trackData) + 1));
    }

    Session::Track Session::Track::previous() const
    {
        return Track (session.get(), session->sequenceNode().getChild (
                      session->sequenceNode().indexOf (trackData) - 1));
    }

    void
    Session::Track::removeFromSession()
    {        
        trackData.removeAllChildren (undoManager());
        session->sequenceNode().removeChild (trackData, undoManager());
    }

    bool Session::Track::supportsAsset (const AssetItem &asset) const
    {
        return supportsFile (asset.getFile());
    }

    bool
    Session::Track::supportsClip (const ClipModel &clip) const
    {
        return clip.node().getProperty("media").equals (trackData.getProperty ("type"));
    }

    bool
    Session::Track::supportsFile (const File &file) const
    {
        return session->media().canOpenFile (file);
    }

}

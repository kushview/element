/*
    SessionAssets.h - This file is part of Element
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

#ifndef ELEMENT_SESSION_ASSETS_H
#define ELEMENT_SESSION_ASSETS_H

namespace Element {

class MediaManager;
class Session;

class SessionAssets :  public AssetTree
{
public:

    SessionAssets (Session& s);
    ~SessionAssets();

protected:

    friend class Session;
    Session& session;

    class RelatedFileQueue :  public AsyncUpdater
    {
    public:

        RelatedFileQueue (Session& s, SessionAssets& o, MediaManager& m);
        ~RelatedFileQueue();
        void handleAsyncUpdate();

        bool isEmpty() const {
            return 0 == (filesToOpen.size() + mediaToLoad.size());
        }

        inline void addFiles (const Array<File>& files) {
            filesToOpen.addArray (files);
        }

    private:

        friend class Session;
        friend class SessionAssets;

        Array<File> filesToOpen;
        Array<MediaPtr> mediaToLoad;

        Session&       session;
        SessionAssets& assets;
        MediaManager&  media;

    };

    friend class RelatedFileQueue;
    RelatedFileQueue related;

    void assetAdded (const AssetTree::Item& item);
    void assetRemoved (const AssetTree::Item& item);

    void printStats() const {

    }

};

}
#endif // ELEMENT_SESSION_ASSETS_H

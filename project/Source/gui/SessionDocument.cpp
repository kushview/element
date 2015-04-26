/*
    SessionDocument.cpp - This file is part of Element
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
#include "SessionDocument.h"

namespace Element {
namespace Gui {

    SessionDocument::SessionDocument (Session& s)
        : FileBasedDocument (".bts", "*.bts;*.xml", "Open Session", "Save Session"),
          session (s)
    {
        connection = session.signalChanged().connect (boost::bind (&SessionDocument::onSessionChanged, this));
    }

    SessionDocument::~SessionDocument()
    {
        connection.disconnect();
    }

    String
    SessionDocument::getDocumentTitle()
    {
        SessionRef sr = session.makeRef();
        return sr->name();
    }

    Result
    SessionDocument::loadDocument (const File& file)
    {
        SessionRef sr = session.makeRef();

        if (XmlElement* e = XmlDocument::parse (file))
        {
            ValueTree newData (ValueTree::fromXml (*e));
            delete e;

            if (! newData.isValid() && newData.hasType ("session"))
                return Result::fail ("Not a valid session file");

            if (sr->loadData (newData)) {
                sr->setName (file.getFileNameWithoutExtension());
                return Result::ok();
            }

            return Result::fail ("Could not load session data");
        }

        return Result::fail ("Could not read session file");
    }

    Result
    SessionDocument::saveDocument (const File& file)
    {
        SessionRef sr = session.makeRef();

        if (XmlElement* e = sr->createXml())
        {
            Result res = e->writeToFile (file, String::empty)
                    ? Result::ok() : Result::fail ("Error writing session file");
            delete e;
            return res;
        }

        return Result::fail ("Could not create session data");
    }

    File SessionDocument::getLastDocumentOpened() { return lastSession; }
    void SessionDocument::setLastDocumentOpened (const File& file) { lastSession = file; }

    void SessionDocument::onSessionChanged()
    {
        changed();
    }
}}

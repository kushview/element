/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include <element/session.hpp>
#include "gui/sessiondocument.hpp"

namespace element {

static void setMissingNodeProperties (const ValueTree& tree)
{
    if (tree.hasType (types::Node))
    {
        const Node node (tree, true);
        ignoreUnused (node);
    }
    else if (tree.hasType (types::Controller) || tree.hasType (types::Control))
    {
        // noop
    }
}

SessionDocument::SessionDocument (SessionPtr s)
    : FileBasedDocument (".els", "*.els", "Open Session", "Save Session"),
      session (s)
{
    if (session)
        session->addChangeListener (this);
}

SessionDocument::~SessionDocument()
{
    if (session)
        session->removeChangeListener (this);
}

String SessionDocument::getDocumentTitle()
{
    return (session != nullptr) ? session->getName() : "Unknown";
}

Result SessionDocument::loadDocument (const File& file)
{
    if (nullptr == session)
        return Result::fail ("No session data target");

    String error;
    if (auto e = XmlDocument::parse (file))
    {
        ValueTree newData (ValueTree::fromXml (*e));

        if (! newData.isValid() && newData.hasType (types::Session))
            error = "Not a valid session file";

        int loadedVersion = (int) newData.getProperty (tags::version, -1);
        if (error.isEmpty() && EL_SESSION_VERSION > loadedVersion)
            error = "Session was created with an old verison of Element. Loading not possible yet.";
        if (error.isEmpty() && ! session->loadData (newData))
            error = "Could not load session data";
    }
    else
    {
        error = "Not a valid session file";
    }

    if (error.isEmpty())
    {
        session->forEach (setMissingNodeProperties);
    }

    return (error.isNotEmpty()) ? Result::fail (error) : Result::ok();
}

Result SessionDocument::saveDocument (const File& file)
{
    if (! session)
        return Result::fail ("Nil session");

    session->saveGraphState();
    if (auto e = session->createXml())
    {
        Result res (e->writeTo (file)
                        ? Result::ok()
                        : Result::fail ("Error writing session file"));
        return res;
    }

    return Result::fail ("Could not create session data");
}

File SessionDocument::getLastDocumentOpened() { return lastSession; }
void SessionDocument::setLastDocumentOpened (const File& file) { lastSession = file; }

void SessionDocument::onSessionChanged()
{
    if (! session->notificationsFrozen())
    {
        changed();
    }
    else
    {
        setChangedFlag (false);
    }
}

void SessionDocument::changeListenerCallback (ChangeBroadcaster* cb)
{
    if (cb == session.get())
        onSessionChanged();
}
} // namespace element

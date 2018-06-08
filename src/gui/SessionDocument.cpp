/*
    SessionDocument.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "session/Session.h"
#include "gui/SessionDocument.h"

namespace Element {

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
        if (ScopedPointer<XmlElement> e = XmlDocument::parse (file))
        {
            ValueTree newData (ValueTree::fromXml (*e));
            if (! newData.isValid() && newData.hasType ("session"))
                error = "Not a valid session file";
            if (error.isEmpty() && !session->loadData (newData))
                error = "Could not load session data";
        }
        else
        {
            error = "Not a valid session file";
        }
        
        return (error.isNotEmpty()) ? Result::fail (error) : Result::ok();
    }

    Result SessionDocument::saveDocument (const File& file)
    {
        if (! session)
            return Result::fail ("Nil session");
        
        session->saveGraphState();
        if (ScopedPointer<XmlElement> e = session->createXml())
        {
            DBG("[EL] saving session to " << file.getFullPathName());
            Result res (e->writeToFile (file, String())
                    ? Result::ok() : Result::fail ("Error writing session file"));
            return res;
        }

        return Result::fail ("Could not create session data");
    }

    File SessionDocument::getLastDocumentOpened() { return lastSession; }
    void SessionDocument::setLastDocumentOpened (const File& file) { lastSession = file; }

    void SessionDocument::onSessionChanged()
    {
        if (! session->notificationsFrozen())
            changed();
        else
            setChangedFlag (false);
    }
    
    void SessionDocument::changeListenerCallback (ChangeBroadcaster* cb)
    {
        if (cb == session.get())
            onSessionChanged();
    }
}

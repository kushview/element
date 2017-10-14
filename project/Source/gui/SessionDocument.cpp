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

        if (ScopedPointer<XmlElement> e = XmlDocument::parse (file))
        {
            ValueTree newData (ValueTree::fromXml (*e));
            if (! newData.isValid() && newData.hasType ("session"))
                return Result::fail ("Not a valid session file");

            if (session->loadData (newData)) {
                session->setName (file.getFileNameWithoutExtension());
                return Result::ok();
            }

            return Result::fail ("Could not load session data");
        }

        return Result::fail ("Could not read session file");
    }

    Result SessionDocument::saveDocument (const File& file)
    {
        if (! session)
            return Result::fail ("Nil session");
        
        if (ScopedPointer<XmlElement> e = session->createXml())
        {
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
        changed();
    }
    
    void SessionDocument::changeListenerCallback (ChangeBroadcaster*)
    {
        onSessionChanged();
    }
}

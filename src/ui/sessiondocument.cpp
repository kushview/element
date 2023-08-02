// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/session.hpp>
#include "ui/sessiondocument.hpp"

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
        if (newData.isValid() && (int) newData.getProperty (tags::version, -1) != EL_SESSION_VERSION)
        {
            std::clog << "[element] migrate session...\n";
            newData = Session::migrate (newData, error);
        }

        if (error.isEmpty() && (! newData.isValid() || ! newData.hasType (types::Session)))
        {
            error = "Not a valid session file or type";
            if (newData.isValid())
                error << ": el." << newData.getType().toString();
        }

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

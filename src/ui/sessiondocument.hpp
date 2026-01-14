// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"
#include <element/session.hpp>

namespace element {
class SessionDocument : public FileBasedDocument,
                        public ChangeListener
{
public:
    SessionDocument (SessionPtr);
    ~SessionDocument();

    String getDocumentTitle() override;
    Result loadDocument (const File& file) override;
    Result saveDocument (const File& file) override;
    File getLastDocumentOpened() override;
    void setLastDocumentOpened (const File& file) override;
    File getSuggestedSaveAsFile (const File&) override
    {
        return getFile().getNonexistentSibling (true);
    }

    void changeListenerCallback (ChangeBroadcaster*) override;

private:
    SessionPtr session;
    File lastSession;
    friend class Session;
    void onSessionChanged();
};
} // namespace element

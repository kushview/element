/*
    SessionDocument.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Session.h"

namespace Element {

    class SessionDocument :  public FileBasedDocument
    {
    public:
        SessionDocument (SessionPtr);
        ~SessionDocument();

        String getDocumentTitle();
        Result loadDocument (const File& file);
        Result saveDocument (const File& file);
        File getLastDocumentOpened();
        void setLastDocumentOpened (const File& file);

    private:
        SessionPtr session;
        File lastSession;
        friend class Session;
        void onSessionChanged();
    };
}

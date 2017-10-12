/*
    SessionDocument.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_SESSION_DOCUMENT_H
#define ELEMENT_SESSION_DOCUMENT_H

#include "ElementApp.h"

namespace Element {

    class Session;

    class SessionDocument :  public FileBasedDocument
    {
    public:
        SessionDocument (Session& session);
        ~SessionDocument();

        String getDocumentTitle();
        Result loadDocument (const File& file);
        Result saveDocument (const File& file);
        File getLastDocumentOpened();
        void setLastDocumentOpened (const File& file);

    private:
        Session& session;
        File lastSession;
        friend class Session;
        void onSessionChanged();
    };
}

#endif // ELEMENT_SESSION_DOCUMENT_H

/*
    SessionDocument.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Session.h"

namespace Element {

    class SessionDocument :  public FileBasedDocument,
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
        
        void changeListenerCallback (ChangeBroadcaster*) override;
    private:
        SessionPtr session;
        File lastSession;
        friend class Session;
        void onSessionChanged();
    };
}

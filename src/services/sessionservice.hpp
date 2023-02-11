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

#pragma once

#include "services.hpp"
#include "documents/sessiondocument.hpp"
#include "session/session.hpp"
#include <element/signals.hpp>

namespace element {
class SessionService : public Service
{
public:
    SessionService();
    ~SessionService();

    void activate() override;
    void deactivate() override;

    void openDefaultSession();
    void openFile (const File& file);
    const File getSessionFile() const { return document != nullptr ? document->getFile() : File(); }
    void closeSession();
    void saveSession (const bool saveAs = false,
                      const bool askForFile = true,
                      const bool showError = true);
    void newSession();
    bool hasSessionChanged() { return (document) ? document->hasChangedSinceSaved() : false; }

    void resetChanges (const bool clearDocumentFile = false);

    void exportGraph (const Node& node, const File& targetFile);
    void importGraph (const File& file);

    Signal<void()> sessionLoaded;

private:
    SessionPtr currentSession;
    std::unique_ptr<SessionDocument> document;
    class ChangeResetter;
    std::unique_ptr<ChangeResetter> changeResetter;
    void loadNewSessionData();
    void refreshOtherControllers();
};

} // namespace element

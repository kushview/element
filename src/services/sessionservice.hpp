// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/services.hpp>
#include <element/session.hpp>
#include <element/signals.hpp>

#include "gui/sessiondocument.hpp"

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

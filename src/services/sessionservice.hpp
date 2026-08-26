// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>
#include <element/session.hpp>
#include <element/signals.hpp>

#include "ui/sessiondocument.hpp"

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
    const File getSessionFile() const;

    /** Closes the current session, leaving an empty, untitled session.

        Prompts to save first if the session has unsaved changes. Does nothing
        if the user cancels.
    */
    void closeSession();
    void saveSession (const bool saveAs = false,
                      const bool askForFile = true,
                      const bool showError = true);
    void newSession();
    bool hasSessionChanged();

    void resetChanges (const bool clearDocumentFile = false);

    void exportGraph (const Node& node, const File& targetFile);
    void importGraph (const File& file);

    Signal<void()> sigSessionLoaded;
    Signal<void()> sigWillSave;

private:
    SessionPtr currentSession;
    std::unique_ptr<SessionDocument> document;
    class ChangeResetter;
    std::unique_ptr<ChangeResetter> changeResetter;

    void loadNewSessionData();
    void refreshOtherControllers();

    /** Asks the user to save the session if it has unsaved changes.

        @return true if it is ok to discard or replace the current session,
                false if the user cancelled
    */
    bool saveIfNeededAndUserAgrees();
};

} // namespace element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <element/juce/gui_basics.hpp>
#include <element/services.hpp>
#include <element/session.hpp>
#include <element/signals.hpp>

#include "ui/sessiondocument.hpp"

namespace element {

class StartupGuard;

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

    /** Opens the initial session for a standalone launch.

        Reopens the last used session when the settings say so. If a previous
        launch died while opening that session, asks whether to open it anyway
        or start empty. If an autosaved recovery copy is newer than the session
        file, asks whether to recover it. The prompts are asynchronous, so the
        rest of startup continues in onFinished.

        @param onFinished Called once on the message thread when a session is
                          in place, whichever path was taken.
    */
    void openStartupSession (std::function<void()> onFinished);

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

    //==========================================================================
    /** Returns the recovery sidecar for a session file.

        @param sessionFile A session (.els) file, or any other File for the
                           untitled slot.
        @return `<name>.els.recover` next to the session, or the untitled
                recovery file under DataPath::recoveryDir().
    */
    static juce::File recoveryFileFor (const juce::File& sessionFile);

    /** Returns the recovery file used for sessions that have never been saved. */
    static juce::File untitledRecoveryFile();

    /** Returns the recovery file for the session currently open. */
    juce::File currentRecoveryFile() const;

    /** Returns true if a recovery file exists for the session currently open. */
    bool hasRecoveryFile() const;

    /** Writes the current session to its recovery file.

        Fires sigWillSave so editors flush, then writes atomically in the same
        format as a saved session. The document's changed flag is untouched.

        @return true if the file was written.
    */
    bool writeRecoveryFile();

    /** Loads the recovery file for a session and points the document at the
        session file, leaving it marked as changed so the user saves explicitly.
        Deletes the recovery file on success.

        @param sessionFile The session the recovery file belongs to, or an
                           invalid File for the untitled slot.
        @return true if the session was recovered.
    */
    bool recoverFrom (const juce::File& sessionFile);

    /** Deletes the recovery file of the session currently open, if any. */
    void deleteRecoveryFile();

    /** Writes the recovery file if the session has unsaved changes and the
        autosave interval has elapsed since the last write or save. */
    void autosaveIfNeeded();

    /** Sets the minimum time between autosave writes. Default is two minutes. */
    void setAutosaveInterval (juce::RelativeTime interval);

    /** Returns true while the autosave timer is running (standalone only). */
    bool isAutosaveEnabled() const;

    Signal<void()> sigSessionLoaded;
    Signal<void()> sigWillSave;

private:
    SessionPtr currentSession;
    std::unique_ptr<SessionDocument> document;
    class ChangeResetter;
    std::unique_ptr<ChangeResetter> changeResetter;

    class Autosave;
    std::unique_ptr<Autosave> autosave;
    juce::Time lastWrite;
    juce::RelativeTime autosaveInterval { 120.0 };

    std::unique_ptr<StartupGuard> startupGuard;
    juce::ScopedMessageBox startupBox;

    void loadNewSessionData();
    void refreshOtherControllers();
    void applyContentState();

    /** Asks the user to save the session if it has unsaved changes.

        @return true if it is ok to discard or replace the current session,
                false if the user cancelled
    */
    bool saveIfNeededAndUserAgrees();

    void openSessionWithRecovery (const juce::File& sessionFile, std::function<void()> onFinished);
    void openDefaultSessionWithRecovery (std::function<void()> onFinished);

    JUCE_DECLARE_WEAK_REFERENCEABLE (SessionService)
};

} // namespace element

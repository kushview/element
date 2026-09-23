// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/context.hpp>
#include <element/engine.hpp>
#include <element/graph.hpp>
#include <element/node.hpp>
#include <element/services.hpp>
#include <element/settings.hpp>
#include <element/ui.hpp>
#include <element/ui/content.hpp>

#include "services/mappingservice.hpp"
#include "services/presetservice.hpp"
#include "services/sessionservice.hpp"
#include "startupguard.hpp"

namespace element {

class SessionService::Autosave : public juce::Timer
{
public:
    explicit Autosave (SessionService& sc) : owner (sc) {}
    void timerCallback() override { owner.autosaveIfNeeded(); }

private:
    SessionService& owner;
};

class SessionService::ChangeResetter : public AsyncUpdater
{
public:
    explicit ChangeResetter (SessionService& sc) : owner (sc) {}
    ~ChangeResetter() = default;

    void handleAsyncUpdate() override
    {
        owner.resetChanges (false);
        jassert (! owner.hasSessionChanged());
    }

private:
    SessionService& owner;
};

SessionService::SessionService() {}
SessionService::~SessionService() {}

void SessionService::activate()
{
    currentSession = context().session();
    document.reset (new SessionDocument (currentSession));
    changeResetter.reset (new ChangeResetter (*this));
    document->setFile (DataPath::defaultSessionDir());

    autosave.reset (new Autosave (*this));
    lastWrite = Time::getCurrentTime();
    // A plugin instance never writes recovery files; the host owns persistence.
    if (getRunMode() == RunMode::Standalone)
        autosave->startTimer (30 * 1000);
}

void SessionService::deactivate()
{
    auto& world = context();
    auto& settings (world.settings());
    auto* props = settings.getUserSettings();

    if (autosave)
        autosave->stopTimer();
    autosave.reset();

    startupBox.close();
    if (startupGuard)
    {
        startupGuard->confirmClean();
        startupGuard->markCleanShutdown();
        startupGuard.reset();
    }

    if (document)
    {
        deleteRecoveryFile();
        if (document->getFile().existsAsFile())
            props->setValue (Settings::lastSessionKey, document->getFile().getFullPathName());
        document = nullptr;
    }

    changeResetter->cancelPendingUpdate();
    changeResetter.reset (nullptr);

    currentSession->clear();
    currentSession = nullptr;
}

void SessionService::openDefaultSession()
{
    if (auto* gc = sibling<GuiService>())
        gc->closeAllPluginWindows();

    loadNewSessionData();
    refreshOtherControllers();
    sibling<GuiService>()->stabilizeContent();
    resetChanges (true);
}

void SessionService::openFile (const File& file)
{
    bool didSomething = true;

    if (file.hasFileExtension ("elg"))
    {
        ValueTree data (Node::parse (file));
        String error;

        if (Node::isProbablyGraphNode (data))
        {
            Model model (data);

            if (model.version() != EL_GRAPH_VERSION)
                data = Node::migrate (model.data(), error);

            if (data.isValid() && error.isEmpty())
            {
                Node node (data, true);
                node.forEach ([] (const ValueTree& tree) {
                    if (! tree.hasType (types::Node))
                        return;
                    auto ref = tree;
                    ref.setProperty (tags::uuid, Uuid().toString(), nullptr);
                });

                if (auto* ec = sibling<EngineService>())
                    ec->addGraph (node, false);
            }
        }
        else
        {
            error = "File does not seem to be an Element graph.";
        }

        if (error.isNotEmpty())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Invalid graph", error);
        }
    }
    else if (file.hasFileExtension ("els"))
    {
        document->saveIfNeededAndUserAgrees();
        // The named session being replaced no longer needs its sidecar. An
        // untitled sidecar is left alone: at startup the document is pristine
        // and that file may belong to a session that was never saved.
        if (document->getFile().hasFileExtension ("els"))
            deleteRecoveryFile();

        Session::ScopedFrozenLock freeze (*currentSession);
        Result result = document->loadFrom (file, true);

        if (result.wasOk())
        {
            auto& gui = *sibling<GuiService>();
            gui.closeAllPluginWindows();
            refreshOtherControllers();
            applyContentState();
            sibling<GuiService>()->stabilizeContent();
            resetChanges();
            lastWrite = Time::getCurrentTime();
        }

        jassert (! hasSessionChanged());
    }
    else
    {
        didSomething = false;
    }

    if (didSomething)
    {
        if (auto* gc = sibling<GuiService>())
            if (! file.hasFileExtension ("els"))
                gc->stabilizeContent();
        changeResetter->triggerAsyncUpdate();
    }
}

const File SessionService::getSessionFile() const
{
    return document != nullptr ? document->getFile() : File();
}

void SessionService::exportGraph (const Node& node, const File& targetFile)
{
    if (! node.hasNodeType (types::Graph))
    {
        jassertfalse;
        return;
    }

    TemporaryFile tempFile (targetFile);
    if (node.writeToFile (tempFile.getFile()))
        tempFile.overwriteTargetFileWithTemporary();
}

void SessionService::importGraph (const File& file)
{
    openFile (file);
}

void SessionService::closeSession()
{
    jassert (document && currentSession);
    if (! saveIfNeededAndUserAgrees())
        return;

    deleteRecoveryFile();
    sibling<GuiService>()->closeAllPluginWindows();
    currentSession->clear();
    refreshOtherControllers();
    sibling<GuiService>()->stabilizeContent();
    resetChanges (true);
    lastWrite = Time::getCurrentTime();
}

bool SessionService::saveIfNeededAndUserAgrees()
{
    jassert (document);
    if (! document->hasChangedSinceSaved())
        return true;

    // - 0 if the third button was pressed ('cancel')
    // - 1 if the first button was pressed ('yes')
    // - 2 if the middle button was pressed ('no')
    const int res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon,
                                                     "Save Session?",
                                                     "The current session has changes. Would you like to save it?",
                                                     "Save Session",
                                                     "Don't Save",
                                                     "Cancel");
    switch (res)
    {
        case 1: // save: only proceed if the save actually completed
            return document->save (true, true) == FileBasedDocument::savedOk;
        case 2: // don't save
            return true;
        default: // cancel
            return false;
    }
}

bool SessionService::hasSessionChanged()
{
    return (document) ? document->hasChangedSinceSaved() : false;
}

void SessionService::resetChanges (const bool resetDocumentFile)
{
    jassert (document && currentSession);
    if (resetDocumentFile)
        document->setFile ({});
    // flush pending change messages so they don't re-flag the document afterwards
    currentSession->dispatchPendingMessages();
    document->setChangedFlag (false);
    jassert (! document->hasChangedSinceSaved());
}

void SessionService::saveSession (const bool saveAs, const bool askForFile, const bool showError)
{
    jassert (document && currentSession);
    auto result = FileBasedDocument::userCancelledSave;

    auto& gui = *sibling<GuiService>();

    if (auto* cc = gui.content())
    {
        String state;
        cc->getSessionState (state);
        auto ui = currentSession->data().getOrCreateChildWithName (tags::ui, nullptr);
        ui.setProperty ("content", state, nullptr);
    }

    sigWillSave();

    // Save As from an untitled session moves the sidecar slot; drop both.
    const auto staleRecovery = currentRecoveryFile();

    if (saveAs)
    {
        result = document->saveAsInteractive (true);
    }
    else
    {
        result = document->save (askForFile, showError);
    }

    if (result == FileBasedDocument::userCancelledSave)
        return;

    if (result == FileBasedDocument::savedOk)
    {
        // ensure change messages are flushed so the changed flag doesn't reset
        currentSession->dispatchPendingMessages();
        document->setChangedFlag (false);
        jassert (! hasSessionChanged());
        staleRecovery.deleteFile();
        deleteRecoveryFile();
        lastWrite = Time::getCurrentTime();
        if (auto* us = context().settings().getUserSettings())
            us->setValue (Settings::lastSessionKey, document->getFile().getFullPathName());

        if (saveAs)
        {
            sibling<UI>()->recentFiles().addFile (document->getFile());
            currentSession->data().setProperty (tags::name,
                                                document->getFile().getFileNameWithoutExtension(),
                                                nullptr);
        }
    }
}

void SessionService::newSession()
{
    jassert (document && currentSession);
    if (! saveIfNeededAndUserAgrees())
        return;

    deleteRecoveryFile();
    sibling<GuiService>()->closeAllPluginWindows();
    loadNewSessionData();
    refreshOtherControllers();
    sibling<GuiService>()->stabilizeContent();
    resetChanges (true);
    lastWrite = Time::getCurrentTime();
}

void SessionService::loadNewSessionData()
{
    currentSession->clear();
    const auto file = context().settings().getDefaultNewSessionFile();
    bool wasLoaded = false;

    if (file.existsAsFile())
    {
        ValueTree data;
        if (auto xml = XmlDocument::parse (file))
            data = ValueTree::fromXml (*xml);
        if (data.isValid() && data.hasType (types::Session) && EL_SESSION_VERSION == (int) data.getProperty (tags::version))
            wasLoaded = currentSession->loadData (data);
    }

    if (! wasLoaded)
    {
        auto engine = context().audio();
        int fallbackCount = 2;
        int numIn = engine != nullptr ? engine->getNumChannels (true) : fallbackCount;
        int numOut = engine != nullptr ? engine->getNumChannels (false) : fallbackCount;
        currentSession->clear();
        currentSession->addGraph (
            Graph::create ("Graph", numIn, numOut, true, true),
            true);
    }
}

void SessionService::refreshOtherControllers()
{
    sibling<EngineService>()->sessionReloaded();
    sibling<MappingService>()->refresh();
    sibling<MappingService>()->learn (false);
    sigSessionLoaded();
}

void SessionService::applyContentState()
{
    if (auto* gui = sibling<GuiService>())
    {
        if (auto* cc = gui->content())
        {
            auto ui = currentSession->data().getOrCreateChildWithName (tags::ui, nullptr);
            cc->applySessionState (ui.getProperty ("content").toString());
        }
    }
}

//==============================================================================
File SessionService::untitledRecoveryFile()
{
    return DataPath::recoveryDir().getChildFile ("untitled.els.recover");
}

File SessionService::recoveryFileFor (const File& sessionFile)
{
    if (sessionFile.hasFileExtension ("els"))
        return sessionFile.getSiblingFile (sessionFile.getFileName() + ".recover");
    return untitledRecoveryFile();
}

File SessionService::currentRecoveryFile() const
{
    return recoveryFileFor (document != nullptr ? document->getFile() : File());
}

bool SessionService::hasRecoveryFile() const
{
    return currentRecoveryFile().existsAsFile();
}

void SessionService::deleteRecoveryFile()
{
    currentRecoveryFile().deleteFile();
}

void SessionService::setAutosaveInterval (RelativeTime interval)
{
    autosaveInterval = interval;
}

bool SessionService::isAutosaveEnabled() const
{
    return autosave != nullptr && autosave->isTimerRunning();
}

bool SessionService::writeRecoveryFile()
{
    if (document == nullptr || currentSession == nullptr || currentSession->notificationsFrozen())
        return false;

    if (auto* gui = sibling<GuiService>())
    {
        if (auto* cc = gui->content())
        {
            String state;
            cc->getSessionState (state);
            auto ui = currentSession->data().getOrCreateChildWithName (tags::ui, nullptr);
            ui.setProperty ("content", state, nullptr);
        }
    }

    sigWillSave();
    currentSession->saveGraphState();

    auto xml = currentSession->createXml();
    if (xml == nullptr)
        return false;

    const auto target = currentRecoveryFile();
    target.getParentDirectory().createDirectory();

    TemporaryFile temp (target);
    const bool ok = xml->writeTo (temp.getFile()) && temp.overwriteTargetFileWithTemporary();
    if (! ok)
        Logger::writeToLog ("[session] could not write recovery file: " + target.getFullPathName());

    lastWrite = Time::getCurrentTime();
    return ok;
}

void SessionService::autosaveIfNeeded()
{
    if (document == nullptr || currentSession == nullptr)
        return;
    if (! document->hasChangedSinceSaved())
        return;
    if (Time::getCurrentTime() - lastWrite < autosaveInterval)
        return;
    if (currentSession->notificationsFrozen())
        return;
    // Never touch plugin state underneath a dialog.
    if (ModalComponentManager::getInstance()->getNumModalComponents() > 0)
        return;

    writeRecoveryFile();
}

bool SessionService::recoverFrom (const File& sessionFile)
{
    jassert (document && currentSession);
    const auto recovery = recoveryFileFor (sessionFile);
    if (! recovery.existsAsFile())
        return false;

    auto& gui = *sibling<GuiService>();
    gui.closeAllPluginWindows();

    Result result = Result::ok();
    {
        Session::ScopedFrozenLock freeze (*currentSession);
        result = document->loadFrom (recovery, false);
    }

    if (result.failed())
    {
        Logger::writeToLog ("[session] could not recover " + recovery.getFullPathName() + ": " + result.getErrorMessage());
        return false;
    }

    const bool named = sessionFile.hasFileExtension ("els");
    document->setFile (named ? sessionFile : File());
    document->setLastDocumentOpened (named ? sessionFile : File());

    refreshOtherControllers();
    applyContentState();
    gui.stabilizeContent();

    // Recovered content is unsaved by definition: flush queued change
    // messages and leave the document marked as changed.
    currentSession->dispatchPendingMessages();
    document->setChangedFlag (true);

    recovery.deleteFile();
    lastWrite = Time::getCurrentTime();

    if (named)
        if (auto* us = context().settings().getUserSettings())
            us->setValue (Settings::lastSessionKey, sessionFile.getFullPathName());

    Logger::writeToLog ("[session] recovered autosaved changes for " + (named ? sessionFile.getFullPathName() : String ("untitled session")));
    return true;
}

//==============================================================================
static MessageBoxOptions startupPrompt (GuiService* gui, const String& title, const String& message, const String& button0, const String& button1)
{
    auto options = MessageBoxOptions()
                       .withIconType (MessageBoxIconType::WarningIcon)
                       .withTitle (title)
                       .withMessage (message)
                       .withButton (button0)
                       .withButton (button1);
    if (gui != nullptr)
        if (auto* window = gui->getMainWindow())
            options = options.withAssociatedComponent (window);
    return options;
}

void SessionService::openStartupSession (std::function<void()> onFinished)
{
    if (getRunMode() != RunMode::Standalone)
    {
        jassertfalse; // the host owns the session in plugin mode
        if (onFinished)
            onFinished();
        return;
    }

    auto& settings = context().settings();
    File target;
    if (settings.openLastUsedSession())
    {
        const auto last = settings.getUserSettings()->getValue (Settings::lastSessionKey);
        if (File::isAbsolutePath (last) && File (last).existsAsFile())
            target = File (last);
    }

    startupGuard = std::make_unique<StartupGuard> (*settings.getUserSettings());
    if (startupGuard->previousRunWasUnclean())
        Logger::writeToLog ("[element] previous run did not shut down cleanly");

    const auto pending = startupGuard->pendingSession();
    if (pending != File() && pending != target)
    {
        Logger::writeToLog ("[element] clearing stale startup marker for " + pending.getFullPathName());
        startupGuard->confirmClean();
    }

    if (target.existsAsFile() && pending == target)
    {
        Logger::writeToLog ("[element] previous run died while opening " + target.getFullPathName());
        openDefaultSession();

        const auto options = startupPrompt (sibling<GuiService>(),
                                            "Safe Start",
                                            "Element did not finish starting last time while opening \"" + target.getFileName()
                                                + "\".\n\nOpen it anyway, or start with an empty session?",
                                            "Open Anyway",
                                            "Start Empty");

        startupBox = AlertWindow::showScopedAsync (options, [weak = WeakReference<SessionService> (this), target, onFinished] (int result) {
            auto* self = weak.get();
            if (self == nullptr || self->document == nullptr)
                return;
            if (result == 0)
            {
                self->openSessionWithRecovery (target, onFinished);
            }
            else
            {
                self->startupGuard->confirmClean();
                if (onFinished)
                    onFinished();
            }
        });
        return;
    }

    if (target.existsAsFile())
        openSessionWithRecovery (target, onFinished);
    else
        openDefaultSessionWithRecovery (onFinished);
}

void SessionService::openSessionWithRecovery (const File& sessionFile, std::function<void()> onFinished)
{
    jassert (startupGuard != nullptr);
    startupGuard->beginOpening (sessionFile);

    auto finish = [weak = WeakReference<SessionService> (this), onFinished]() {
        if (auto* self = weak.get())
            if (self->startupGuard)
                self->startupGuard->scheduleConfirm();
        if (onFinished)
            onFinished();
    };

    const auto recovery = recoveryFileFor (sessionFile);
    if (recovery.existsAsFile() && recovery.getLastModificationTime() > sessionFile.getLastModificationTime())
    {
        const auto options = startupPrompt (sibling<GuiService>(),
                                            "Recover Session?",
                                            "An autosaved copy of \"" + sessionFile.getFileName()
                                                + "\" is newer than the saved file.\n\nRecover the autosaved changes?",
                                            "Recover",
                                            "Discard");

        startupBox = AlertWindow::showScopedAsync (options, [weak = WeakReference<SessionService> (this), sessionFile, recovery, finish] (int result) {
            auto* self = weak.get();
            if (self == nullptr || self->document == nullptr)
                return;
            if (result == 0)
            {
                if (! self->recoverFrom (sessionFile))
                    self->openFile (sessionFile);
            }
            else
            {
                recovery.deleteFile();
                self->openFile (sessionFile);
            }
            finish();
        });
        return;
    }

    openFile (sessionFile);
    finish();
}

void SessionService::openDefaultSessionWithRecovery (std::function<void()> onFinished)
{
    const auto recovery = untitledRecoveryFile();
    if (recovery.existsAsFile())
    {
        const auto options = startupPrompt (sibling<GuiService>(),
                                            "Recover Session?",
                                            "An autosaved copy of an unsaved session was found.\n\nRecover it?",
                                            "Recover",
                                            "Discard");

        startupBox = AlertWindow::showScopedAsync (options, [weak = WeakReference<SessionService> (this), recovery, onFinished] (int result) {
            auto* self = weak.get();
            if (self == nullptr || self->document == nullptr)
                return;
            if (result != 0 || ! self->recoverFrom (File()))
            {
                recovery.deleteFile();
                self->openDefaultSession();
            }
            if (onFinished)
                onFinished();
        });
        return;
    }

    openDefaultSession();
    if (onFinished)
        onFinished();
}

} // namespace element

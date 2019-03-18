#pragma once

#include "session/Session.h"

namespace Element {

class AppController;

class SessionImportWizard final : public Component
{
public:
    SessionImportWizard();
    ~SessionImportWizard();

    void loadSession (const File& file);
    SessionPtr getSession();

    void paint (Graphics& g) override;
    void resized() override;

private:
    class Content; std::unique_ptr<Content> content;
    SessionPtr session;
};

class SessionImportWizardDialog : public DialogWindow
{
public:
    SessionImportWizardDialog (std::unique_ptr<Component>& h, const File& file);
    ~SessionImportWizardDialog();

    bool escapeKeyPressed() override;
    void closeButtonPressed() override;

    std::function<void(const Node&)> onGraphChosen;

private:
    std::unique_ptr<Component>& holder;
};

}

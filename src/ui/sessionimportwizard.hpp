// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/session.hpp>

namespace element {

class Services;

class SessionImportWizard final : public Component
{
public:
    SessionImportWizard();
    ~SessionImportWizard();

    void loadSession (const File& file);
    SessionPtr session();

    void paint (Graphics& g) override;
    void resized() override;

private:
    class Content;
    std::unique_ptr<Content> content;
    SessionPtr _session;
};

class SessionImportWizardDialog : public DialogWindow
{
public:
    SessionImportWizardDialog (std::unique_ptr<Component>& h, const File& file);
    ~SessionImportWizardDialog();

    bool escapeKeyPressed() override;
    void closeButtonPressed() override;

    std::function<void (const Node&)> onGraphChosen;

private:
    std::unique_ptr<Component>& holder;
};

} // namespace element

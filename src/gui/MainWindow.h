// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include <element/session.hpp>

namespace element {

class Services;
class Context;
class MainMenu;
class MainMenuBarModel;

class MainWindow : public DocumentWindow,
                   public ChangeListener
{
public:
    MainWindow (Context&);
    virtual ~MainWindow();
    void closeButtonPressed() override;
    void minimiseButtonPressed() override;

    void refreshMenu();
    Context& context() { return world; }
    Services& services();

    void changeListenerCallback (ChangeBroadcaster* source) override;
    void activeWindowStatusChanged() override;
    void refreshName();

private:
    friend class GuiService;

    Context& world;
    std::function<juce::String()> windowTitleFunction;
    std::unique_ptr<juce::MenuBarModel> mainMenu;
    void setMainMenuModel (std::unique_ptr<MainMenuBarModel> model);

    void nameChanged();
    void nameChangedSession();
};

} // namespace element

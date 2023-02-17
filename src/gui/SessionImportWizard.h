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

#include <element/session.hpp>

namespace element {

class ServiceManager;

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
    class Content;
    std::unique_ptr<Content> content;
    SessionPtr session;
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

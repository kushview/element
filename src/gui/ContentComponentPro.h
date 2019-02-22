/*
    ContentComponentPro.h - This file is part of Element
    Copyright (c) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class AppController;

class ContentComponentPro :  public ContentComponent
{
public:
    ContentComponentPro (AppController& app);
    ~ContentComponentPro() noexcept;

    void resizeContent (const Rectangle<int>& area) override;

    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;

    ValueTree getWorkspaceState() override;
    void applyWorkspaceState (const ValueTree&) override;

private:
    class Impl; std::unique_ptr<Impl> impl;
};

}

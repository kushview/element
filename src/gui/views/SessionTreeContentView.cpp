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

#include "gui/views/SessionTreeContentView.h"
#include "gui/SessionTreePanel.h"
#include "Common.h"

namespace Element {

SessionTreeContentView::SessionTreeContentView()
{
    tree.reset (new SessionTreePanel());
    addAndMakeVisible (tree.get());
}

SessionTreeContentView::~SessionTreeContentView()
{
    tree.reset();
}

void SessionTreeContentView::didBecomeActive()
{

}

void SessionTreeContentView::stabilizeContent()
{
    if (auto session = ViewHelpers::getSession (this))
        tree->setSession (session);
    resized();
    repaint();
}

void SessionTreeContentView::resized()
{
    tree->setBounds (getLocalBounds());
}

}

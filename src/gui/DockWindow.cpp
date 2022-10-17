/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "gui/Dock.h"
#include "gui/DockContainer.h"
#include "gui/DockWindow.h"

namespace element {

DockWindow::DockWindow (Dock& d, const int width, const int height)
    : DocumentWindow ("DockWindow", Colours::black, DocumentWindow::closeButton, false),
      dock (d)
{
    setUsingNativeTitleBar (true);
    setResizable (true, false);
    setAlwaysOnTop (true);
    container.reset (new DockContainer (dock));
    container->setSize (width, height);
    setContentNonOwned (container.get(), false);
    setContentComponentSize (container->getWidth(), container->getHeight());
}

DockWindow::~DockWindow()
{
    clearContentComponent();
    container.reset (nullptr);
}

bool DockWindow::contains (DockArea* area) const
{
    return container != nullptr ? container->contains (area)
                                : false;
}

bool DockWindow::dockItem (DockItem* const item, DockPlacement placement)
{
    if (! placement.isDirectional())
        return false;
    return container->dockItem (item, placement);
}

bool DockWindow::empty() const
{
    if (container == nullptr || container->getRootArea() == nullptr)
        return true;
    if (auto* const root = container->getRootArea())
    {
        for (int i = 0; i < root->getNumItems(); ++i)
        {
            if (auto* const item = dynamic_cast<DockItem*> (root->getItem (i)))
                if (item->getNumPanels() > 0)
                    return false;
        }
    }
    return true;
}

void DockWindow::closeButtonPressed()
{
    dock.windows.removeObject (this);
    dock.triggerAsyncUpdate();
}

int DockWindow::getDesktopWindowStyleFlags() const
{
    return DocumentWindow::getDesktopWindowStyleFlags();
}

} // namespace element

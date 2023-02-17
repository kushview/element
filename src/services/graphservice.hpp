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

#include <element/services.hpp>
#include "documents/graphdocument.hpp"
#include <element/signals.hpp>

namespace element {

/** Responsible for creating new, opening, and saving graph files in
    Element SE */
class GraphService final : public Service
{
public:
    GraphService() = default;
    ~GraphService() = default;

    void activate() override;
    void deactivate() override;

    bool hasGraphChanged() const { return document.hasChangedSinceSaved(); }
    const File getGraphFile() const { return document.getFile(); }
    Node getGraph() const { return document.getGraph(); }

    void openDefaultGraph();
    void openGraph (const File& file);
    void newGraph();
    void saveGraph (const bool saveAs);
    void loadGraph (const Node& graph);
    Signal<void()> graphChanged;

private:
    GraphDocument document;
    std::unique_ptr<Component> wizard;
    void refreshOtherControllers();
};

} // namespace element

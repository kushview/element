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

#if 0

#include "controllers/GraphManager.h"
#include "engine/nodes/SubGraphProcessor.h"
#include "Globals.h"

namespace Element
{

SubGraphProcessor::SubGraphProcessor ()
{
    setPlayConfigDetails (2, 2, 44100.f, 512);
}

SubGraphProcessor::~SubGraphProcessor()
{
    clear();
    controller = nullptr;
}

void SubGraphProcessor::initController (PluginManager& plugins)
{
    if (controller != nullptr)
        return;
    // controller = new GraphManager (*this, plugins);
}

void SubGraphProcessor::createAllIONodes() { }

void SubGraphProcessor::fillInPluginDescription (PluginDescription& d) const
{
    d.name                = "Graph";
    d.descriptiveName     = "A nested graph";
    d.pluginFormatName    = "Element";
    d.category            = "Utility";
    d.manufacturerName    = "Element";
    d.version             = ProjectInfo::versionString;
    d.fileOrIdentifier    = "element.graph";
    d.uniqueId            = (d.name + d.fileOrIdentifier).getHexValue32();
    d.isInstrument        = false;
    d.numInputChannels    = getTotalNumInputChannels();
    d.numOutputChannels   = getTotalNumOutputChannels();
    d.hasSharedContainer  = false;
}

}
#endif

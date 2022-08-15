/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Tests.h"

namespace Element {

class PresetScanTest : public UnitTestBase {
public:
    PresetScanTest() : UnitTestBase ("Preset Scanning", "presets", "scan") { }

    virtual void initialise() override
    {
        globals.reset (new Context());
        globals->getPluginManager().addDefaultFormats();
    }

    virtual void shutdown() override
    {
        globals.reset (nullptr);
    }

    virtual void runTest() override
    {
        beginTest ("scan presets");
        Node node;
        DataPath path;
        NodeArray nodes;
        path.findPresetsFor ("AudioUnit", "AudioUnit:Synths/aumu,samp,appl", nodes);
    }

private:
    std::unique_ptr<Context> globals;
    AudioPluginFormatManager plugins;
    AudioProcessor* createPluginProcessor()
    {
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;
        return plugins.createPluginInstance (desc, 44100.0, 1024, msg).release();
    }
};

static PresetScanTest sPresetScanTest;

}

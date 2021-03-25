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
#include "NodeManager.h"

using namespace Element;

class NodeManagerTests : public UnitTest
{
public:
    NodeManagerTests() : UnitTest ("NodeManager", "NodeManager") { }
    virtual ~NodeManagerTests() { }

    void runTest()
    {
        NodeManager nodes;
        const StringArray expectedIDs (
            EL_INTERNAL_ID_AUDIO_ROUTER,             
            EL_INTERNAL_ID_LUA,                      
            EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER,    
            EL_INTERNAL_ID_MIDI_MONITOR,            
            EL_INTERNAL_ID_MIDI_PROGRAM_MAP,        
            EL_INTERNAL_ID_MIDI_ROUTER,           
            // EL_INTERNAL_ID_MIDI_SEQUENCER,        
            EL_INTERNAL_ID_OSC_RECEIVER,           
            EL_INTERNAL_ID_OSC_SENDER,            
            EL_INTERNAL_ID_SCRIPT              
        );

        OwnedArray<PluginDescription> types;
        for (const auto& ID : expectedIDs)
        {
            beginTest (ID);
            expect (nodes.getKnownIDs().contains (ID));
            expect (nullptr != nodes.instantiate (ID));
            nodes.getPluginDescriptions (types, ID);
        }

        beginTest ("array sizes");
        expect (nodes.getKnownIDs().size() == expectedIDs.size());
        expect (types.size() == expectedIDs.size());

        for (const auto* tp : types)
        {
            beginTest (tp->name);
            expect (tp->name.isNotEmpty());
            expect (expectedIDs.contains (tp->fileOrIdentifier));
            expect (tp->pluginFormatName == EL_INTERNAL_FORMAT_NAME);
        }
    }
};

static NodeManagerTests sNodeManagerTests;

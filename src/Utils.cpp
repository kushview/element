/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "Utils.h"
#include "engine/nodes/BaseProcessor.h"

namespace Element {
namespace Util {

StringArray getFullVesrionPluginIdentifiers()
{
    return {
        EL_INTERNAL_ID_AUDIO_MIXER,
        EL_INTERNAL_ID_CHANNELIZE,
        EL_INTERNAL_ID_MIDI_CHANNEL_MAP,
        EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER,
        EL_INTERNAL_ID_GRAPH,
        
        EL_INTERNAL_ID_AUDIO_ROUTER,
        EL_INTERNAL_ID_MEDIA_PLAYER,
        EL_INTERNAL_ID_MIDI_PROGRAM_MAP,
        EL_INTERNAL_ID_PLACEHOLDER
    };
}

}}

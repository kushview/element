
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

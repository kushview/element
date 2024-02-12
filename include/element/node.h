
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define EL_NODE_FORMAT_NAME "Element"

#ifndef EL_NODE_FORMAT_AUTHOR
    #define EL_NODE_FORMAT_AUTHOR "Element"
#endif

// Nodes backed by juce::AudioProcessor
#define EL_NODE_ID_ALLPASS_FILTER     "element.allPass"
#define EL_NODE_ID_AUDIO_FILE_PLAYER  "element.audioFilePlayer"
#define EL_NODE_ID_AUDIO_MIXER        "element.audioMixer"
#define EL_NODE_ID_CHANNELIZE         "element.channelize"
#define EL_NODE_ID_COMB_FILTER        "element.comb"
#define EL_NODE_ID_COMPRESSOR         "element.compressor"
#define EL_NODE_ID_EQ_FILTER          "element.eqfilt"
#define EL_NODE_ID_FREQ_SPLITTER      "element.freqsplit"
#define EL_NODE_ID_MEDIA_PLAYER       "element.mediaPlayer"
#define EL_NODE_ID_MIDI_CHANNEL_MAP   "element.midiChannelMap"
#define EL_NODE_ID_MIDI_INPUT_DEVICE  "element.midiInputDevice"
#define EL_NODE_ID_MIDI_OUTPUT_DEVICE "element.midiOutputDevice"
#define EL_NODE_ID_PLACEHOLDER        "element.placeholder"
#define EL_NODE_ID_REVERB             "element.reverb"
#define EL_NODE_ID_WET_DRY            "element.wetDry"
#define EL_NODE_ID_VOLUME             "element.volume"

// Processor subclass
#define EL_NODE_ID_AUDIO_ROUTER          "element.audioRouter"
#define EL_NODE_ID_GRAPH                 "element.graph"
#define EL_NODE_ID_MIDI_CHANNEL_SPLITTER "element.midiChannelSplitter"
#define EL_NODE_ID_MIDI_MONITOR          "element.midiMonitor"
#define EL_NODE_ID_MIDI_PROGRAM_MAP      "element.programChangeMap"
#define EL_NODE_ID_MIDI_ROUTER           "element.midiRouter"
#define EL_NODE_ID_MIDI_SEQUENCER        "element.midiSequencer"
#define EL_NODE_ID_OSC_RECEIVER          "element.oscReceiver"
#define EL_NODE_ID_OSC_SENDER            "element.oscSender"
#define EL_NODE_ID_SCRIPT                "element.script"
#define EL_NODE_ID_MCU                   "el.MCU"
#define EL_NODE_ID_MIDI_SET_LIST         "element.midiSetList"

//==============================================================================
#define EL_NODE_UID_AUDIO_FILE_PLAYER     1000
#define EL_NODE_UID_AUDIO_MIXER           1001
#define EL_NODE_UID_AUDIO_ROUTER          1002
#define EL_NODE_UID_COMB_FILTER           1003
#define EL_NODE_UID_CHANNELIZE            1004
#define EL_NODE_UID_GRAPH                 1005
#define EL_NODE_UID_MEDIA_PLAYER          1006
#define EL_NODE_UID_MIDI_CHANNEL_MAP      1007
#define EL_NODE_UID_MIDI_CHANNEL_SPLITTER 1008
#define EL_NODE_UID_MIDI_PROGRAM_MAP      1009
#define EL_NODE_UID_MIDI_SEQUENCER        1010
#define EL_NODE_UID_PLACEHOLDER           1011
#define EL_NODE_UID_REVERB                1012
#define EL_NODE_UID_WET_DRY               1013
#define EL_NODE_UID_MIDI_INPUT_DEVICE     1014
#define EL_NODE_UID_MIDI_OUTPUT_DEVICE    1015
#define EL_NODE_UID_MIDI_MONITOR          1016
#define EL_NODE_UID_OSC_RECEIVER          1017
#define EL_NODE_UID_OSC_SENDER            1018
#define EL_NODE_UID_EQ_FILTER             1019
#define EL_NODE_UID_FREQ_SPLITTER         1020
#define EL_NODE_UID_COMPRESSOR            1022
#define EL_NODE_UID_MIDI_ROUTER           1023
#define EL_NODE_UID_SCRIPT                1024
#define EL_NODE_UID_ALLPASS_FILTER        1025
#define EL_NODE_UID_VOLUME                1026
#define EL_NODE_UID_MCU                   1027
#define EL_NODE_UID_MIDI_SET_LIST         1028

#ifdef __cplusplus
}
#endif

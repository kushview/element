// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ELEMENT_H_INCLUDED
#define ELEMENT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    #define EL_EXTERN extern "C"
#else
    #define EL_EXTERN
#endif

#ifdef _WIN32
    // windows exports
    #if defined(EL_SHARED_BUILD)
        #define EL_API __declspec (dllexport)
        #pragma warning(disable : 4251)
    #elif defined(EL_SHARED)
        #define EL_API __declspec (dllimport)
        #pragma warning(disable : 4251)
    #endif
    #define EL_PLUGIN_EXPORT EL_EXTERN __declspec (dllexport)
#else
    #if defined(EL_SHARED) || defined(EL_SHARED_BUILD)
        #define EL_API __attribute__ ((visibility ("default")))
    #endif
    #define EL_PLUGIN_EXPORT EL_EXTERN __attribute__ ((visibility ("default")))
#endif

#define EL_EXPORT EL_EXTERN EL_API

#ifndef EL_API
    #define EL_API
#endif

// export macro: includes extern C if needed followed by visibility attribute;
#ifndef EL_EXPORT
    #define EL_EXPORT
#endif

//==============================================================================
#define EL_MT_AUDIO_BUFFER_64 "el.AudioBuffer64"
#define EL_MT_AUDIO_BUFFER_32 "el.AudioBuffer32"
#define EL_MT_BYTES           "el.Bytes"
#define EL_MT_MIDI_MESSAGE    "el.MidiMessage"
#define EL_MT_MIDI_BUFFER     "el.MidiBuffer"
#define EL_MT_MIDI_PIPE       "el.MidiPipe"
#define EL_MT_VECTOR          "el.Vector"

#ifdef __cplusplus
} // extern "C"
#endif

#endif

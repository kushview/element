#ifndef ELEMENT_H_INCLUDED
#define ELEMENT_H_INCLUDED

#include <stdint.h>

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
    #ifdef EL_DLLEXPORT
        #define EL_API __declspec(dllexport)
    #else
        #define EL_API __declspec(dllimport)
    #endif
    #define EL_EXPORT        EL_EXTERN EL_API
    #define EL_PLUGIN_EXPORT EL_EXTERN __declspec(dllexport)
#else
    // *nix exports
    #define EL_API           __attribute__ ((visibility ("default")))
    #define EL_EXPORT        EL_EXTERN EL_API
    #define EL_PLUGIN_EXPORT EL_EXTERN EL_API
#endif

// export macro: includes extern C if needed followed by visibility attribute;
#ifndef EL_EXPORT
    #define EL_EXPORT
#endif

//=============================================================================
typedef enum {
    EL_PORT_TYPE_AUDIO,
    EL_PORT_TYPE_CV,
    EL_PORT_TYPE_ATOM,
    EL_PORT_TYPE_MIDI,
    EL_PORT_TYPE_VIDEO,
    EL_PORT_TYPE_UNKNOWN
} elPortType;

//=============================================================================
#define EL_MT_AUDIO_BUFFER_64 "el.AudioBuffer64"
#define EL_MT_AUDIO_BUFFER_32 "el.AudioBuffer32"
#define EL_MT_BYTE_ARRAY      "el.ByteArray"
#define EL_MT_MIDI_MESSAGE    "el.MidiMessage"
#define EL_MT_MIDI_BUFFER     "el.MidiBuffer"
#define EL_MT_MIDI_PIPE       "el.MidiPipe"
#define EL_MT_VECTOR          "el.Vector"

//=============================================================================
struct elContext;
typedef struct elContext elContext;

EL_EXPORT elContext* element_context_new();
EL_EXPORT void element_context_free (elContext* ctx);

#ifdef __cplusplus
} // extern "C"
#endif

#endif


#pragma once

#ifndef EL_USE_DATA_PATH_TREE
 #define EL_USE_DATA_PATH_TREE 1
#endif

#ifndef EL_USE_PRESETS
 #define EL_USE_PRESETS 1
#endif

#ifndef EL_USE_SUBGRAPHS
 #define EL_USE_SUBGRAPHS 0
#endif

#ifndef EL_ROOT_MIDI_CHANNEL
 #define EL_ROOT_MIDI_CHANNEL 1
#endif

#ifdef _MSC_VER
 #pragma warning(disable: 4100) // unreferenced formal param
 #pragma warning(disable: 4244) // convert possible data loss
 #pragma warning(disable: 4245) // signed/unsigned mismatch
 #pragma warning(disable: 4373) // virtual override params minor differences
 #pragma warning(disable: 4702) // unreachable code
 #pragma warning(disable: 4505) // unreferenced local function has been removed
 #pragma warning(disable: 4457) // declaration of 'xxx' hides function parameter
 #pragma warning(disable: 4458) // declaration of 'xxx' hides class member
 #pragma warning(disable: 4389) // signed/unsigned mismatch
#endif

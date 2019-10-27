/*
    This file is part of Element
    Copyright (c) 2019 Kushview, LLC.  All rights reserved.

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

#pragma once

#ifndef EL_USE_DATA_PATH_TREE
 #define EL_USE_DATA_PATH_TREE 1
#endif

#ifndef EL_USE_PRESETS
 #define EL_USE_PRESETS 1
#endif

#ifndef EL_USE_SUBGRAPHS
 #define EL_USE_SUBGRAPHS 1
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

#if defined(EL_FREE) && defined(EL_SOLO)
 #pragma error "Cannot enable EL_FREE and EL_SOLO at the same time"
#endif

#if !defined(EL_FREE) && !defined(EL_SOLO)
 #define EL_PRO 1
#endif

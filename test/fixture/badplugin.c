// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

/* A deliberately misbehaving "plugin" used by PluginScannerTests to verify
   the scanner survives plugins that crash or hang the worker process.

   Built as a VST3-shaped module so the scanner worker's load runs this
   code on attach. EL_TEST_HANG selects hanging instead of crashing. */

#include <stdlib.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL WINAPI DllMain (HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
#if defined(EL_TEST_HANG)
        for (;;)
            Sleep (1000);
#else
        abort();
#endif
    }

    return TRUE;
}

#else

#include <unistd.h>

__attribute__ ((constructor)) static void badplugin_entry (void)
{
#if defined(EL_TEST_HANG)
    for (;;)
        sleep (1);
#else
    abort();
#endif
}

#endif

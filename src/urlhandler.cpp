// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_WINDOWS

#include <element/application.hpp>
#include "log.hpp"

#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
 #define NOMINMAX
#endif
#include <windows.h>

using namespace juce;

namespace element {

void Application::registerURLSchemeHandler()
{
    const auto exePath = File::getSpecialLocation (File::currentExecutableFile).getFullPathName();
    const auto cmd = "\"" + exePath + "\" \"%1\"";

    HKEY hkey;
    DWORD disposition;
    const wchar_t* keyPath = L"Software\\Classes\\element";

    // Create the element key
    if (::RegCreateKeyExW (HKEY_CURRENT_USER, keyPath, 0, nullptr, 0, KEY_WRITE, nullptr, &hkey, &disposition) == ERROR_SUCCESS)
    {
        // Set default value to "URL:Element Protocol"
        ::RegSetValueExW (hkey, nullptr, 0, REG_SZ, (const BYTE*)L"URL:Element Protocol", sizeof (wchar_t) * 20);

        // Set "URL Protocol" value to empty string
        ::RegSetValueExW (hkey, L"URL Protocol", 0, REG_SZ, (const BYTE*)L"", sizeof (wchar_t));

        ::RegCloseKey (hkey);
    }

    // Create the shell\open\command subkey
    const std::wstring fullPath = std::wstring (keyPath) + L"\\shell\\open\\command";
    if (::RegCreateKeyExW (HKEY_CURRENT_USER, fullPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hkey, &disposition) == ERROR_SUCCESS)
    {
        const std::wstring cmdW = cmd.toWideCharPointer();
        ::RegSetValueExW (hkey, nullptr, 0, REG_SZ, (const BYTE*)cmdW.c_str(), (DWORD)((cmdW.length() + 1) * sizeof (wchar_t)));
        ::RegCloseKey (hkey);
    }

    Logger::writeToLog ("URL scheme handler (Windows Registry) registered");
}

void Application::unregisterURLSchemeHandler()
{
    ::RegDeleteTreeW (HKEY_CURRENT_USER, L"Software\\Classes\\element");
}

} // namespace element

#endif

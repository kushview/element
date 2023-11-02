// Copyright 2023 Kushview, LLC <info@kushview.net>
// Author: @fbeguec on forum.juce.com
// Modified: @gettdunne on forum.juce.com (JUCE v5 support)
// Modified: Integrated in with Element. LiquidSky <paolo.onersi@outlook.it>
// Thread: https://forum.juce.com/t/jbridge-x64-bridge-for-x86-vst-support-in-juce-vst-host-windows/11608
// SPDX-License-Identifier: GPL3-or-later

// JBridge 
#ifdef _WIN32

#define JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN if (moduleMain == nullptr) \
{ \
    if(module.getNativeHandle() == nullptr && file.existsAsFile()) \
    { \
        DynamicLibrary proxyModule (WindowsRegistry::getValue ("HKEY_LOCAL_MACHINE\\Software\\JBridge\\Proxy64", "")); \
        if(proxyModule.getNativeHandle() != nullptr) \
            *((unsigned __int64*)&customMain) = 0xfffffffffffffffe;\
    } \
    return customMain != nullptr;\
}

#define JUCE_VST_WRAPPER_INVOKE_MAIN typedef Vst2::AEffect * (VSTCALLBACK *PFNBRIDGEMAIN)( Vst2::audioMasterCallback audiomaster_, char * pszPluginPath ); \
if (module->moduleMain) \
{ \
    effect = module->moduleMain (&audioMaster); \
} \
else if ((unsigned __int64)(module->customMain) == 0xfffffffffffffffe) \
{ \
    PFNBRIDGEMAIN jBridgeMain = 0; \
    if(module->module.getNativeHandle() == nullptr) \
        module->module.open(WindowsRegistry::getValue ("HKEY_LOCAL_MACHINE\\Software\\JBridge\\Proxy64", "")); \
    if(module->module.getNativeHandle() != nullptr) \
        jBridgeMain = (PFNBRIDGEMAIN) (module->module.getFunction("BridgeMain")); \
    char plug32bitPath[_MAX_PATH]; \
    plug32bitPath[0] = 0; \
    strcpy (plug32bitPath, module->file.getFullPathName().toRawUTF8()); \
    if (jBridgeMain) \
        effect = (Vst2::AEffect*)(jBridgeMain (&audioMaster, plug32bitPath));\
}
#endif

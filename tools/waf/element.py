#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, platform
from waflib.Configure import conf

juce_modules = '''
    juce_analytics juce_audio_basics juce_audio_devices juce_audio_formats
    juce_audio_processors juce_audio_utils juce_core juce_cryptography
    juce_data_structures juce_events juce_graphics juce_gui_basics
    juce_gui_extra juce_product_unlocking kv_core kv_edd kv_engines
    kv_gui kv_lv2 kv_models
'''

mingw_libs = '''
    uuid wsock32 wininet version ole32 ws2_32 oleaut32
    imm32 comdlg32 shlwapi rpcrt4 winmm gdi32
'''

@conf 
def check_common (self):
    self.check(header_name='stdbool.h', mandatory=True)

    # VST support
    line_just = self.line_just
    self.check(header_name='pluginterfaces/vst2.x/aeffect.h', 
               mandatory=False, uselib_store="VST")
    self.line_just = line_just

    # LV2 Support
    # self.env.append_unique ('CXXFLAGS', ["-I../../libs/lv2"])
    # self.check(header_name='lv2/lv2plug.in/ns/lv2core/lv2.h', mandatory=True)
    self.check(header_name='lilv.h', use='LV2', mandatory=False)
    self.check_cxx(lib = 'lilv', use='LV2', mandatory=False)
    self.check(header_name='suil.h', use='LV2', mandatory=False)
    self.check_cxx(lib = 'suil', use='LV2', mandatory=False)

@conf
def check_mingw (self):
    for l in mingw_libs.split():
        self.check_cxx(lib=l, uselib_store=l.upper())
    self.define('JUCE_PLUGINHOST_VST3', 0)
    self.define('JUCE_PLUGINHOST_VST', bool(self.env.HAVE_VST))
    self.define('JUCE_PLUGINHOST_AU', 0)
    for flag in '-Wno-multichar -Wno-deprecated-declarations'.split():
        self.env.append_unique ('CFLAGS', [flag])
        self.env.append_unique ('CXXFLAGS', [flag])

@conf
def check_mac (self):
    # VST/VST3 OSX Support
    self.define('JUCE_PLUGINHOST_VST3', 1)
    self.define('JUCE_PLUGINHOST_VST', bool(self.env.HAVE_VST))

@conf
def check_linux (self):
    return

def get_mingw_libs():
    return [ l.upper() for l in mingw_libs.split() ]

def get_juce_library_code (prefix, extension='.cpp'):
    cpp_only = [ 'juce_analytics' ]
    code = []
    for f in juce_modules.split():
        e = '.cpp' if f in cpp_only else extension
        code.append (prefix + '/include_' + f + e)
    return code

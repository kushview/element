#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, platform
from waflib.Configure import conf
import juce

juce_modules = '''
    jlv2_host juce_audio_basics juce_audio_devices juce_audio_formats
    juce_audio_processors juce_audio_utils juce_core juce_cryptography
    juce_data_structures juce_events juce_graphics juce_gui_basics
    juce_gui_extra kv_core kv_engines kv_gui kv_models
'''

mingw_libs = '''
    uuid wsock32 wininet version ole32 ws2_32 oleaut32
    imm32 comdlg32 shlwapi rpcrt4 winmm gdi32
'''

@conf 
def check_common (self):
    self.check(lib='curl', mandatory=False)
    self.check(header_name='stdbool.h', mandatory=True)
    self.check(header_name='boost/signals2.hpp', mandatory=True, uselib_store="BOOST_SIGNALS")

    # JACK
    self.check_cfg(package='jack', uselib_store="JACK", args='--cflags --libs', mandatory=False)
    self.env.JACK = bool(self.env.HAVE_JACK) and not self.options.no_jack
    self.define('KV_JACK_AUDIO', self.env.JACK)

    # VST support
    line_just = self.line_just
    self.check(header_name='pluginterfaces/vst2.x/aeffect.h', 
               mandatory=False, uselib_store="VST")
    self.define('JUCE_PLUGINHOST_VST', bool(self.env.HAVE_VST))
    self.line_just = line_just

    # LV2 Support
    self.check_cfg(package='lv2', uselib_store="LV2", args='--cflags', mandatory=False)
    self.check_cfg(package='lilv-0', uselib_store="LILV", args='--cflags --libs', mandatory=False)
    self.check_cfg(package='suil-0', uselib_store="SUIL", args='--cflags --libs', mandatory=False)
    self.env.LV2 = bool(self.env.HAVE_LILV) and bool(self.env.HAVE_LILV)
    self.define('JLV2_PLUGINHOST_LV2', self.env.LV2)

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

@conf
def check_linux (self):
    self.check(lib='pthread', mandatory=True)
    self.check(lib='dl', mandatory=True)
    self.check(header_name='curl/curl.h', uselib_store='CURL', mandatory=True)
    self.check(lib='curl', uselib_store='CURL', mandatory=True)
    self.check(header_name='ladspa.h', uselib_store='LADSPA', mandatory=False)
    self.define('JUCE_PLUGINHOST_LADSPA', bool(self.env.HAVE_LADSPA))
    self.check_cfg(package='freetype2', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='x11', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='xext', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='xrandr', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='xcomposite', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='xinerama', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='xcursor', args='--cflags --libs', mandatory=True)
    self.check_cfg(package='alsa', args='--cflags --libs', mandatory=True)

def get_mingw_libs():
    return [ l.upper() for l in mingw_libs.split() ]

def get_juce_library_code (prefix, ext=''):
    extension = ext
    if len(ext) <= 0:
        if juce.is_mac():
            extension = '.mm'
        else:
            extension = '.cpp'

    cpp_only = [ 'juce_analytics', 'jlv2_host' ]
    code = []
    for f in juce_modules.split():
        e = '.cpp' if f in cpp_only else extension
        code.append (prefix + '/include_' + f + e)
    return code

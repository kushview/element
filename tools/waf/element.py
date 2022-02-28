#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, platform
from waflib.Configure import conf
import juce

# Package name
APPNAME="element"
# Version number
VERSION="0.47.0"
# plugin version is +1 that of standalone. VST2 has to start at 1.0.0
PLUGIN_VERSION="1.47.0"

JUCE_VST3SDK_PATH = 'libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

juce_modules = '''
    jlv2_host juce_audio_basics juce_audio_devices juce_audio_formats
    juce_audio_processors juce_audio_utils juce_core juce_cryptography
    juce_data_structures juce_dsp juce_events juce_graphics juce_gui_basics
    juce_gui_extra juce_osc kv_core kv_engines kv_gui kv_models
'''

mingw_libs = '''
    uuid wsock32 wininet version ole32 ws2_32 oleaut32
    imm32 comdlg32 shlwapi rpcrt4 winmm gdi32
'''

@conf
def check_ladspa (self):
    self.env.LADSPA = not bool(self.options.no_ladspa)
    if self.env.LADSPA:
        self.check(header_name='ladspa.h', uselib_store='LADSPA', mandatory=False)
        self.env.LADSPA = bool(self.env.HAVE_LADSPA)
    self.define('JUCE_PLUGINHOST_LADSPA', self.env.LADSPA)

@conf
def check_curl (self):
    self.check (header_name='curl/curl.h', uselib_store='CURL', mandatory=self.host_is_linux())
    self.check (lib='curl', uselib_store='CURL', mandatory=self.host_is_linux())
    self.define ('JUCE_USE_CURL', bool (self.env.HAVE_CURL))

@conf
def check_filesystem (self):
    self.env.USE_LIBCXXFS = False
    self.check (header_name='filesystem', uselib_store='FILESYSTEM', mandatory=False)
    if self.env.HAVE_FILESYSTEM:
        return

    self.env.USE_LIBCXXFS = True
    self.check (header_name='experimental/filesystem', uselib_store='FILESYSTEM', mandatory=True)
    self.check (lib='c++fs' if self.host_is_mac() else 'stdc++fs', uselib_store='FILESYSTEM', mandatory=True)

@conf
def check_common (self):
    self.check_filesystem()
    self.check_curl()
    self.check (header_name='stdbool.h', mandatory=True)

    # Boost
    self.check (header_name='boost/signals2.hpp', 
                msg="Checking for Boost.Signals2", 
                mandatory=True, uselib_store="BOOST_SIGNALS",
                use=['DEPENDS'])
    
    if self.env.TEST:
        self.check_cxx (
            msg = "Checking for Boost.Test",
            fragment = '''
                #define BOOST_TEST_MODULE ElementConfigure
                #include <boost/test/included/unit_test.hpp>
            ''',
            execute = False,
            uselib_store = 'BOOST_TEST',
            define_name = 'HAVE_BOOST_TEST',
            mandatory = False,
            use = ['DEPENDS']
        )

    # Web Browser
    self.define ('JUCE_WEB_BROWSER', 0)

    # JACK
    self.check_cfg(package='jack', uselib_store="JACK", args='--cflags --libs', mandatory=False)
    self.env.JACK = bool(self.env.HAVE_JACK) and not bool(self.options.no_jack)
    self.define('KV_JACK_AUDIO', self.env.JACK)

    # VST2 host support
    self.env.VST = False
    if not bool (self.options.no_vst):
        if isinstance(self.options.vstsdk24, str) and len(self.options.vstsdk24) > 0:
            self.env.append_unique ('CFLAGS', [ '-I%s' % self.options.vstsdk24 ])
            self.env.append_unique ('CXXFLAGS', [ '-I%s' % self.options.vstsdk24 ])

        line_just = self.line_just
        self.check(header_name='pluginterfaces/vst2.x/aeffect.h',  uselib_store='AEFFECT_H',  mandatory=False)
        self.check(header_name='pluginterfaces/vst2.x/aeffectx.h', uselib_store='AEFFECTX_H', mandatory=False)
        self.env.VST = bool(self.env.HAVE_AEFFECT_H) and bool(self.env.HAVE_AEFFECTX_H)
        if not self.env.VST:
            # check for distrho... somehow?
            pass
        self.line_just = line_just
    self.define ('JUCE_PLUGINHOST_VST', self.env.VST)

    # VST3 hosting
    self.env.VST3 = not bool (self.options.no_vst3)
    self.define ('JUCE_PLUGINHOST_VST3', self.env.VST3)
    self.define ('JUCE_VST3HEADERS_INCLUDE_HEADERS_ONLY', False)
    if len (self.options.vst3sdk) > 0:
        self.env.VST3SDK_PATH = self.options.vst3sdk
    else:
        self.env.VST3SDK_PATH = JUCE_VST3SDK_PATH

    if self.env.VST3:
        line_just = self.line_just
        self.check(header_name='pluginterfaces/vst2.x/vstfxstore.h', uselib_store='VSTFXSTORE_H', mandatory=False)
        self.define ('JUCE_VST3_CAN_REPLACE_VST2', bool (self.env.HAVE_VSTFXSTORE_H))
        self.env.INCLUDES_VST3 = [ self.env.VST3SDK_PATH ]
        self.line_just = line_just

    # LV2 Support
    self.env.LV2 = not bool(self.options.no_lv2)
    if self.env.LV2:
        self.check_cfg(package='lv2',    uselib_store="LV2", args='--cflags', mandatory=False)
        self.check_cfg(package='lilv-0', uselib_store="LILV", args='--cflags --libs', mandatory=False)
        self.check_cfg(package='suil-0', uselib_store="SUIL", args='--cflags --libs', mandatory=False)
        if bool(self.env.HAVE_SUIL):
            self.check_cxx (
                msg = "Checking for suil_init(...)",
                fragment = '''
                    #include <suil/suil.h>
                    int main(int, char**) {
                        suil_init (nullptr, nullptr, SUIL_ARG_NONE);
                        return 0;
                    }
                ''',
                execute = False,
                use = [ 'SUIL' ],
                uselib_store = 'SUIL_INIT',
                define_name = 'HAVE_SUIL_INIT',
                mandatory = False
            )
            self.define('JLV2_SUIL_INIT', bool(self.env.HAVE_SUIL_INIT))
        self.env.LV2 = bool(self.env.HAVE_LILV) and bool(self.env.HAVE_SUIL)
    self.define('JLV2_PLUGINHOST_LV2', self.env.LV2)

    # ALSA
    self.env.ALSA = False

    # LADSPA
    self.env.LADSPA = False

@conf
def check_asio (self):
    asio_includes = []
    if len(self.options.asiosdk) > 0:
        asio_includes.append (os.path.join (self.options.asiosdk, 'common'))

    return self.check_cxx (
        msg = "Checking for ASIO SDK",
        fragment = '''
            #include <windows.h>
            #include <iasiodrv.h>
            int main() { return 0; }
        ''',
        execute         = False,
        uselib_store    = 'ASIO',
        define_name     = 'HAVE_ASIO',
        mandatory       = False,
        includes        = asio_includes
    )

@conf
def check_mingw (self):
    for l in mingw_libs.split():
        self.env['LIB_%s' % l.upper()] = l
    self.check_ladspa()
    self.check_asio()
    self.define ('JUCE_ASIO', bool (self.env.HAVE_ASIO))
    
    # VST3 sdk is broken with mingw32 g++
    # self.env.VST3 = False
    # self.define ('JUCE_PLUGINHOST_VST3', self.env.VST3)
    self.define ('JUCE_PLUGINHOST_AU', False)
    self.define ('JUCE_USE_WINDOWS_MEDIA_FORMAT', False)
    for flag in '-Wno-multichar -Wno-deprecated-declarations'.split():
        self.env.append_unique ('CFLAGS', [flag])
        self.env.append_unique ('CXXFLAGS', [flag])

@conf
def check_mac (self):
    self.check_cxx(lib='readline', uselib_store='READLINE', mandatory=True)

    if self.env.JACK:
        # C++17 and JackOSX do not get along. This avoids compile
        # errors. TODO: base this on jack version instead of OS
        self.env.append_unique ('CXXFLAGS', ['-Wno-register'])

@conf
def check_linux (self):
    self.check(lib='pthread', mandatory=True)
    self.check(lib='dl', uselib_store='DL', mandatory=True)
    
    self.check_cxx(lib='readline', uselib_store='READLINE', mandatory=False)
    self.define ('LUA_USE_READLINE',  bool(self.env.LIB_READLINE))
    
    self.check_ladspa()

    self.env.ALSA = not bool (self.options.no_alsa)
    if self.env.ALSA:
        self.check_cfg(package='alsa', uselib_store='ALSA', args='--cflags --libs', mandatory=False)
        self.env.ALSA = bool(self.env.HAVE_ALSA)
    self.define ('JUCE_ALSA', self.env.ALSA)

    # Lua setup
    if self.env.LUA:
        self.define ('LUA_USE_LINUX', True)

    self.check_cfg (package='freetype2', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='x11', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='xext', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='xrandr', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='xcomposite', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='xinerama', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='xcursor', args='--cflags --libs', mandatory=True)
    self.check_cfg (package='gtk+-3.0', uselib_store='GTK',
        args='--cflags --libs', mandatory=False)
    
    self.define ('JUCE_USE_XRANDR', True)
    self.define ('JUCE_USE_XINERAMA', True)
    self.define ('JUCE_USE_XSHM', True)
    self.define ('JUCE_USE_XRENDER', True)
    self.define ('JUCE_USE_XCURSOR', True)

    self.define ('JLV2_GTKUI', False)

def get_mingw_libs():
    return [ l.upper() for l in mingw_libs.split() ]

def get_juce_library_code (prefix, ext=''):
    extension = ext
    if len(ext) <= 0:
        if juce.is_mac():
            extension = '.mm'
        else:
            extension = '.cpp'

    cpp_only = [ 'juce_analytics', 'juce_osc', 'jlv2_host' ]
    code = []
    for f in juce_modules.split():
        e = '.cpp' if f in cpp_only else extension
        code.append (prefix + '/include_' + f + e)
    return code

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, os, platform, re, sys, unicodedata
from xml.etree import ElementTree as ET

from waflib import Utils, Logs, Errors
from waflib.Configure import conf

def convert_camel (words, upper=False):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', words)
    if upper: return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).upper()
    else: return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def display_header (title):
    Logs.pprint ('BOLD', title)

def display_msg (conf, msg, status = None, color = None):
    color = 'CYAN'
    if type(status) == bool and status or status == "True":
        color = 'GREEN'
    elif type(status) == bool and not status or status == "False":
        color = 'YELLOW'
    Logs.pprint('BOLD', " *", sep='')
    Logs.pprint('NORMAL', "%s" % msg.ljust(conf.line_just - 3), sep='')
    Logs.pprint('BOLD', ":", sep='')
    Logs.pprint(color, status)

@conf
def prefer_clang(self):
    '''Use clang by default on non-windows'''
    if is_windows(): return
    if not 'CC' in os.environ or not 'CXX' in os.environ:
        if None != self.find_program ('clang', mandatory=False):
            self.env.CC  = 'clang'
            self.env.CXX = 'clang++'

@conf 
def check_ccache(self):
    return self.find_program ('ccache', mandatory=False)

@conf
def check_juce (self):
    '''this just checks that a version of juce exists'''

    display_msg (self, "Checking for JUCE")
    mpath = self.env.JUCE_MODULES_PATH = self.options.juce_modules
    if os.path.exists (mpath + '/juce_core/juce_core.h'):
        self.end_msg ('yes')
    else:
        self.end_msg ('no')

@conf
def check_rez (self):
    self.find_program("Rez")

@conf
def check_cxx_version (self, required=False):
    line_just = self.line_just

    if is_mac() and len(self.options.cross) <= 0:
        self.check_cxx (linkflags = [ "-stdlib=libc++", "-lc++" ],
                        cxxflags = [ "-stdlib=libc++", "-std=c++17" ],
                        mandatory = required)
        self.env.append_unique ("CXXFLAGS", ["-stdlib=libc++", "-std=c++17"])
        self.env.append_unique ("LINKFLAGS", ["-stdlib=libc++", "-lc++"])
    elif is_linux() or len(self.options.cross) > 0:
        self.check_cxx (cxxflags = [ "-std=c++17" ], mandatory = required)
        self.env.append_unique ("CXXFLAGS", [ "-std=c++17" ])
    else:
        print("setup cxx version for " + platform.system())
        exit(1)

    self.line_just = line_just

def is_mac():
    return 'Darwin' in platform.system()

def is_linux():
    return 'Linux' in platform.system()

def is_win32():
    return 'Windows' in platform.system()

def is_windows():
    return 'Windows' in platform.system()

def get_module_info (ctx, mod):
    nodes = find (ctx, os.path.join (mod, 'juce_module_info'))
    infofile = "%s" % nodes[0].relpath()
    return ModuleInfo (infofile)

def plugin_pattern (bld):
    ''' this is only valid after 'juce.py' has been loading during configure'''
    return bld.env['plugin_PATTERN']

def plugin_extension (bld):
    ''' this is only valid after 'juce.py' has been loading during configure'''
    return bld.env['plugin_EXT']

def options (opt):
    opt.add_option ('--debug', default=False, action="store_true", dest="debug", \
        help="Compile debuggable binaries [ Default: False ]")
    opt.add_option ('--juce-path', default='', action="store", dest="juce_modules", \
        help="Path to JUCE sources [ Default: $HOME/SDKs/JUCE ]")

def configure (conf):
    # debugging option
    if conf.options.debug:
        conf.define ("DEBUG", 1)
        conf.define ("_DEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-g', '-ggdb', '-O0'])
        conf.env.append_unique ('CFLAGS', ['-g', '-ggdb', '-O0'])
    else:
        conf.define ("NDEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-Os'])
        conf.env.append_unique ('CFLAGS', ['-Os'])

    # output dir (build dir)
    outdir = conf.options.out
    if len (outdir) == 0:
        outdir = "build"

    # module path
    if not conf.env.JUCE_MODULE_PATH:
        conf.env.JUCE_MODULE_PATH = os.path.join (os.path.expanduser("~"), 'juce/modules')

    # define a library pattern suitable for plugins/modules
    # (e.g. remove the 'lib' from libplugin.so)
    pat = conf.env['cshlib_PATTERN']
    if not pat:
        pat = conf.env['cxxshlib_PATTERN']
    if pat.startswith('lib'):
        pat = pat[3:]
    conf.env['plugin_PATTERN'] = pat
    conf.env['plugin_EXT'] = pat[pat.rfind('.'):]

    # do platform stuff
    if is_linux() and not 'mingw' in conf.env.CXX[0]:
        conf.define ('LINUX', 1)
    elif is_mac():
        conf.env.FRAMEWORK_ACCELERATE     = 'Accelerate'
        conf.env.FRAMEWORK_AUDIO_TOOLBOX  = 'AudioToolbox'
        conf.env.FRAMEWORK_AUDIO_UNIT     = 'AudioUnit'
        conf.env.FRAMEWORK_CORE_AUDIO     = 'CoreAudio'
        conf.env.FRAMEWORK_CORE_AUDIO_KIT = 'CoreAudioKit'
        conf.env.FRAMEWORK_CORE_MIDI      = 'CoreMIDI'
        conf.env.FRAMEWORK_COCOA          = 'Cocoa'
        conf.env.FRAMEWORK_CARBON         = 'Carbon'
        conf.env.FRAMEWORK_DISC_RECORDING = 'DiscRecording'
        conf.env.FRAMEWORK_IO_KIT         = 'IOKit'
        conf.env.FRAMEWORK_OPEN_GL        = 'OpenGL'
        conf.env.FRAMEWORK_QT_KIT         = 'QTKit'
        conf.env.FRAMEWORK_QuickTime      = 'QuickTime'
        conf.env.FRAMEWORK_QUARTZ_CORE    = 'QuartzCore'
        conf.env.FRAMEWORK_WEB_KIT        = 'WebKit'
        conf.env.FRAMEWORK_PYTHON         = 'Python'
    elif is_win32(): pass

def extension():
    if platform.system() != "Darwin":
        return ".cpp"
    else:
        return ".mm"

def find (ctx, pattern):
    '''find resources in the juce module path'''

    if len(pattern) <= 0:
        return None

    pattern = '%s/**/%s' % (ctx.env.JUCE_MODULE_PATH, pattern)
    return ctx.path.ant_glob (pattern)

def build_modular_libs (bld, mods, vnum='4.0.2', postfix=''):
    '''compile the passed modules into individual targets. returns
       a list of waf bld objects in case further setup is required'''
    libs = []
    mext = extension()

    for mod in mods:
        info = get_module_info (bld, mod)
        src  = find (bld, mod + mext)
        slug = mod.replace('_', '-')
        use  = info.requiredPackages()
        major_version = vnum[:1]
        if is_linux() and mod == 'juce_graphics':
            use += ['FREETYPE2']

        obj = bld (
            features  = "cxx cxxshlib",
            source    = list (set (src)),
            name      = '%s-%s' % (slug + postfix.replace('_', '-'), major_version),
            target    = '%s-%s' % (mod + postfix, major_version),
            use       = list (set (use)),
            includes  = [],
            linkflags = info.linkFlags()
        )

        if len(vnum) > 0:
            obj.vnum = vnum
        obj.module_info = info
        libs += [obj]

    return libs

def create_unified_lib (bld, tgt, mods, feats="cxx cxxshlib"):

    mext = extension()

    mod_path = bld.env.JUCE_MODULE_PATH
    src = []
    ug  = []

    for mod in mods:
        src += [mod_path + "/" + mod + "/" + mod + mext]
        ug += get_use_libs (mod)

    # strip out duplicate use libs
    us = list (set (us))

    obj = bld (
        features    = feats,
        source      = src,
        includes    = [],
        name        = tgt,
        target      = tgt,
        use         = us
    )

    return obj

def module_path (ctx):
    return ctx.env.JUCE_MODULE_PATH

def available_modules (ctx):
    return os.listdir (module_path (ctx))

from waflib import TaskGen
@TaskGen.extension ('.mm')
def juce_mm_hook (self, node):
    return self.create_compiled_task ('cxx', node)

from waflib import TaskGen
@TaskGen.extension ('.m')
def juce_m_hook (self, node):
    return self.create_compiled_task ('c', node)

#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, element, juce

VERSION='0.39.0'

VST3_PATH='libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")
    opt.add_option ('--enable-docking', default=False, action='store_true', dest='enable_docking', \
        help="Build with docking window support")

def silence_warnings (conf):
    '''TODO: resolve these'''
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])
    conf.env.append_unique ('CFLAGS', ['-Wno-dynamic-class-memaccess'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-dynamic-class-memaccess'])
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-declarations'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-declarations'])

def configure_product (conf):
    conf.define ('EL_PRO', 1)
    conf.env.EL_SOLO = False
    conf.env.EL_FREE = False
    conf.env.EL_PRO  = True

def configure (conf):
    conf.env.DATADIR = os.path.join (conf.env.PREFIX, 'share/element')
    
    conf.check_ccache()
    cross.setup_compiler (conf)
    if len(conf.options.cross) <= 0:
        conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx ar cross juce")
    conf.check_cxx_version()

    silence_warnings (conf)

    conf.check_common()
    if cross.is_mingw(conf): conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.env.DEBUG = conf.options.debug
    conf.env.EL_VERSION_STRING = VERSION
    
    configure_product (conf)

    conf.define ('EL_USE_JACK', 0)
    conf.define ('EL_VERSION_STRING', conf.env.EL_VERSION_STRING)
    conf.define ('EL_DOCKING', 1 if conf.options.enable_docking else 0)
    conf.define ('KV_DOCKING_WINDOWS', 1)
    
    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])

    print
    juce.display_header ("Element Configuration")
    juce.display_msg (conf, "Workspaces", conf.options.enable_docking)
    juce.display_msg (conf, "Debug", conf.options.debug)
    juce.display_msg (conf, "VST2", bool(conf.env.HAVE_VST))
    juce.display_msg (conf, "VST3", True)
    juce.display_msg (conf, "LADSPA", bool(conf.env.HAVE_LADSPA))
    juce.display_msg (conf, "LV2", bool(conf.env.LV2))
    print
    juce.display_msg (conf, "PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "DATADIR", conf.env.DATADIR)
    print
    juce.display_msg (conf, "CFLAGS", conf.env.CFLAGS)
    juce.display_msg (conf, "CXXFLAGS", conf.env.CXXFLAGS)
    juce.display_msg (conf, "LINKFLAGS", conf.env.LINKFLAGS)

def build_desktop (bld, slug='element'):
    if not juce.is_linux():
        return

    src = "data/%s.desktop.in" % (slug)
    tgt = "%s.desktop" % (slug)

    element_data = '%s/element' % (bld.env.DATADIR)
    element_bin  = '%s/bin' % (bld.env.PREFIX)

    if os.path.exists (src):
        bld (features = "subst",
             source    = src,
             target    = tgt,
             name      = tgt,
             ELEMENT_BINDIR = element_bin,
             ELEMENT_DATA = element_data,
             install_path = bld.env.DATADIR + "/applications"
        )

        bld.install_files (element_data, 'data/element_icon.xpm')

def copy_mingw_libs (bld):
    return

def build_mingw (bld):
    mingwEnv = bld.env.derive()
    mingwSrc = element.get_juce_library_code ("project/JuceLibraryCode", ".cpp")
    mingwSrc += bld.path.ant_glob('src/**/*.cpp')
    mingwSrc += bld.path.ant_glob('project/JuceLibraryCode/BinaryData*.cpp')
    mingwSrc.sort()
    
    bld.program (
        source      = mingwSrc,
        includes    =  [ '/opt/kushview/include', 'libs/JUCE/modules', \
                         'libs/kv/modules', 'project/JuceLibraryCode', \
                         'src', 'libs/vst3sdk', \
                         os.path.expanduser('~') + '/SDKs/ASIOSDK/common' ],
        cxxflags    = '',
        target      = '%s/Element' % (mingwEnv.CROSS),
        name        = 'Element',
        linkflags   = [ '-mwindows', '-static-libgcc', '-static-libstdc++' ],
        use         = element.get_mingw_libs(),
        env         = mingwEnv
    )

    bld.add_post_fun (copy_mingw_libs)

def build_linux (bld):
    libEnv = bld.env.derive()
    bld.shlib (
        source      = element.get_juce_library_code ("project/JuceLibraryCode", ".cpp"),
        includes    = [ 'libs/JUCE/modules', \
                        'libs/kv/modules', \
                        'libs/jlv2/modules', \
                        'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'lib/kv',
        name        = 'KV',
        env         = libEnv,
        use         = [ 'FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL', 'LILV', 'SUIL' ]
    )

    appEnv = bld.env.derive()

    bld.stlib (
        source      = bld.path.ant_glob ('src/**/*.cpp') + \
                      bld.path.ant_glob ('project/JuceLibraryCode/BinaryData*.cpp'),
        includes    = [ 'libs/JUCE/modules', \
                        'libs/kv/modules', \
                        'libs/jlv2/modules', \
                        'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'lib/element',
        name        = 'EL',
        env         = appEnv,
        use         = [ 'KV' ]
    )

    bld.add_group()

    bld.program (
        source      = [ 'project/Source/Main.cpp' ],
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'libs/jlv2/modules', 'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'bin/element',
        name        = 'Element',
        env         = appEnv,
        use         = [ 'KV', 'EL' ],
    )

def build_mac (bld):
    libEnv = bld.env.derive()
    bld (
        features    = 'cxx cxxshlib',
        source      = element.get_juce_library_code ("project/JuceLibraryCode", ".mm"),
        includes    = [ 'libs/JUCE/modules', \
                        'libs/kv/modules', \
                        'libs/jlv2/modules', \
                        'project/JuceLibraryCode', \
                        VST3_PATH, \
                        'src' ],
        target      = 'lib/kv',
        name        = 'KV',
        env         = libEnv,
        use         = 'ACCELERATE AUDIO_TOOLBOX AUDIO_UNIT CORE_AUDIO CORE_AUDIO_KIT \
                       COCOA CORE_MIDI IO_KIT QUARTZ_CORE LILV SUIL'
    )

    appEnv = bld.env.derive()

    bld.stlib (
        source      = bld.path.ant_glob ('src/**/*.cpp') + \
                      bld.path.ant_glob ('project/JuceLibraryCode/BinaryData*.cpp'),
        includes    = [ '/opt/kushview/include', \
                        'libs/JUCE/modules', \
                        'libs/kv/modules', \
                        'libs/jlv2/modules', \
                        'project/JuceLibraryCode', \
                        'src', VST3_PATH ],
        target      = 'lib/element',
        name        = 'EL',
        env         = appEnv,
        use         = [ 'KV', 'BOOST_SIGNALS' ]
    )

    bld.add_group()
    
    app = bld.program (
        source      = [ 'project/Source/Main.cpp' ],
        includes    = [ 'src', VST3_PATH ],
        target      = 'Applications/Element',
        name        = 'Element',
        env         = appEnv,
        use         = [ 'KV', 'EL' ],
        linkflags   = [ ],
        mac_app     = True,
        mac_plist   = 'data/InfoPro.plist',
        mac_files   = [ 'project/Builds/MacOSX/Icon.icns' ]
    )
    
def build (bld):
    if cross.is_windows (bld):
        build_mingw (bld)
    elif juce.is_mac():
        build_mac (bld)
    else:
        build_linux (bld)
        build_desktop (bld)

    bld.recurse ('tests')

def check (ctx):
    call (["build/bin/test-element"])

#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, element, juce

VERSION='0.22.3'

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")
    opt.add_option ('--disable-unlocking', default=False, action="store_true", dest="disable_unlocking", \
        help="Build without license protection [ Default: False ]")
    opt.add_option ('--disable-unlocking', default=False, action="store_true", dest="disable_unlocking", \
        help="Build without license protection [ Default: False ]")
    opt.add_option ('--enable-free', default=False, action='store_true', dest='enable_free', \
        help="Build the free version")
    opt.add_option ('--enable-docking', default=False, action='store_true', dest='enable_docking', \
        help="Build with docking window support")
    opt.add_option ('--enable-local-auth', default=False, action='store_true', dest='enable_local_auth', \
        help="Authenticate locally")

def silence_warnings(conf):
    '''TODO: resolve these'''
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])
    # conf.env.append_unique ('CFLAGS', ['-Wno-dynamic-class-memaccess'])
    # conf.env.append_unique ('CXXFLAGS', ['-Wno-dynamic-class-memaccess'])
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-declarations'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-declarations'])

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
    
    conf.define ('EL_DISABLE_UNLOCKING', 1 if conf.options.disable_unlocking else 0)
    conf.define ('EL_USE_LOCAL_AUTH', 1 if conf.options.enable_local_auth else 0)

    if conf.options.enable_free: conf.define ('EL_FREE', 1)
    conf.define ('EL_VERSION_STRING', conf.env.EL_VERSION_STRING)
    conf.define ('EL_DOCKING', 1 if conf.options.enable_docking else 0)
    conf.define ('KV_DOCKING_WINDOWS', 1 if conf.options.enable_docking else 0)
    conf.define ('EL_USE_JACK', 0)

    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])

    conf.check(lib='curl', mandatory=False)
    if juce.is_linux():
        conf.check(lib='pthread', mandatory=True)
        conf.check(lib='dl', mandatory=True)
        conf.check_cfg(package='freetype2', args='--cflags --libs', \
            mandatory=True)
        conf.check_cfg(package='x11', args='--cflags --libs', \
            mandatory=True)
        conf.check_cfg(package='xext', args='--cflags --libs', \
            mandatory=True)
        conf.check_cfg(package='alsa', args='--cflags --libs', \
            mandatory=True)
    if cross.is_windows (conf):
        conf.check(lib='ws2_32', mandatory=True)
        conf.check(lib='pthread', mandatory=True)

    print
    juce.display_header ("Element Configuration")
    juce.display_msg (conf, "Installation PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "Installation DATADIR", conf.env.DATADIR)
    juce.display_msg (conf, "Debugging Symbols", conf.options.debug)

    juce.display_msg (conf, "Element Free", conf.options.enable_free)
    juce.display_msg (conf, "Docking Windows", conf.options.enable_docking)
    juce.display_msg (conf, "Copy Protection", not conf.options.disable_unlocking)
    juce.display_msg (conf, "Local authentication", conf.options.enable_local_auth)

    print
    juce.display_header ("Compiler")
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

def common_use_flags():
    return [ 'JUCE_AUDIO_UTILS', 'JUCE_GUI_EXTRA', 'JUCE_OPENGL', 'KV_GUI', \
             'KV_ENGINE', 'KV_MODELS', 'KV_LV2', 'LILV', 'SUIL' ]

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
                         'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK', \
                         os.path.expanduser('~') + '/SDKs/ASIOSDK/common' ],
        cxxflags    = '',
        target      = '%s/Element' % (mingwEnv.CROSS),
        name        = 'Element',
        linkflags   = [ '-mwindows', '-static-libgcc', '-static-libstdc++' ],
        use         = element.get_mingw_libs(),
        env         = mingwEnv
    )

    bld.add_post_fun (copy_mingw_libs)

def build_plugin (bld, name):
    bundle = '%s.element' % name
    plugin_dir = 'plugins/%s' % bundle
    env = bld.env.derive()
    env.cxxshlib_PATTERN = env.cshlib_PATTERN = juce.plugin_pattern(bld)
    lib = bld.shlib (
        source = bld.path.ant_glob('%s/**/*.cpp' % plugin_dir),
        includes = ['plugins', plugin_dir, 'src'],
        name = name,
        target = 'modules/%s/%s' % (bundle, name),
        use = common_use_flags(),
        env = env,
        install_path = bld.env.LIBDIR + '/element/%s' % bundle
    )

    if bld.env.INTERNAL_MODULES:
        lib.includes += ['libs/element', 'libs/element/element']

    bld (
        features     = 'subst',
        source       = '%s/manifest.json' % plugin_dir,
        target       = '%s/manifest.json' % plugin_dir,
        install_path = bld.env.LIBDIR + '/element/%s' % bundle,
    )

def build_plugins (bld):
    for name in 'test'.split():
        build_plugin (bld, name)
    bld.add_group()

def build_linux (bld):
    libEnv = bld.env.derive()
    bld.shlib (
        source      = element.get_juce_library_code ("project/JuceLibraryCode", ".cpp"),
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'lib/kv',
        name        = 'KV',
        env         = libEnv,
        use         = [ 'FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT' ]
    )

    appEnv = bld.env.derive()

    bld.stlib (
        source      = bld.path.ant_glob ('src/**/*.cpp') + \
                      bld.path.ant_glob ('project/JuceLibraryCode/BinaryData*.cpp'),
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'project/JuceLibraryCode', \
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
                        'libs/kv/modules', 'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'bin/element',
        name        = 'Element',
        env         = appEnv,
        use         = [ 'KV', 'EL' ],
    )

def build_mac (bld):
    libEnv = bld.env.derive()
    bld.shlib (
        source      = element.get_juce_library_code ("project/JuceLibraryCode", ".mm"),
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'lib/kv',
        name        = 'KV',
        env         = libEnv,
        use         = 'ACCELERATE AUDIO_TOOLBOX AUDIO_UNIT CORE_AUDIO CORE_AUDIO_KIT \
                       COCOA CORE_MIDI IO_KIT QUARTZ_CORE'
    )

    appEnv = bld.env.derive()

    bld.stlib (
        source      = bld.path.ant_glob ('src/**/*.cpp') + \
                      bld.path.ant_glob ('project/JuceLibraryCode/BinaryData*.cpp'),
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'project/JuceLibraryCode', \
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
                        'libs/kv/modules', 'project/JuceLibraryCode', \
                        'src', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'Applications/Element',
        name        = 'Element',
        env         = appEnv,
        use         = [ 'KV', 'EL' ],
        mac_app     = True,
        mac_plist   = 'data/MacDeploy/Info-Standalone.plist',
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
    call (["build/tests/tests"])

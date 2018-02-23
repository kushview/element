#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, element, juce

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")

def configure (conf):
    cross.setup_compiler (conf)
    if len(conf.options.cross) <= 0:
        conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx ar cross juce")

    conf.env.DATADIR = os.path.join (conf.env.PREFIX, 'share/element')
    conf.check_cxx11()
    
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])

    if cross.is_mingw(conf): conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.env.DEBUG = conf.options.debug
    conf.env.ELEMENT_VERSION_STRING = '0.15.7'
    conf.define ('ELEMENT_VERSION_STRING', conf.env.ELEMENT_VERSION_STRING)
    conf.define ('ELEMENT_USE_JACK', len(conf.env.LIB_JACK) > 0)
    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])

    print
    juce.display_header ("Element Build Summary")
    juce.display_msg (conf, "Installation PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "Installation DATADIR", conf.env.DATADIR)

    print
    juce.display_header ("Compiler")
    juce.display_msg (conf, "CPPFLAGS", conf.env.CPPFLAGS)
    juce.display_msg (conf, "CFLAGS", conf.env.CFLAGS)
    juce.display_msg (conf, "CXXFLAGS", conf.env.CXXFLAGS)
    juce.display_msg (conf, "LINKFLAGS", conf.env.LINKFLAGS)

def build_desktop (bld, slug):
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
    return ['JUCE_AUDIO_UTILS', 'JUCE_GUI_EXTRA', 'JUCE_OPENGL', 'ELEMENT_GUI', \
            'ELEMENT_ENGINES', 'ELEMENT_MODELS', 'ELEMENT_LV2', 'LILV', 'SUIL']

def copy_mingw_libs (bld):
    return

def build_mingw (bld):
    mingwEnv = bld.env.derive()
    mingwSrc = element.get_juce_library_code ("project/JuceLibraryCode", ".cpp")
    mingwSrc += bld.path.ant_glob('project/Source/**/*.cpp')
    mingwSrc += bld.path.ant_glob('project/JuceLibraryCode/BinaryData*.cpp')
    mingwSrc.sort()
    
    bld.program (
        source      = mingwSrc,
        includes    =  [ '/opt/kushview/include', 'libs/JUCE/modules', \
                         'libs/kv/modules', 'project/JuceLibraryCode', \
                         'project/Source', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK', \
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
        includes = ['plugins', plugin_dir, 'project/Source'],
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
    return

def build_mac (bld):
    appEnv = bld.env.derive()
    bld.program (
        source      = bld.path.ant_glob ('project/Source/**/*.cpp') + \
                      bld.path.ant_glob ('project/JuceLibraryCode/BinaryData*.cpp') + \
                      element.get_juce_library_code ("project/JuceLibraryCode", ".mm"),
        includes    = [ '/opt/kushview/include', 'libs/JUCE/modules', \
                        'libs/kv/modules', 'project/JuceLibraryCode', \
                        'project/Source', os.path.expanduser('~') + '/SDKs/VST_SDK/VST3_SDK' ],
        target      = 'Applications/Element',
        name        = 'Element',
        env         = appEnv,
        use         = 'ACCELERATE AUDIO_TOOLBOX AUDIO_UNIT CORE_AUDIO CORE_AUDIO_KIT \
                       COCOA CORE_MIDI IO_KIT QUARTZ_CORE',
        mac_app     = True,
        mac_plist   = 'data/MacDeploy/Info-Standalone.plist',
        mac_files   = [ 'project/Builds/MacOSX/Icon.icns' ]
    )

def build (bld):
    if cross.is_windows (bld):
        return build_mingw (bld)
    elif juce.is_mac():
        return build_mac (bld)
    else:
        build_linux (bld)

    for subdir in 'tests'.split():
        bld.recurse (subdir)

def check (ctx):
    call (["build/tests/tests"])

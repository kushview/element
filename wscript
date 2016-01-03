#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, juce

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")
    opt.add_option ('--internal-modules', default=False, action='store_true', \
        dest='internal_modules', help='Compile Intrenal Element Modules [ Default: False ]')
    opt.recurse('libs/element')

def check_modules (conf):
    if juce.is_linux() and not cross.is_windows(conf) and not conf.options.internal_modules:
        conf.check_juce_cfg(['audio-processors', 'audio-devices', 'core', 'cryptography', \
                             'audio-utils', 'gui-basics', 'gui-extra', 'graphics', 'opengl'])
        conf.check_juce_cfg(['base', 'engines', 'lv2', 'models', 'gui'], 0, 'element_', True)
    elif juce.is_linux() and not cross.is_windows(conf):
        conf.recurse('libs/element')
        conf.define('HAVE_JUCE_CORE', len(conf.env.LIB_JUCE_CORE) > 0)
        if not conf.is_defined('ELEMENT_USE_LIBJUCE'):
            conf.define('ELEMENT_USE_LIBJUCE', 1)
    elif juce.is_linux() and cross.is_windows(conf):
        conf.check_cfg(package='juce', uselib_store='JUCE', args='--cflags --libs', mandatory=True)
        conf.recurse('libs/element')
        conf.define('HAVE_JUCE_CORE', True)
        if not conf.is_defined('ELEMENT_USE_LIBJUCE'):
            conf.define('ELEMENT_USE_LIBJUCE', 1)

def configure (conf):
    cross.setup_compiler (conf)
    if not conf.options.cross:
        conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx cross juce")
    conf.env.DATADIR = os.path.join (conf.env.PREFIX, 'share')

    print
    juce.display_header ("Element Configuration")
    conf.check_cxx11()

    # Do pkg-config stuff
    check_modules (conf)

    conf.check_cfg (package="lv2", uselib_store="LV2", args='--cflags --libs', mandatory=True)
    conf.check_cfg (package="lilv-0", uselib_store="LILV", args='--cflags --libs', mandatory=True)
    conf.check_cfg (package="suil-0", uselib_store="SUIL", args='--cflags --libs', mandatory=True)
    conf.check_cfg (package="jack", uselib_store="JACK", args='--cflags --libs', mandatory=False)
    pkg_defs = ['HAVE_LILV', 'HAVE_JACK', 'HAVE_SUIL', 'HAVE_LV2']

    if juce.is_linux() and not cross.is_windows (conf):
        conf.check_cfg (package="alsa", uselib_store="ALSA", args='--cflags --libs', mandatory=True)
        conf.check_cfg (package="x11", uselib_store="X11", args='--cflags --libs', mandatory=True)
        conf.check_cfg (package="xext", uselib_store="XEXT", args='--cflags --libs', mandatory=True)
        conf.check_cfg (package="freetype2", uselib_store="FREETYPE2", args='--cflags --libs', mandatory=True)
        conf.check_cfg (package="gl", uselib_store="GL", args='--cflags --libs', mandatory=True)
        conf.check_cfg (package="glesv2", uselib_store="GLESV2", args='--cflags --libs', mandatory=False)
        conf.check_cfg (package="egl", uselib_store="EGL", args='--cflags --libs', mandatory=False)
        pkg_defs += ['HAVE_ALSA', 'HAVE_X11', 'HAVE_XEXT', 'HAVE_FREETYPE2', 'HAVE_GL']

    for d in pkg_defs: conf.env[d] = conf.is_defined (d)

    if 'clang' in conf.env.CXX[0]:
        conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])

    conf.env.DEBUG = conf.options.debug
    conf.env.INTERNAL_MODULES = conf.options.internal_modules
    conf.env.ELEMENT_VERSION_STRING = '0.0.1'
    conf.define ('ELEMENT_VERSION_STRING', conf.env.ELEMENT_VERSION_STRING)
    conf.define ('ELEMENT_USE_JACK', len(conf.env.LIB_JACK) > 0)

    if juce.is_mac():
        conf.env.MODULEDIR = "/Library/Application Support/Element/Plug-Ins"
    else:
        conf.env.MODULEDIR = conf.env.LIBDIR + "/element/modules"

    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])
    conf.define ("ELEMENT_DEFAULT_MODULE_PATH", ":".join(conf.env.MODULE_PATH))

    print
    juce.display_header ("Element Build Summary")
    juce.display_msg (conf, "Installation Prefix", conf.env.PREFIX)
    juce.display_msg (conf, "Installed DATADIR", conf.env.DATADIR)
    juce.display_msg (conf, "Jack Audio Support", conf.env.HAVE_JACK)
    juce.display_msg (conf, "LV2 Plugin Support", conf.env.HAVE_LILV)
    juce.display_msg (conf, "LV2 Plugin UI Support", conf.env.HAVE_SUIL)
    juce.display_msg (conf, "Library Version", conf.env.ELEMENT_VERSION_STRING)
    juce.display_msg (conf, "Module Install Dir", conf.env.MODULEDIR)
    juce.display_msg (conf, "Module Search Path", conf.env.MODULE_PATH)

    if juce.is_mac():
        print
        juce.display_header ("Apple Configuration Summary")
        juce.display_msg (conf, "Apple Framework Dir", conf.env.FRAMEWORKDIR)
        juce.display_msg (conf, "Apple Deployment Target", conf.env.APPLE_VERSION_MIN)

    print
    juce.display_header ("Global Compiler Flags")
    juce.display_msg (conf, "Compile flags (c)", conf.env.CFLAGS)
    juce.display_msg (conf, "Compile flags (c++)", conf.env.CXXFLAGS)
    juce.display_msg (conf, "Linker flags", conf.env.LINKFLAGS)

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

def internal_library_use_flags(bld):
    debug = bld.env.DEBUG
    return ['element-base-0', 'element-gui-0', 'element-engines-0', 'element-lv2-0'] if not debug \
        else ['element-base-debug-0', 'element-gui-debug-0', 'element-engines-debug-0', 'element-lv2-debug-0']

def build_mingw (bld):
    bld.program(
        source = bld.path.ant_glob('project/Source/**/*.cpp') + \
                    ['libs/element/element/modules/element_base/element_base.cpp',
                     'libs/element/element/modules/element_engines/element_engines.cpp',
                     'libs/element/element/modules/element_lv2/element_lv2.cpp',
                     'libs/element/element/modules/element_gui/element_gui.cpp',
                     'libs/element/element/modules/element_models/element_models.cpp'],
        target = 'Element',
        name = 'Element',
        includes = ['libs/element', 'src', 'project/Source'],
        use = ['JUCE', 'LILV', 'SUIL']
    )

def build_internal_library (bld):
    libEnv = bld.env.derive()
    bld.recurse('libs/element')

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

def build_plugins(bld):
    for name in 'test'.split():
        build_plugin (bld, name)
    bld.add_group()

def build_linux (bld):
    node = bld.path.find_resource ('project/Element.jucer')
    proj = juce.IntrojucerProject (bld, node.relpath())

    build_desktop (bld, 'element')

    if bld.env.INTERNAL_MODULES:
        build_internal_library (bld)

    build_plugins(bld)

    appEnv = bld.env.derive()
    obj = bld.program (
        source = bld.path.ant_glob ("project/Source/**/*.cpp"),
        includes = ['project/Source'],
        use = common_use_flags(),
        target = 'bin/element',
        linkflags = '-Wl,-rpath,$ORIGIN'
    )

    if bld.env.INTERNAL_MODULES:
        obj.includes += ['libs/element', 'libs/element/element']
        obj.use += internal_library_use_flags (bld)
        obj.linkflags += ' -Wl,-rpath,$ORIGIN/../libs/element'

def build (bld):
    if cross.is_windows (bld):
        return build_mingw (bld)
    else:
        build_linux (bld)

    for subdir in 'tests'.split():
        bld.recurse (subdir)

def check (ctx):
    call (["build/tests/tests"])

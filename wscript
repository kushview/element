#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, element, juce

APPNAME='element'
VERSION='0.41.1'

VST3_PATH='libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")
    opt.add_option ('--enable-docking', default=False, action='store_true', dest='enable_docking', \
        help="Build with docking window support")
    opt.add_option ('--enable-lua', default=False, action='store_true', dest='lua', \
        help="Build with Lua scripting support")
    opt.add_option ('--without-jack', default=False, action='store_true', dest='no_jack', \
        help="Build without JACK support")
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite")
    opt.add_option ('--with-vst-sdk', default='', type='string', dest='vst_sdk', \
        help="Specify the VST2 SDK path")
    opt.add_option('--ziptype', default='gz', dest='ziptype', type='string', 
        help='Zip type for waf dist (gz/bz2/zip) [ Default: gz ]')

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

    conf.env.TEST = bool(conf.options.test)
    conf.env.DEBUG = conf.options.debug
    conf.env.EL_VERSION_STRING = VERSION
    
    configure_product (conf)

    conf.env.LUA = bool(conf.options.lua)
    conf.define ('EL_LUA', conf.env.LUA)

    conf.define ('EL_USE_JACK', 0)
    conf.define ('EL_VERSION_STRING', conf.env.EL_VERSION_STRING)
    conf.define ('EL_DOCKING', 1 if conf.options.enable_docking else 0)
    conf.define ('KV_DOCKING_WINDOWS', 1)
    
    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])

    print
    juce.display_header ("Element")
    juce.display_msg (conf, "JACK", conf.env.JACK)
    juce.display_msg (conf, "VST2", bool(conf.env.HAVE_VST))
    juce.display_msg (conf, "VST3", True)
    juce.display_msg (conf, "LADSPA", bool(conf.env.HAVE_LADSPA))
    juce.display_msg (conf, "LV2", bool(conf.env.LV2))
    juce.display_msg (conf, "Workspaces", conf.options.enable_docking)
    juce.display_msg (conf, "Lua", conf.env.LUA)
    juce.display_msg (conf, "Debug", conf.options.debug)

    print
    juce.display_msg (conf, "PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "DATADIR", conf.env.DATADIR)
    juce.display_msg (conf, "CFLAGS", conf.env.CFLAGS)
    juce.display_msg (conf, "CXXFLAGS", conf.env.CXXFLAGS)
    juce.display_msg (conf, "LINKFLAGS", conf.env.LINKFLAGS)

def common_includes():
    return [ 'libs/JUCE/modules', \
             'libs/kv/modules', \
             'libs/jlv2/modules', \
             'libs/compat', \
             'libs/lua/src', \
             VST3_PATH, \
             'src' ]

def build_desktop (bld, slug='element'):
    if not juce.is_linux():
        return

    src = "data/%s.desktop.in" % (slug)
    tgt = "%s.desktop" % (slug)

    element_data = '%s' % (bld.env.DATADIR)
    element_bin  = '%s/bin' % (bld.env.PREFIX)

    if os.path.exists (src):
        bld (features = "subst",
             source    = src,
             target    = tgt,
             name      = tgt,
             ELEMENT_BINDIR = element_bin,
             ELEMENT_DATA = element_data,
             install_path = bld.env.PREFIX + "/share/applications"
        )

        bld.install_files (element_data, 'data/ElementIcon.png')

def build_lua (bld):
    lua_src = '''
        libs/lua/src/lauxlib.c
        libs/lua/src/liolib.c
        libs/lua/src/lopcodes.c
        libs/lua/src/lstate.c
        libs/lua/src/lobject.c
        libs/lua/src/lmathlib.c
        libs/lua/src/loadlib.c
        libs/lua/src/lvm.c
        libs/lua/src/lfunc.c
        libs/lua/src/lstrlib.c
        libs/lua/src/lua.c
        libs/lua/src/linit.c
        libs/lua/src/lstring.c
        libs/lua/src/lundump.c
        libs/lua/src/lctype.c
        libs/lua/src/ltable.c
        libs/lua/src/ldump.c
        libs/lua/src/loslib.c
        libs/lua/src/lgc.c
        libs/lua/src/lzio.c
        libs/lua/src/ldblib.c
        libs/lua/src/lutf8lib.c
        libs/lua/src/lmem.c
        libs/lua/src/lcorolib.c
        libs/lua/src/lcode.c
        libs/lua/src/ltablib.c
        libs/lua/src/lbitlib.c
        libs/lua/src/lapi.c
        libs/lua/src/lbaselib.c
        libs/lua/src/ldebug.c
        libs/lua/src/lparser.c
        libs/lua/src/llex.c
        libs/lua/src/ltm.c
        libs/lua/src/ldo.c'''.split()

    lua = bld (
        features    = "c cstlib",
        source      = lua_src,
        includes    = [ 'libs/lua/src' ],
        target      = 'lib/lua',
        name        = 'LUA',
        use         = [],
        linkflags   = [],
        env         = bld.env.derive()
    )

    lua.env.CFLAGS = ['-std=c99', '-O2', '-Wall', '-Wextra', '-DLUA_COMPAT_5_2', '-fPIC']
    lua.env.CXXFLAGS = []

    if juce.is_mac():
        lua.env.append_unique('CFLAGS', [ '-DLUA_USE_MACOSX' ])
        lua.use.append('READLINE')
    elif juce.is_linux():
        lua.env.append_unique('CFLAGS', [ '-DLUA_USE_LINUX' ])
        lua.use.append('READLINE')
        lua.use.append('DL')
        lua.linkflags.append('-Wl,-E')

    bld.add_group()
    return lua

def compile (bld):
    libEnv = bld.env.derive()
    library = bld (
        features    = 'cxx cxxshlib',
        source      = element.get_juce_library_code ("libs/compat") +
                      bld.path.ant_glob ('libs/compat/BinaryData*.cpp') +
                      bld.path.ant_glob ('src/**/*.cpp'),
        includes    = common_includes(),
        target      = 'lib/element-0',
        name        = 'ELEMENT',
        env         = libEnv,
        use         = [ 'BOOST_SIGNALS', 'LUA' ]
    )

    bld.add_group()

    appEnv = bld.env.derive()
    app = bld.program (
        source      = [ 'src/Main.cc' ],
        includes    = common_includes(),
        target      = 'bin/element',
        name        = 'ElementApp',
        env         = appEnv,
        use         = [ 'ELEMENT' ],
        linkflags   = []
    )
    
    if bld.env.LV2:     library.use += [ 'SUIL', 'LILV', 'LV2' ]
    if bld.env.JACK:    library.use += [ 'JACK' ]

    if juce.is_linux():
        build_desktop(bld)
        library.use += [ 'FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL' ]

    elif juce.is_mac():
        library.use += [ 'ACCELERATE', 'AUDIO_TOOLBOX', 'AUDIO_UNIT', 'CORE_AUDIO', 
                         'CORE_AUDIO_KIT', 'COCOA', 'CORE_MIDI', 'IO_KIT', 'QUARTZ_CORE' ]
        app.target      = 'Applications/Element'
        app.mac_app     = True
        app.mac_plist   = 'data/Info.plist'
        app.mac_files   = [ 'data/Icon.icns' ]
    else:
        pass

def build (bld):
    if bld.env.LUA: build_lua(bld)
    compile(bld)
    if bld.env.TEST: bld.recurse ('tests')

def check (ctx):
    if not os.path.exists('build/bin/test-element'):
        ctx.fatal("Tests not compiled")
        return
    if 0 != call (["build/bin/test-element"]):
        ctx.fatal("Tests failed")

def dist(ctx):
    z = ctx.options.ziptype
    if 'zip' in z:
        ziptype = z
    else:
        ziptype = "tar." + z

    ctx.base_name = '%s-%s' % (APPNAME, VERSION)
    ctx.algo = ziptype
    ctx.excl = ' **/.waf-1* **/.waf-2* **/.waf3-* **/*~ **/*.pyc **/*.swp **/.lock-w*'
    ctx.excl += ' **/.gitignore **/.gitmodules **/.git dist deploy pro **/Builds **/build'
    ctx.excl += ' **/.DS_Store **/.vscode **/.travis.yml *.bz2 *.zip *.gz'
    ctx.excl += ' tools/jucer/**/JuceLibraryCode'

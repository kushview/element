#!/usr/bin/env python

from subprocess import call, Popen, PIPE
import os, sys
import string
sys.path.append (os.getcwd() + "/tools/waf")
import cross, element, juce

APPNAME='element'
VERSION='0.45.1'

VST3_PATH='libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")
    
    opt.add_option ('--disable-ladspa', default=False, action='store_true', dest='no_ladspa', \
        help="Disable LADSPA plugin hosting")
    opt.add_option ('--disable-lv2', default=False, action='store_true', dest='no_lv2', \
        help="Disable LV2 plugin hosting")
    opt.add_option ('--disable-gtkui', default=False, action='store_true', dest='no_gtkui', \
        help="Disable GtkUI plugin hosting")
    
    opt.add_option ('--enable-docking', default=False, action='store_true', dest='enable_docking', \
        help="Build with docking window support")
    
    opt.add_option ('--without-alsa', default=False, action='store_true', dest='no_alsa', \
        help="Build without ALSA support")
    opt.add_option ('--without-jack', default=False, action='store_true', dest='no_jack', \
        help="Build without JACK support")
    opt.add_option ('--without-lua', default=False, action='store_true', dest='no_lua', \
        help="Build without LUA scripting")
    
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite")
    opt.add_option ('--with-vst-sdk', default='', type='string', dest='vst_sdk', \
        help="Specify the VST2 SDK path")
    opt.add_option('--ziptype', default='gz', dest='ziptype', type='string', 
        help='Zip type for waf dist (gz/bz2/zip) [ Default: gz ]')

def silence_warnings (conf):
    '''TODO: resolve these'''
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-declarations'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-declarations'])
    
    if 'clang' in conf.env.CXX[0]:
        conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CFLAGS', ['-Wno-dynamic-class-memaccess'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-dynamic-class-memaccess'])

def configure_product (conf):
    conf.define ('EL_PRO', 1)
    conf.env.EL_SOLO = False
    conf.env.EL_FREE = False
    conf.env.EL_PRO  = True

def configure_git_version (ctx):
    if os.path.exists('.git'):
        call(["python", "tools/gitversion.py"], stdout=PIPE)

def configure (conf):
    configure_git_version (conf)
    conf.env.DATADIR = os.path.join (conf.env.PREFIX, 'share/element')
    
    conf.check_ccache()
    cross.setup_compiler (conf)
    if len(conf.options.cross) <= 0:
        conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx ar cross juce")
    conf.check_cxx_version()
    silence_warnings (conf)

    conf.find_program('convert', mandatory=False)
    conf.find_program('ldoc', mandatory=False)

    conf.check_common()
    if cross.is_mingw(conf): conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.env.TEST = bool(conf.options.test)
    conf.env.DEBUG = conf.options.debug
    conf.env.EL_VERSION_STRING = VERSION
    
    conf.define ('EL_USE_JACK', 0)
    conf.define ('EL_VERSION_STRING', conf.env.EL_VERSION_STRING)
    conf.define ('EL_DOCKING', 1 if conf.options.enable_docking else 0)
    conf.define ('KV_DOCKING_WINDOWS', 1)
    
    conf.env.append_unique ("MODULE_PATH", [conf.env.MODULEDIR])

    print
    juce.display_header ("Element")
    juce.display_msg (conf, "ALSA",   conf.env.ALSA)
    juce.display_msg (conf, "JACK",   conf.env.JACK)
    juce.display_msg (conf, "AU",     juce.is_mac())
    juce.display_msg (conf, "VST2",   bool(conf.env.HAVE_VST))
    juce.display_msg (conf, "VST3",   True)
    juce.display_msg (conf, "LADSPA", bool(conf.env.LADSPA))
    juce.display_msg (conf, "LV2",    bool(conf.env.LV2))
    juce.display_msg (conf, "GtkUI",  bool(conf.env.GTKUI))
    juce.display_msg (conf, "Lua",    bool(conf.env.LUA))
    juce.display_msg (conf, "Workspaces", conf.options.enable_docking)
    juce.display_msg (conf, "Debug", conf.options.debug)

    print
    juce.display_msg (conf, "PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "DATADIR", conf.env.DATADIR)
    juce.display_msg (conf, "CFLAGS", conf.env.CFLAGS)
    juce.display_msg (conf, "CXXFLAGS", conf.env.CXXFLAGS)
    juce.display_msg (conf, "LINKFLAGS", conf.env.LINKFLAGS)

def common_includes():
    return [ 
        'libs/JUCE/modules', \
        'libs/kv/modules', \
        'libs/jlv2/modules', \
        'libs/compat', \
        'libs/lua', \
        'libs/lua/src', \
        'libs/lua-kv', \
        'libs/lua-kv/src', \
        'build/include', \
        VST3_PATH, \
        'src'
    ]

def lua_kv_sources (ctx):
    return ctx.path.ant_glob ('libs/lua-kv/src/kv/**/*.c') + \
           ctx.path.ant_glob ('libs/lua-kv/src/kv/**/*.cpp')

def juce_sources (ctx):
    return element.get_juce_library_code ("libs/compat") + \
           ctx.path.ant_glob ('libs/compat/BinaryData*.cpp')

def common_sources (ctx):
    return juce_sources (ctx) + lua_kv_sources (ctx) + \
            ctx.path.ant_glob ('src/**/*.cpp')

def build_desktop (bld, slug='element'):
    if not juce.is_linux():
        return

    if bool(bld.env.CONVERT):
        for size in '16 32 64 128 256 512'.split():
            geometry = '%sx%s' % (size, size)
            bld (
                rule   = '%s ${SRC} -resize %s ${TGT}' % (bld.env.CONVERT[0], geometry),
                source = 'data/ElementIcon.png',
                target = 'share/icons/hicolor/%s/apps/net.kushview.element.png' % geometry,
                install_path = bld.env.PREFIX + '/share/icons/hicolor/%s/apps' % geometry
            )
    else:
        bld (
            rule = 'cp -f ${SRC} ${TGT}',
            source = 'data/ElementIcon_512x512.png',
            target = 'share/icons/hicolor/512x512/apps/net.kushview.element.png',
            install_path = bld.env.PREFIX + '/share/icons/hicolor/512x512/apps'
        )

    src = "data/net.kushview.%s.desktop.in" % (slug)
    tgt = "share/applications/net.kushview.%s.desktop" % (slug)

    if os.path.exists (src):
        bld (features = "subst",
             source         = src,
             target         = tgt,
             name           = tgt,
             ELEMENT_EXE    = 'element',
             ELEMENT_ICON   = 'net.kushview.element',
             install_path   = bld.env.PREFIX + "/share/applications"
        )

    bld.install_files (bld.env.DATADIR, 'data/ElementIcon.png')

def build_lua_docs (bld):
    if bool(bld.env.LDOC):
        call ([bld.env.LDOC[0], '.' ])

def build_lua_lib (bld):
    lua = bld (
        features = 'c cstlib',
        includes = [
            'libs/lua/src'
        ],
        source = '''
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
            libs/lua/src/lapi.c
            libs/lua/src/lbaselib.c
            libs/lua/src/ldebug.c
            libs/lua/src/lparser.c
            libs/lua/src/llex.c
            libs/lua/src/ltm.c
            libs/lua/src/ldo.c
        '''.split(),
        name = 'LUA',
        target = 'lib/lua',
        install_path = None
    )
    bld.add_group()
    return lua

def compile_vst_linux (bld):
    libEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        libEnv.append_unique (k, [ '-fPIC' ])
    vst = bld (
        features    = 'cxx cxxshlib',
        source      = common_sources (bld),
        includes    = common_includes(),
        target      = 'lib/vst/Element',
        name        = 'ELEMENT_VST',
        env         = libEnv,
        use         = [ 'BOOST_SIGNALS' ]
    )
    if bld.env.LV2:     vst.use += [ 'SUIL', 'LILV', 'LV2' ]
    if bld.env.JACK:    vst.use += [ 'JACK' ]

    if juce.is_linux():
        build_desktop (bld)
        vst.use += [ 'FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL', 'GTK' ]

def compile_vst (bld):
    if juce.is_linux(): compile_vst_linux (bld)

def compile (bld):
    libEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        libEnv.append_unique (k, [ '-fPIC' ])
    library = bld (
        features    = 'cxx cxxshlib',
        source      = common_sources (bld),
        includes    = common_includes(),
        target      = 'lib/element-0',
        name        = 'ELEMENT',
        env         = libEnv,
        use         = [ 'BOOST_SIGNALS' ]
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
        build_desktop (bld)
        library.use += [ 'FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL', 'GTK' ]

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
    bld.add_pre_fun (configure_git_version)
    # build_lua_lib (bld)
    compile (bld)
    # compile_vst (bld)

    if bld.env.LUA and bool(bld.env.LIB_READLINE):
        bld.program(
            source = [ 'tools/lua-el/lua.cpp' ],
            name = 'lua-el',
            target = 'bin/lua-el',
            includes = common_includes(),
            use = [ 'ELEMENT', 'LUA', 'READLINE' ],
            install_path = None
        )

    if bld.env.TEST: bld.recurse ('tests')

def check (ctx):
    if not os.path.exists('build/bin/test-element'):
        ctx.fatal("Tests not compiled")
        return
    os.environ["LD_LIBRARY_PATH"] = "build/lib"
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
    ctx.excl += ' **/.gitignore **/.gitmodules **/.git dist deploy element* pro **/Builds **/build'
    ctx.excl += ' **/.DS_Store **/.vscode **/.travis.yml *.bz2 *.zip *.gz'
    ctx.excl += ' tools/jucer/**/JuceLibraryCode'

def docs (ctx):
    build_lua_docs (ctx)

from waflib.Build import BuildContext
class BuildDocs (BuildContext):
    cmd = 'docs'
    fun = 'docs'

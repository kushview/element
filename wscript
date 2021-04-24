#!/usr/bin/env python

from subprocess import call, Popen, PIPE
from waflib import Utils
import os, sys
import string
sys.path.insert (0, os.path.join (os.getcwd(), 'tools/waf'))
import cross, element, juce, git

APPNAME         = element.APPNAME
VERSION         = element.VERSION
PLUGIN_VERSION  = element.PLUGIN_VERSION

VST3_PATH = 'libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

def options (opt):
    opt.load ("scripting compiler_c compiler_cxx ccache cross juce")

    opt.add_option ('--disable-ladspa', default=False, action='store_true', dest='no_ladspa', \
        help="Disable LADSPA plugin hosting")
    opt.add_option ('--disable-lv2', default=False, action='store_true', dest='no_lv2', \
        help="Disable LV2 plugin hosting")
    opt.add_option ('--with-vstsdk24', default='', dest='vstsdk24', type='string', 
        help='Path to vstsdk2.4 sources')
    opt.add_option ('--disable-vst', default=False, action='store_true', dest='no_vst', \
        help="Disable VST2 plugin hosting")
    opt.add_option ('--disable-vst3', default=False, action='store_true', dest='no_vst3', \
        help="Disable VST3 plugin hosting")
    
    opt.add_option ('--enable-docking', default=False, action='store_true', dest='enable_docking', \
        help="Build with docking window support")
    
    opt.add_option ('--without-alsa', default=False, action='store_true', dest='no_alsa', \
        help="Build without ALSA support")
    opt.add_option ('--without-jack', default=False, action='store_true', dest='no_jack', \
        help="Build without JACK support")
    
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite")
    opt.add_option ('--ziptype', default='gz', dest='ziptype', type='string', 
        help='Zip type for waf dist (gz/bz2/zip) [ Default: gz ]')

    opt.add_option ('--minimal', dest='minimal', default=False, action='store_true')

def silence_warnings (conf):
    '''TODO: resolve these'''
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-declarations'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-declarations'])
    
    if 'clang' in conf.env.CXX[0]:
        conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CFLAGS', ['-Wno-dynamic-class-memaccess'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-dynamic-class-memaccess'])

def configure (conf):
    conf.env.DATADIR = os.path.join (conf.env.PREFIX,  'share/element')
    conf.env.DOCDIR  = os.path.join (conf.env.PREFIX,  'share/doc/element')
    conf.env.LIBDIR  = os.path.join (conf.env.PREFIX,  'lib')
    conf.env.VSTDIR  = os.path.join (conf.env.LIBDIR,  'vst')
    conf.env.VST3DIR = os.path.join (conf.env.LIBDIR,  'vst3')
    
    conf.load ('bundle')
    conf.load ('templates')
    conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx")
    conf.check_cxx_version()
    conf.load ('ccache')
    conf.load ("git")
    conf.load ("juce")
    conf.load ('scripting')

    silence_warnings (conf)

    conf.find_projucer (mandatory=False)
    conf.find_program ('convert', mandatory=False)
    
    conf.check_common()
    if cross.is_mingw (conf): conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.env.TEST = bool(conf.options.test)
    conf.env.DEBUG = conf.options.debug
    
    conf.define ('EL_DOCKING', True if conf.options.enable_docking else False)
    conf.define ('KV_DOCKING_WINDOWS', 1)
    if len(conf.env.GIT_HASH) > 0:
        conf.define ('EL_GIT_VERSION', conf.env.GIT_HASH)

    # Hidden Visibiility by default
    for k in 'CFLAGS CXXFLAGS'.split():
        conf.env.append_unique (k, ['-fvisibility=hidden'])

    print
    juce.display_header ("Element")
    conf.message ("Config", 'Debug' if conf.options.debug else 'Release')
    conf.message ("Workspaces", conf.options.enable_docking)
    conf.message ("Scripting",  bool(conf.env.LUA))
    conf.message ("Workspaces", conf.options.enable_docking)
    print
    juce.display_header ("Audio Devices")
    conf.message ("Alsa",       conf.env.ALSA)
    conf.message ("JACK Audio", conf.env.JACK)
    print
    juce.display_header ("Plugin Host")
    conf.message ("AudioUnit",  juce.is_mac())
    conf.message ("VST2",       conf.env.VST)
    conf.message ("VST3",       conf.env.VST3)
    conf.message ("LADSPA",     bool(conf.env.LADSPA))
    conf.message ("LV2",        bool(conf.env.LV2))
    print
    juce.display_header ("Plugin Clients")
    conf.message ("VST2",       conf.env.VST)
    print
    juce.display_header ("Paths")
    conf.message ("Prefix",      conf.env.PREFIX)
    conf.message ("Data",        conf.env.DATADIR)
    conf.message ("Modules",     conf.env.LUADIR)
    conf.message ("Scripts",     conf.env.SCRIPTSDIR)
    if conf.env.VST:
        conf.message ("VST",     conf.env.VSTDIR)
    print
    juce.display_header ("Compiler")
    conf.display_archs()
    conf.message ("CC",             ' '.join (conf.env.CC))
    conf.message ("CXX",            ' '.join (conf.env.CXX))
    conf.message ("CFLAGS",         conf.env.CFLAGS)
    conf.message ("CXXFLAGS",       conf.env.CXXFLAGS)
    conf.message ("LINKFLAGS",      conf.env.LINKFLAGS)

def common_includes():
    return [
        VST3_PATH, \
        'libs/JUCE/modules', \
        'libs/kv/modules', \
        'libs/jlv2/modules', \
        'libs/compat', \
        'libs/element/lua', \
        'build/include', \
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
        call ([bld.env.LDOC[0], '-f', 'markdown', '.' ])

def build_lua_lib (bld):
    luaEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        luaEnv.append_unique (k, [ '-fPIC' ])
    lua = bld (
        name     = 'LUA',
        target   = 'lib/lua',
        env      = luaEnv,
        install_path = None,
        features = 'cxx cxxstlib',
        includes = [
            'libs/lua',
            'libs/lua/src',
            'libs/lua-kv',
            'libs/lua-kv/src',
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
        '''.split()
    )
    lua.export_includes = lua.includes
    return lua

def add_scripts_to (bld, builddir, instdir, 
                    modsdir='Modules', 
                    scriptsdir='Scripts'):
    for node in bld.path.ant_glob ('libs/element/lua/el/*.lua'):
        s = bld (
            features    ='subst', 
            source      = node,
            target      = '%s/%s/el/%s' % (builddir, modsdir, node.name),
            install_path= '%s/%s/el' % (instdir, modsdir) if instdir else None
        )
    for node in bld.path.ant_glob ('libs/lua-kv/src/kv/*.lua'):
        bld (
            features    ='subst',
            source      = node,
            target      = '%s/%s/kv/%s' % (builddir, modsdir, node.name),
            install_path= '%s/%s/kv' % (instdir, modsdir) if instdir else None
        )
    for node in bld.path.ant_glob ('scripts/**/*.lua'):
        bld (
            features    ='subst',
            source      = node,
            target      = '%s/%s/%s' % (builddir, scriptsdir, node.name),
            install_path= '%s/%s' % (instdir, scriptsdir) if instdir else None
        )

def build_vst_linux (bld, plugin):
    vstEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        vstEnv.append_unique (k, [ '-fPIC' ])
    vstEnv.cxxshlib_PATTERN = bld.env.plugin_PATTERN
    vst = bld (
        features        = 'cxx cxxshlib',
        source          = [
            'tools/jucer/%s/Source/%s.cpp' % (plugin, plugin),
            'libs/compat/include_juce_audio_plugin_client_VST2.cpp'
        ],
        includes        = common_includes(),
        target          = 'plugins/VST/%s' % plugin,
        name            = 'ELEMENT_VST',
        env             = vstEnv,
        use             = [ 'ELEMENT', 'LUA' ],
        install_path    = '%s/Kushview' % bld.env.VSTDIR,
    )

def build_vst (bld):
    if not bld.env.VST:
        return
    if juce.is_linux():
        for plugin in 'Element ElementFX'.split():
            build_vst_linux (bld, plugin)

def vst3_bundle_arch():
    if juce.is_mac():
        return 'MacOS'
    if juce.is_linux():
        return 'x86_64-linux'
    return ''

def build_vst3_linux (bld, plugin):
    if not bld.env.VST:
        return

    vstEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        vstEnv.append_unique (k, [ '-fPIC' ])
    
    vstEnv.cxxshlib_PATTERN = bld.env.plugin_PATTERN
    vst3 = bld.bundle (
        name            = '%s_BUNDLE_VST3' % plugin,
        target          = 'plugins/VST3/%s.vst3' % plugin,
        install_path    = '%s/Kushview' % (bld.env.VST3DIR)
    )

    binary = vst3.bundle_create_child ('Contents/%s/%s' % (vst3_bundle_arch(), plugin),
        features        = 'cxx cxxshlib',
        name            = '%s_VST3' % plugin,        
        source          = [
            'tools/jucer/%s/Source/%s.cpp' % (plugin, plugin),
            'libs/compat/include_juce_audio_plugin_client_VST3.cpp'
        ],
        includes        = common_includes(),
        env             = vstEnv,
        use             = [ 'ELEMENT', 'LUA', vst3.name ]
    )

    add_scripts_to (bld,
        '%s/Contents/Resources' % vst3.bundle_node(),
        '%s/Contents/Resources' % vst3.bundle_install_path()
    )

def build_vst3 (bld):
    if juce.is_linux():
        for plugin in 'Element ElementFX'.split():
            build_vst3_linux (bld, plugin)

def copy_app_bundle_lua_files(ctx):
    if not juce.is_mac():
        return
    call (['bash', 'tools/osx-lua.copy.sh'])

def build_app (bld):
    libEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        libEnv.append_unique (k, [ '-fPIC' ])
    
    library = bld (
        features    = 'cxx cxxstlib',
        source      = common_sources (bld),
        includes    = common_includes(),
        target      = 'lib/element',
        name        = 'ELEMENT',
        env         = libEnv,
        use         = [ 'BOOST_SIGNALS' ],
        cxxflags    = [],
        install_path = None
    )
    library.export_includes = library.includes
    
    appEnv = bld.env.derive()
    app = bld.program (
        source      = [ 'src/Main.cc' ],
        includes    = common_includes(),
        target      = 'bin/element',
        name        = 'ElementApp',
        env         = appEnv,
        use         = [ 'LUA', 'ELEMENT' ],
        linkflags   = []
    )

    if bld.env.LUA:     library.use += [ 'LUA' ]
    if bld.env.LV2:     library.use += [ 'SUIL', 'LILV', 'LV2' ]
    if bld.env.JACK:    library.use += [ 'JACK' ]
    
    if juce.is_linux():
        build_desktop (bld)
        library.use += ['FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL']
        library.cxxflags += [
            '-DLUA_PATH_DEFAULT="%s"'  % libEnv.LUA_PATH_DEFAULT,
            '-DLUA_CPATH_DEFAULT="%s"' % libEnv.LUA_CPATH_DEFAULT,
            '-DEL_LUADIR="%s"'         % libEnv.LUADIR,
            '-DEL_SCRIPTSDIR="%s"'     % libEnv.SCRIPTSDIR,
            '-DEL_API_DOCS_URL="file://%s"' % os.path.join (libEnv.DOCDIR, 'lua', 'index.html')
        ]

    elif juce.is_mac():
        library.use += [
            'ACCELERATE', 'AUDIO_TOOLBOX', 'AUDIO_UNIT', 'CORE_AUDIO', 
            'CORE_AUDIO_KIT', 'COCOA', 'CORE_MIDI', 'IO_KIT', 'QUARTZ_CORE',
            'TEMPLATES'
        ]
        app.target       = 'Applications/Element'
        app.mac_app      = True
        app.mac_plist    = 'data/Info.plist'
        app.mac_files    = [ 'data/Icon.icns' ]
        add_scripts_to (bld, '%s.app/Contents/Resources' % app.target, None)

    else:
        pass

def install_lua_files (bld):
    if not juce.is_linux():
        return
    path = bld.path
    join = os.path.join
    bld.install_files (bld.env.SCRIPTSDIR,
                       path.ant_glob ("scripts/**/*.lua"),
                       relative_trick=True,
                       cwd=path.find_dir ('scripts'))

    bld.install_files (join (bld.env.DOCDIR, 'lua'),
                       bld.path.ant_glob ("build/doc/lua/**/*.*"),
                       relative_trick=True,
                       cwd=bld.path.find_dir ('build/doc/lua'))

    bld.install_files (bld.env.LUADIR,
                       bld.path.ant_glob ("libs/lua-kv/src/**/*.lua"),
                       relative_trick=True,
                       cwd=bld.path.find_dir ('libs/lua-kv/src'))

    bld.install_files (bld.env.LUADIR,
                       bld.path.ant_glob ("libs/element/lua/**/*.lua"),
                       relative_trick=True,
                       cwd=bld.path.find_dir ('libs/element/lua'))

def build (bld):
    if bld.is_install:
        if juce.is_mac(): bld.fatal ("waf install not supported on OSX")
        if bld.options.minimal: return

    if bld.cmd == 'build' and git.is_repo():
        bld.add_pre_fun (bld.git_update_env)

    bld.template (
        name = 'TEMPLATES',
        source = bld.path.ant_glob ('src/**/*.h.in') \
               + bld.path.ant_glob ('data/**/*.in') \
               + bld.path.ant_glob ("tools/**/*.in"),
        install_path = None,
        PACKAGE_VERSION = VERSION
    )

    bld.add_group()
    if bld.options.minimal:
        return

    build_lua_lib (bld)
    install_lua_files (bld)
    build_app (bld)
    build_vst (bld)
    build_vst3 (bld)

    if bld.env.LUA and bool (bld.env.LIB_READLINE):
        bld.recurse ('tools/lua-el')
    if bld.env.TEST:
        bld.recurse ('test')

def check (ctx):
    if not os.path.exists('build/bin/test_juce'):
        ctx.fatal ("JUCE tests not compiled")
        return
    if not os.path.exists('build/bin/test_element'):
        ctx.fatal ("Test suite not compiled")
        return
    os.environ["LD_LIBRARY_PATH"] = "build/lib"
    failed = 0
    print ("JUCE Tests")
    if 0 != call (["build/bin/test_juce"]):
        failed += 1
    print ("Done!\n")

    print ("Element Test Suite")
    if 0 != call (["build/bin/test_element"]):
        failed += 1
    if (failed > 0):
        ctx.fatal ("Test suite exited with fails")

def dist (ctx):
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
    ctx.add_pre_fun (build_lua_docs)

def versionbump (ctx):
    import projects
    ctx.add_pre_fun (projects.update_version)

def resave (ctx):
    import projects
    ctx.add_pre_fun (projects.resave)

from waflib.Build import BuildContext

class ResaveBuildContext (BuildContext):
    cmd  = 'resave'
    fun  = 'resave'

class DocsBuildContext (BuildContext):
    cmd = 'docs'
    fun = 'docs'

class VersionBumpContext (BuildContext):
    cmd = 'versionbump'
    fun = 'versionbump'

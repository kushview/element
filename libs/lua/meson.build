
lua_includes = include_directories ('.', 'src')
lua_c_args = []
if host_machine.system() == 'linux'
    lua_c_args += '-DLUA_USE_LINUX'
elif host_machine.system() == 'darwin'
    lua_c_args += '-DLUA_USE_MACOSX'
elif host_machine.system() == 'windows'
    # not actually needed
    # lua_c_args += '-DLUA_USE_WINDOWS'
endif

libelement_lua = static_library ('element_lua', '''
    src/lauxlib.c
    src/liolib.c
    src/lopcodes.c
    src/lstate.c
    src/lobject.c
    src/lmathlib.c
    src/loadlib.c
    src/lvm.c
    src/lfunc.c
    src/lstrlib.c
    src/linit.c
    src/lstring.c
    src/lundump.c
    src/lctype.c
    src/ltable.c
    src/ldump.c
    src/loslib.c
    src/lgc.c
    src/lzio.c
    src/ldblib.c
    src/lutf8lib.c
    src/lmem.c
    src/lcorolib.c
    src/lcode.c
    src/ltablib.c
    src/lapi.c
    src/lbaselib.c
    src/ldebug.c
    src/lparser.c
    src/llex.c
    src/ltm.c
    src/ldo.c'''.split(),
    include_directories : lua_includes,
    install : false,
    gnu_symbol_visibility : 'default',
    c_args : lua_c_args
)

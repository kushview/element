project ('element', ['c', 'cpp' ],
    version : '0.47.0',
    meson_version : '>= 0.58.0',
    default_options : [ 'c_std=c11', 'cpp_std=c++17' ])

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# Add objective-c if on mac
if host_machine.system() == 'darwin'
  add_languages(['objc', 'objcpp'], native : false, required : true)
  objcc = meson.get_compiler ('objc')
  objcpp = meson.get_compiler ('objcpp')
endif

# Boost
boost_dep = dependency ('boost', required : true)
deps = [ boost_dep ]

# Filesystem
if not cpp.check_header ('filesystem', required : false)
    cpp.check_header ('experimental/filesystem', required : true)
    if host_machine.system() == 'darwin'
        deps += cpp.find_library ('c++fs', required : true)
    elif cpp.get_argument_syntax() == 'gcc'
        deps += cpp.find_library ('stdc++fs', required : true)
    endif
endif

subdir ('libs/lua')
subdir ('libs/compat')
subdir ('include/element/juce')
subdir ('scripts')
subdir ('src')

###############################################################################
# Element Application
element_app_link_args = []
element_app_deps = deps
element_app_sources = [ 'src/main.cpp' ]

if host_machine.system() == 'windows'
    windows = import ('windows')
    element_app_sources += windows.compile_resources ('tools/windeploy/resources.rc',
        include_directories :  ['tools/windeploy'])

    if cpp.get_id() == 'gcc' and cpp.get_argument_syntax() == 'gcc'
        foreach l : 'uuid wsock32 wininet version ole32 ws2_32 oleaut32 imm32 comdlg32 shlwapi rpcrt4 winmm dxgi'.split()
            element_app_deps += cpp.find_library (l, required : true)
        endforeach
        element_app_link_args += [ '-static-libgcc', '-static-libstdc++',
                                   '-Wl,-Bstatic,--whole-archive', '-lwinpthread', 
                                   '-Wl,--no-whole-archive' ]
    endif
endif

subdir ('test')

if not meson.is_subproject()
element_app = executable ('element', element_app_sources,
    install : true,
    win_subsystem : 'windows',
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ 'src', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

subdir ('src/plugins')

if host_machine.system() == 'linux'
    subdir ('tools/lindeploy')
elif host_machine.system() == 'darwin'
    subdir ('tools/macdeploy')
elif host_machine.system() == 'windows'
    subdir ('tools/windeploy')
endif
endif

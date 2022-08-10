project ('element', ['c', 'cpp' ] + meson.get_external_property ('extra_languages', []),
    version : '0.47.0',
    meson_version : '>= 0.58.0',
    default_options : [ 'c_std=c11', 'cpp_std=c++17' ])

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# Global Arguments

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
subdir ('scripts')
subdir ('src')

if not meson.is_subproject()
###############################################################################
# Element Application
element_app_link_args = []
element_app_deps = deps
element_app_sources = [ 'src/Main.cc' ]

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

element_app = executable ('element', element_app_sources,
    install : true,
    win_subsystem : 'windows',
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ 'src', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

if host_machine.system() == 'linux'
    custom_target ('desktop icon',
        command : [ 'cp', '-f', '@INPUT@', '@OUTPUT@' ],
        input   : 'data/ElementIcon_256x256.png',
        output  : 'net.kushview.element.png',
        install : true,
        install_dir : get_option('datadir') / 'icons/hicolor/256x256/apps')
    configure_file (
        input: 'data/net.kushview.element.desktop.in',
        output: 'net.kushview.element.desktop',
        configuration : {
            'ELEMENT_ICON' : 'net.kushview.element',
            'ELEMENT_EXE'  : 'element'
        },
        install : true,
        install_dir : get_option('datadir') / 'applications')
elif host_machine.system() == 'darwin'
    install_data ('data/Icon.icns', install_dir : 'Contents/Resources')
    install_data ('tools/macdeploy/Info.plist', install_dir : 'Contents')
elif host_machine.system() == 'windows'
    subdir ('tools/windeploy')
endif
endif
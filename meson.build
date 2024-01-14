project ('element', ['c', 'cpp' ],
    version : '1.0.0',
    meson_version : '>= 0.58.0',
    default_options : [ 
        'c_std=c11', 
        'cpp_std=c++17', 
        'buildtype=release' ])

pymod       = import ('python')
python3     = pymod.find_installation ('python3', required : true)
bundle_py   = files ('util/bundle.py').get(0)
date_py     = find_program ('util/date.py')
version_py  = find_program ('util/version.py')

## Build & version information
element_release_date = run_command (python3, date_py, check: true).stdout().strip()
version_with_build   = run_command ([python3, version_py] + '--build --ignore-dirty --cwd=.'.split(), check: true).stdout().strip()
build_number         = run_command ([python3, version_py] + '--build --build-style=onlybuild --ignore-dirty --cwd=.'.split(), check: true).stdout().strip()
git_hash             = run_command ([python3, version_py] + '--hash --cwd=.'.split(), check: true).stdout().strip()
git_short_hash       = run_command ([python3, version_py] + '--short-hash --cwd=.'.split(), check: true).stdout().strip()

## Updater integration
if get_option('updater')
    add_project_arguments ('-DEL_UPDATER=1', language: ['cpp', 'objcpp'])
    updater_host = get_option('updater-host').strip()
    if updater_host != ''
        add_project_arguments ('-DEL_UPDATE_REPOSITORY_HOST="@0@"'.format(updater_host), 
                                language: ['cpp', 'objcpp'])
    endif
endif

## Installer Metadata
net_kushview      = 'net.kushview'
installerdir      = get_option('prefix')
packagesdir       = installerdir / 'packages'
packagedir_element = packagesdir / net_kushview + '.element'
GPL3_txt = configure_file (copy: true, 
    input : files ('LICENSES/GPL3.txt'),
    output: 'GPL3.txt', 
    install : get_option ('deploy'),
    install_dir : packagedir_element / 'meta')

## This collects app and plugin bundles needing code signing on Mac OS
element_bundles = []

## This collects app and plugin bundles needing code signing on Windows
element_binaries = []

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# GCC
if cpp.get_id() == 'gcc'
    add_project_arguments('-Wno-attributes', language: ['cpp'])
endif

# No delete
nodelete_cpp_link_args = cpp.get_supported_link_arguments(['-Wl,-z,nodelete'])

# Add objective-c if on mac
if host_machine.system() == 'darwin'
  add_languages(['objc', 'objcpp'], native : false, required : true)
  objcc = meson.get_compiler ('objc')
  objcpp = meson.get_compiler ('objcpp')
endif

element_include_subdir = 'element-1.0' / 'element'
element_include_dir = get_option ('includedir') / element_include_subdir

subdir ('meson/juce')
subdir ('meson/deps')

deps = [ boost_dep, lv2_dep, lilv_dep, suil_dep, lvtk_host_dep ]

# Filesystem
if not cpp.check_header ('filesystem', required : false)
    cpp.check_header ('experimental/filesystem', required : true)
    if host_machine.system() == 'darwin'
        deps += cpp.find_library ('c++fs', required : true)
    elif cpp.get_argument_syntax() == 'gcc'
        deps += cpp.find_library ('stdc++fs', required : true)
    endif
endif

element_includes = include_directories ('include')

###############################################################################
subdir ('src/lua')
subdir ('src/juce')
subdir ('util')
subdir ('scripts')
subdir ('src')

###############################################################################
subdir ('include/element')
subdir ('include/element/juce')

subdir ('docs')

###############################################################################
# Element Application
element_app_link_args = []
element_app_deps = [ deps, juce_dep ]
element_app_sources = [ 'src/main.cpp' ]
element_exe_install = true
element_exe_install_dir = get_option ('bindir')
element_exe_name = 'element'

if host_machine.system() == 'linux'
    if get_option('deploy')
        element_exe_install_dir = packagedir_element / 'data' / 'bin'
        element_app_link_args += [ '-static-libgcc', '-static-libstdc++' ]
    endif
elif host_machine.system() == 'darwin'
    element_exe_install = false
    element_exe_name = 'Element'
elif host_machine.system() == 'windows'
    element_exe_install_dir = packagedir_element / 'data' / 'bin'
    windows = import ('windows')
    element_app_sources += windows.compile_resources ('src/res/resources.rc',
        include_directories :  ['src/res'])

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

if not get_option ('element-apps').disabled()
    element_app = executable (element_exe_name, element_app_sources,
        install : element_exe_install,
        install_dir : element_exe_install_dir,
        win_subsystem : 'windows',
        gnu_symbol_visibility : 'hidden',
        dependencies : element_app_deps,
        include_directories : [ 'src', libelement_includes ],
        link_args : [ element_app_link_args, nodelete_cpp_link_args ],
        link_with : [ libelement ])
    element_binaries += element_app
endif

subdir ('plugins')

app_logo_png = files ('data/ElementIcon.png')
app_icon = app_logo_png
if host_machine.system() == 'windows'
    app_icon = files ('data/ElementIcon.ico')
elif host_machine.system() == 'darwin'
    app_icon = files ('deploy/osx/Icon.icns')
endif

subdir ('deploy')

if get_option('sign')
    if host_machine.system() == 'darwin'
        identity = get_option ('codesign-identity')
        entitlements = files ('deploy/osx/app.entitlements').get(0)

        foreach bundle : element_bundles
            signed_name = fs.name(bundle.full_path())+'_signed'
            codesign_command = [ find_program ('codesign', required : true),
                    '--verbose=2', '--timestamp', 
                    '--sign', get_option ('codesign-identity'),
                    '--options', 'runtime', '--deep', '--force',
                    '--entitlements', entitlements,
                    '@INPUT@' ]
            custom_target (signed_name,
                        command: codesign_command,
                        input: bundle,
                        output: signed_name,
                        build_by_default: true,
                        install: false)
        endforeach
    elif host_machine.system() == 'windows'
        signtool = find_program ('SignTool', required : true)
        foreach binary : element_binaries
            signed_name = fs.name(binary.full_path())+'_signed'
            custom_target (signed_name,
                command: [ signtool, 'sign', '/as', '/v', '/sha1', get_option ('signtool-sha1'),
                    '/fd', 'SHA256', '/tr', 'http://sha256timestamp.ws.symantec.com/sha256/timestamp', '@INPUT@' 
                ],
                input: binary,
                output: signed_name,
                build_by_default: true,
                install: false)
        endforeach
    endif 
endif # end code signing.

summary ('JACK', jack_enabled, bool_yn: true, section: 'Application')

summary ('Prefix', get_option('prefix'), section : 'Paths')
summary ('Programs', get_option('prefix') / get_option('bindir'), section : 'Paths')
summary ('Headers', get_option('prefix') / get_option('includedir'), section : 'Paths')
summary ('Libraries', get_option('prefix') / get_option('libdir'), section : 'Paths')

summary ('Plugins', not get_option ('element-plugins').disabled(), 
    bool_yn: true, section: 'Application')

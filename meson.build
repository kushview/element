project ('element', ['c', 'cpp' ],
    version : '1.0.0',
    meson_version : '>= 0.58.0',
    default_options : [ 
        'c_std=c11', 
        'cpp_std=c++17', 
        'buildtype=release' ])

bundle_py   = find_program ('util/bundle.py')
date_py     = find_program ('util/date.py')
version_py  = find_program ('util/version.py')
element_release_date = run_command (date_py).stdout().strip()
version_with_build = run_command ([version_py] + '--build --ignore-dirty --cwd=.'.split()).stdout().strip()
build_number = run_command ([version_py] + '--build --build-style=onlybuild --ignore-dirty --cwd=.'.split()).stdout().strip()
git_hash = run_command ([version_py] + '--hash --cwd=.'.split()).stdout().strip()
git_short_hash = run_command ([version_py] + '--short-hash --cwd=.'.split()).stdout().strip()

net_kushview      = 'net.kushview'
installerdir      = get_option('prefix')
packagesdir       = installerdir / 'packages'
packagedir_element = packagesdir / net_kushview + '.element'
GPL3_txt = configure_file (copy: true, 
    input : files ('LICENSES/GPL3.txt'),
    output: 'GPL3.txt', install : get_option ('deploy'),
    install_dir : packagedir_element / 'meta')

element_bundles = []

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# Add objective-c if on mac
if host_machine.system() == 'darwin'
  add_languages(['objc', 'objcpp'], native : false, required : true)
  objcc = meson.get_compiler ('objc')
  objcpp = meson.get_compiler ('objcpp')
endif

element_include_subdir = 'element-1.0' / 'element'
element_include_dir = get_option ('includedir') / element_include_subdir
subdir ('meson/juce')

# Boost
boost_dep = dependency ('boost', required : true)
lvtk_proj = subproject ('lvtk')
lvtk_host_dep = lvtk_proj.get_variable ('lvtk_host_dep')

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

if host_machine.system() == 'darwin'
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
else
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
    link_args : element_app_link_args,
    link_with : [ libelement ])
endif

subdir ('plugins')
subdir ('deploy')



if get_option('sign')
if host_machine.system() == 'darwin'
    macdeployqt = find_program ('macdeployqt', required : true)
    identity = get_option ('codesign-identity')

    foreach bundle : element_bundles
        command = [ macdeployqt, bundle.full_path(), 
            '-no-plugins', '-always-overwrite',
            '-verbose=1', '-codesign='+identity, '-hardened-runtime', 
            '-sign-for-notarization='+identity, '-timestamp' ]
        signed_name = fs.name(bundle.full_path())+'_signed'
        custom_target (signed_name,
            command : command, input: bundle, output: signed_name,
            build_by_default: true, install: false)
    endforeach
endif # end darwin
endif # end code signing.

summary ('Prefix', get_option('prefix'), section : 'Paths')
summary ('Programs', get_option('prefix') / get_option('bindir'), section : 'Paths')
summary ('Headers', get_option('prefix') / get_option('includedir'), section : 'Paths')
summary ('Libraries', get_option('prefix') / get_option('libdir'), section : 'Paths')

summary ('Plugins', get_option ('element-plugins'))

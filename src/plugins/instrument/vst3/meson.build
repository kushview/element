
if host_machine.system() == 'windows'
    element_vst3_name_suffix = 'vst3'
    element_vst3_bindir = 'x86_64-win'
elif host_machine.system() == 'linux'
    element_vst3_name_suffix = 'so'
    element_vst3_bindir = run_command ('uname', '-m', capture : true, check : true).stdout().strip()
    element_vst3_bindir += '-linux'
elif host_machine.system() == 'darwin'
    element_vst3_name_suffix = 'dylib'
    element_vst3_bindir = 'MacOS'
endif

# Element Instrument
element_vst3_install_dir = 'lib' / 'vst3' / 'KV_Element.vst3' / 'Contents' / element_vst3_bindir
element_vst3 = shared_module ('KV_Element', [
        '../plugin.cpp',
        '../../vst3client.cpp',
        '../../utilities.cpp'
    ],
    name_prefix : '',
    name_suffix : element_vst3_name_suffix,
    install : true,
    install_dir : element_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST3=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

if host_machine.system() == 'darwin'
configure_file (
    input: '../../plugin.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV_ElementFX.dylib',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
        'BUNDLE_NAME'          : 'KV_ElementFX',
        'BUNDLE_DISPLAY_NAME'  : 'Element FX',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : 'lib' / 'vst3' / 'KV_Element.vst3' / 'Contents'
)
endif

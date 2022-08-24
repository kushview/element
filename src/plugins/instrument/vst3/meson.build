
# Element Instrument
element_vst3_sources = [
    '../plugin.cpp',
    '../../vst3client.cpp',
    '../../utilities.cpp'
]

if host_machine.system() == 'darwin'
    element_vst3_sources += '../../macvstutils.mm'
endif

element_vst3_install_dir = 'lib' / 'vst3' / 'KV-Element.vst3' / 'Contents' / element_vst3_bindir
element_vst3 = shared_module ('KV-Element',
    element_vst3_sources,
    name_prefix : '',
    name_suffix : element_vst3_name_suffix,
    install : true,
    install_dir : element_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST3=1' ],
    objcpp_args : [ '-DJucePlugin_Build_VST3=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

if host_machine.system() == 'darwin'
configure_file (
    input: '../../plugin.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV-Element.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.Element',
        'BUNDLE_NAME'          : 'KV-Element',
        'BUNDLE_DISPLAY_NAME'  : 'Element',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : 'lib' / 'vst3' / 'KV-Element.vst3' / 'Contents'
)
endif

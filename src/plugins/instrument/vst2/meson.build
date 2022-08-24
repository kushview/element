if aeffect_h_found
element_vst2_sources = [
    '../plugin.cpp',
    '../../vst2client.cpp'
]

element_vst2_install_dir = 'lib' / 'vst'

if host_machine.system() == 'darwin'
    element_vst2_sources += '../../macvstutils.mm'
    element_vst2_contents_dir = element_vst2_install_dir / 'KV_Element.vst' / 'Contents'
    element_vst2_install_dir = element_vst2_contents_dir / 'MacOS'
endif

element_vst2 = shared_module ('KV_Element',
    element_vst2_sources,
    name_prefix : '',
    install : true,
    install_dir : element_vst2_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST=1' ],
    objcpp_args : [ '-DJucePlugin_Build_VST=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

if host_machine.system() == 'darwin'
configure_file (
    input: '../../plugin.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV_Element.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.Element',
        'BUNDLE_NAME'          : 'KV_Element',
        'BUNDLE_DISPLAY_NAME'  : 'Element',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : element_vst2_contents_dir
)
endif
endif

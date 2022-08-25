if aeffect_h_found
element_fx_vst2_sources = [
    '../plugin.cpp',
    '../../vst2client.cpp'
]

element_fx_vst2_install_dir = 'lib' / 'vst'

if host_machine.system() == 'darwin'
    element_fx_vst2_sources += '../../macvstutils.mm'
    element_fx_vst2_contents_dir = element_fx_vst2_install_dir / 'KV-Element-FX.vst' / 'Contents'
    element_fx_vst2_install_dir = element_fx_vst2_contents_dir / 'MacOS'
endif

element_fx_vst2 = shared_module ('KV-Element-FX',
    element_fx_vst2_sources,
    name_prefix : '',
    name_suffix : host_machine.system() == 'darwin' ? 'bundle' : [],
    install : true,
    install_dir : element_fx_vst2_install_dir,
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
        'BUNDLE_EXECUTABLE'    : 'KV-Element-FX.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
        'BUNDLE_NAME'          : 'KV-Element-FX',
        'BUNDLE_DISPLAY_NAME'  : 'Element FX',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : element_fx_vst2_contents_dir
)
endif
endif

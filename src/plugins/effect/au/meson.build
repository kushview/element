if host_machine.system() == 'darwin'

element_fx_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_fx_au_install_dir = 'lib' / 'au'

if host_machine.system() == 'darwin'
    element_fx_au_sources += '../../macvstutils.mm'
    element_fx_au_contents_dir = element_fx_au_install_dir / 'KV-Element-FX.component' / 'Contents'
    element_fx_au_install_dir = element_fx_au_contents_dir / 'MacOS'
endif

element_au = shared_module ('KV-Element-FX',
    element_fx_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : true,
    install_dir : element_fx_au_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes, juce_au_includes ],
    cpp_args : [ '-DJucePlugin_Build_AU=1' ],
    objcpp_args : [ '-DJucePlugin_Build_AU=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV-Element-FX.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
        'BUNDLE_NAME'          : 'KV-Element-FX',
        'BUNDLE_DISPLAY_NAME'  : 'Element FX',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : element_fx_au_contents_dir
)

endif

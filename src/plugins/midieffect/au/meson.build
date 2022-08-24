if host_machine.system() == 'darwin'

element_mfx_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_mfx_au_install_dir = 'lib' / 'au'

if host_machine.system() == 'darwin'
    element_mfx_au_sources += '../../macvstutils.mm'
    element_mfx_au_contents_dir = element_mfx_au_install_dir / 'KV_ElementMFX.component' / 'Contents'
    element_mfx_au_install_dir = element_mfx_au_contents_dir / 'MacOS'
endif

element_au = shared_module ('KV_ElementMFX',
    element_mfx_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : true,
    install_dir : element_mfx_au_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_AU=1' ],
    objcpp_args : [ '-DJucePlugin_Build_AU=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV_ElementMFX.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementMFX',
        'BUNDLE_NAME'          : 'KV_ElementMFX',
        'BUNDLE_DISPLAY_NAME'  : 'Element',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : element_mfx_au_contents_dir
)

endif

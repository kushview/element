if host_machine.system() == 'darwin'

element_mfx_au_code = 'ElMX'

element_mfx_au_juce_args = [
    '-DJucePlugin_Build_AU=1', 
    '-DJucePlugin_Version=@0@'.format (meson.project_version()),
    '-DJucePlugin_VersionString="@0@"'.format(meson.project_version()),
    '-DJucePlugin_VersionCode=@0@'.format (element_plugin_hex_verson_str),
    '-DJucePlugin_ManufacturerCode=@0@'.format (element_manu_hex_code_str),
    '-DJucePlugin_PluginCode=@0@'.format (run_command (
        getcode_py, '--code', element_mfx_au_code).stdout().strip())
]

fs = import ('fs')

element_mfx_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_mfx_au_install_dir = fs.expanduser ('~/Library/Audio/Plug-Ins/Components')

element_mfx_au = shared_module ('KV-Element-MFX',
    element_mfx_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : false,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes, juce_au_includes ],
    cpp_args : [ element_mfx_au_juce_args ],
    objcpp_args : [ element_mfx_au_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

element_mfx_au_plist = configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV-Element-MFX.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementMFX',
        'BUNDLE_NAME'          : 'KV-Element-MFX',
        'BUNDLE_DISPLAY_NAME'  : 'Element FX',
        'BUNDLE_VERSION'       : meson.project_version(),
        'EXPORT_PREFIX'        : 'ElementMFX',
        'AU_NAME'              : 'Kushview: Element MFX',
        'AU_DESCRIPTION'       : 'Element modular MIDI effect.',
        'AU_INT_VERSION'       : element_plugin_int_version,
        'TYPE'                 : 'aumi',
        'SUBT'                 : element_mfx_au_code,
        'MANU'                 : element_manu_code
    },
    install : false
)

element_bundles += custom_target (
    'KV-Element-MFX.component',
    build_by_default: true,
    command: [ bundle_py, '--binary', '@INPUT@',
                '--output', '@OUTPUT@',
                '--plist', element_mfx_au_plist,
                '--type', 'bundle' ],
    input: element_mfx_au,
    output: 'KV-Element-MFX.component',
    install: get_option ('deploy'),
    install_dir: element_au_install_dir
)

endif

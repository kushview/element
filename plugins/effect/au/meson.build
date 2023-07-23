if host_machine.system() == 'darwin'

element_fx_au_juce_args = [
    '-DJucePlugin_Build_AU=1', 
    '-DJucePlugin_Version=@0@'.format (meson.project_version()),
    '-DJucePlugin_VersionString="@0@"'.format(meson.project_version()),
    '-DJucePlugin_VersionCode=@0@'.format (element_plugin_hex_verson_str),
    '-DJucePlugin_ManufacturerCode=@0@'.format (element_manu_hex_code_str),
    '-DJucePlugin_PluginCode=@0@'.format (run_command (python3, 
        getcode_py, '--code', element_fx_code).stdout().strip())
]

fs = import ('fs')

element_fx_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_fx_au = shared_module ('KV-Element-FX',
    element_fx_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : false,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes, juce_au_includes ],
    cpp_args : [ element_fx_au_juce_args ],
    objcpp_args : [ element_fx_au_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

element_fx_au_plist = configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV-Element-FX.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
        'BUNDLE_NAME'          : 'KV-Element-FX',
        'BUNDLE_DISPLAY_NAME'  : 'Element FX',
        'BUNDLE_VERSION'       : meson.project_version(),
        'EXPORT_PREFIX'        : 'ElementFX',
        'AU_NAME'              : 'Kushview: Element FX',
        'AU_DESCRIPTION'       : 'Element modular effects router.',
        'AU_INT_VERSION'       : element_plugin_int_version,
        'TYPE'                 : 'aumf',
        'SUBT'                 : element_fx_code,
        'MANU'                 : element_manu_code
    },
    install : false
)

element_bundles += custom_target (
    'KV-Element-FX.component',
    build_by_default: true,
    command: [ python3, bundle_py, '--binary', '@INPUT@',
                '--output', '@OUTPUT@',
                '--plist', element_fx_au_plist,
                '--type', 'bundle' ],
    input: element_fx_au,
    output: 'KV-Element-FX.component',
    install: get_option ('deploy'),
    install_dir: element_au_install_dir
)

endif

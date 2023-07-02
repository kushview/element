if host_machine.system() == 'darwin'

element_au_juce_args = [
    '-DJucePlugin_Build_AU=1', 
    '-DJucePlugin_Version=@0@'.format (meson.project_version()),
    '-DJucePlugin_VersionString="@0@"'.format(meson.project_version()),
    '-DJucePlugin_VersionCode=@0@'.format (element_plugin_hex_verson_str),
    '-DJucePlugin_ManufacturerCode=@0@'.format (element_manu_hex_code_str),
    '-DJucePlugin_PluginCode=@0@'.format (run_command (
        getcode_py, '--code', element_code).stdout().strip())
]

fs = import ('fs')

element_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_au = shared_module ('KV-Element',
    element_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : false,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes, juce_au_includes ],
    cpp_args : [ element_au_juce_args ],
    objcpp_args : [ element_au_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

element_au_plist = configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV-Element.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.Element',
        'BUNDLE_NAME'          : 'KV-Element',
        'BUNDLE_DISPLAY_NAME'  : 'Element',
        'BUNDLE_VERSION'       : meson.project_version(),
        'EXPORT_PREFIX'        : 'ElementAU',
        'AU_NAME'              : 'Kushview: Element',
        'AU_DESCRIPTION'       : 'Element modular instrument.',
        'AU_INT_VERSION'       : element_plugin_int_version,
        'TYPE'                 : 'aumu',
        'SUBT'                 : element_code,
        'MANU'                 : element_manu_code
    },
    install : false
)

element_bundles += custom_target (
    'KV-Element.component',
    build_by_default: true,
    command: [ bundle_py, '--binary', '@INPUT@',
                '--output', '@OUTPUT@',
                '--plist', element_au_plist,
                '--type', 'bundle' ],
    input: element_au,
    output: 'KV-Element.component',
    install: get_option ('deploy'),
    install_dir: element_au_install_dir
)

endif

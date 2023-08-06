if aeffect_h_found
element_vst2_sources = [
    '../plugin.cpp',
    '../../vst2client.cpp',
    '../../utilities.cpp'
]

element_vst2_juce_args = [
    '-DJucePlugin_Build_VST=1', 
    '-DJucePlugin_Version=@0@'.format (meson.project_version()),
    '-DJucePlugin_VersionString="@0@"'.format(meson.project_version()),
    '-DJucePlugin_VersionCode=@0@'.format (run_command (python3,
        getcode_py, '--version', meson.project_version(), check: true).stdout().strip()),
    '-DJucePlugin_ManufacturerCode=@0@'.format (run_command (python3,
        getcode_py, '--code', element_manu_code, check: true).stdout().strip()),
    '-DJucePlugin_PluginCode=@0@'.format (run_command (python3,
        getcode_py, '--code', element_code, check: true).stdout().strip())
]

if host_machine.system() == 'darwin'
    element_vst2_sources += '../../macvstutils.mm'
endif

element_vst2 = shared_module ('KV-Element',
    element_vst2_sources,
    name_prefix : '',
    name_suffix : host_machine.system() == 'darwin' ? 'bundle' : [],
    install : host_machine.system() != 'darwin',
    install_dir : element_vst2_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ element_vst2_juce_args ],
    objcpp_args : [ element_vst2_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])
element_binaries += element_vst2

if host_machine.system() == 'darwin'
    element_vst2_plist = configure_file (
        input: '../../plugin.plist.in',
        output: 'Info.plist',
        configuration : {
            'BUNDLE_EXECUTABLE'    : 'KV-Element.bundle',
            'BUNDLE_ID'            : 'net.kushview.plugins.Element',
            'BUNDLE_NAME'          : 'KV-Element',
            'BUNDLE_DISPLAY_NAME'  : 'Element',
            'BUNDLE_VERSION'       : meson.project_version(),
            'EXPORT_PREFIX'        : 'Element',
            'TYPE'                 : 'aumf',
            'SUBT'                 : element_code,
            'MANU'                 : element_manu_code
        },
        install : false
    )

    element_bundles += custom_target (
        'KV-Element.vst',
        build_by_default: true,
        command: [ python3, bundle_py, '--binary', '@INPUT@',
                    '--output', '@OUTPUT@',
                    '--plist', element_vst2_plist,
                    '--type', 'bundle' ],
        input: element_vst2,
        output: 'KV-Element.vst',
        install: get_option ('deploy'),
        install_dir: element_vst2_install_dir
    )
endif

endif

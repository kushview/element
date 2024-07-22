if aeffect_h_found
element_fx_vst2_sources = [
    '../plugin.cpp',
    '../../utilities.cpp'
]

element_fx_vst2_juce_args = [
    '-DJucePlugin_Build_VST=1', 
    '-DJucePlugin_Version=@0@'.format (meson.project_version()),
    '-DJucePlugin_VersionString="@0@"'.format(meson.project_version()),
    '-DJucePlugin_VersionCode=@0@'.format (run_command (python3, 
        getcode_py, '--version', meson.project_version(), check: true).stdout().strip()),
    '-DJucePlugin_ManufacturerCode=@0@'.format (run_command (python3, 
        getcode_py, '--code', element_manu_code, check: true).stdout().strip()),
    '-DJucePlugin_PluginCode=@0@'.format (run_command (python3, 
        getcode_py, '--code', element_fx_code, check: true).stdout().strip())
]

if host_machine.system() == 'darwin'
    element_fx_vst2_sources += '../../vst2client.mm'
    element_fx_vst2_sources += '../../macvstutils.mm'
else
    element_fx_vst2_sources += '../../vst2client.cpp'
endif

element_fx_vst2 = shared_module ('KV-Element-FX',
    element_fx_vst2_sources,
    name_prefix : '',
    name_suffix : host_machine.system() == 'darwin' ? 'bundle' : [],
    install : host_machine.system() != 'darwin',
    install_dir : element_vst2_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ element_fx_vst2_juce_args ],
    objcpp_args : [ element_fx_vst2_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])
element_binaries += element_fx_vst2

if host_machine.system() == 'darwin'
    element_fx_vst2_plist = configure_file (
        input: '../../plugin.plist.in',
        output: 'Info.plist',
        configuration : {
            'BUNDLE_EXECUTABLE'    : 'KV-Element-FX.bundle',
            'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
            'BUNDLE_NAME'          : 'KV-Element-FX',
            'BUNDLE_DISPLAY_NAME'  : 'Element FX',
            'BUNDLE_VERSION'       : meson.project_version(),
            'EXPORT_PREFIX'        : 'ElementFX',
            'TYPE'                 : 'aumf',
            'SUBT'                 : element_fx_code,
            'MANU'                 : element_manu_code
        },
        install : false
    )

    element_bundles += custom_target (
        'KV-Element-FX.vst',
        build_by_default: true,
        command: [ python3, bundle_py, '--binary', '@INPUT@',
                    '--output', '@OUTPUT@',
                    '--plist', element_fx_vst2_plist,
                    '--type', 'bundle' ],
        input: element_fx_vst2,
        output: 'KV-Element-FX.vst',
        install: get_option ('deploy'),
        install_dir: element_vst2_install_dir
    )
endif

endif


# Element FX
element_fx_vst3_sources = [
    '../plugin.cpp',
    '../../utilities.cpp'
]

element_fx_vst3_juce_args = [
    '-DJucePlugin_Build_VST3=1', 
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
    element_fx_vst3_sources += '../../vst3client.mm'
    element_fx_vst3_sources += '../../macvstutils.mm'
else
    element_fx_vst3_sources += '../../vst3client.cpp'
endif

element_fx_vst3 = shared_module ('KV-Element-FX',
    element_fx_vst3_sources,
    name_prefix : '',
    name_suffix : element_vst3_name_suffix,
    install : false,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ element_fx_vst3_juce_args ],
    objcpp_args : [ element_fx_vst3_juce_args ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

element_fx_vst3_bundle_py =  [ python3, bundle_py, 
    '--binary', '@INPUT@', '--output', '@OUTPUT@',
    '--type', host_machine.system() == 'darwin' ? 'bundle' : 'vst3',
    '--bindir', element_vst3_bindir ]

if host_machine.system() == 'darwin'
    element_fx_vst3_plist = configure_file (
        input: '../../plugin.plist.in',
        output: 'Info.plist',
        configuration : {
            'BUNDLE_EXECUTABLE'    : 'KV-Element-FX.bundle',
            'BUNDLE_ID'            : 'net.kushview.plugins.ElementFX',
            'BUNDLE_NAME'          : 'KV-Element-FX',
            'BUNDLE_DISPLAY_NAME'  : 'Element FX',
            'BUNDLE_VERSION'       : meson.project_version(),
        },
        install : false
    )

    element_fx_vst3_bundle_py += [ '--plist', element_fx_vst3_plist ]
elif host_machine.system() == 'windows' and get_option ('sign')
    element_fx_vst3_bundle_py += [ '--signtool-sha1', get_option ('signtool-sha1') ]
endif

subdir ('bundle')

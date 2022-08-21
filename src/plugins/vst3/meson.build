
if host_machine.system() == 'windows'
    element_vst3_name_suffix = 'vst3'
    element_vst3_bindir = 'x86_64-win'
elif host_machine.system() == 'linux'
    element_vst3_name_suffix = 'so'
    element_vst3_bindir = run_command ('uname', '-m', capture : true, check : true).stdout().strip()
elif host_machine.system() == 'darwin'
    element_vst3_name_suffix = ''
    element_vst3_bindir = 'MacOS'
endif

# Element Instrument
element_vst3_install_dir = 'lib' / 'vst3' / 'KV_Element.vst3' / 'Contents' / element_vst3_bindir
element_vst3 = shared_module ('KV_Element', [
        '../../../tools/jucer/Element/Source/Element.cpp',
        'client.cpp',
        'utilities.cpp'
    ],
    name_prefix : '',
    name_suffix : element_vst3_name_suffix,
    install : true,
    install_dir : element_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST3=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

# Element FX
element_fx_vst3_install_dir = 'lib' / 'vst3' / 'KV_ElementFX.vst3' / 'Contents' / element_vst3_bindir
element_fx_vst3 = shared_module ('KV_ElementFX', [
        '../../../tools/jucer/ElementFX/Source/ElementFX.cpp',
        'client.cpp',
        'utilities.cpp',
    ],
    name_prefix : '',
    name_suffix : element_vst3_name_suffix,
    install : true,
    install_dir : element_fx_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST3=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

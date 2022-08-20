# Element Instrument
element_vst3_install_dir = 'lib' / 'vst3' / 'KV_Element.vst3' / 'Contents' / 'x86_64-linux'
element_vst3 = shared_module ('KV_Element', [
        '../../../tools/jucer/Element/Source/Element.cpp',
        '../../../libs/compat/include_juce_audio_plugin_client_VST3.cpp'
    ],
    name_prefix : '',
    install : true,
    install_dir : element_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

# Element FX
element_fx_vst3_install_dir = 'lib' / 'vst3' / 'KV_ElementFX.vst3' / 'Contents' / 'x86_64-linux'
element_fx_vst3 = shared_module ('KV_ElementFX', [
        '../../../tools/jucer/ElementFX/Source/ElementFX.cpp',
        '../../../libs/compat/include_juce_audio_plugin_client_VST3.cpp'
    ],
    name_prefix : '',
    install : true,
    install_dir : element_fx_vst3_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

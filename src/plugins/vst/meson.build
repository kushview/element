if aeffect_h_found
element_vst2 = shared_module ('KV_Element', [
        '../../../tools/jucer/Element/Source/Element.cpp',
        '../../../libs/compat/include_juce_audio_plugin_client_VST2.cpp'
    ],
    name_prefix : '',
    install : true,
    install_dir : 'lib' / 'vst',
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

element_fx_vst2 = shared_module ('KV_ElementFX', [
        '../../../tools/jucer/ElementFX/Source/ElementFX.cpp',
        '../../../libs/compat/include_juce_audio_plugin_client_VST2.cpp'
    ],
    name_prefix : '',
    install : true,
    install_dir : 'lib' / 'vst',
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

endif

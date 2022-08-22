if aeffect_h_found
element_vst2 = shared_module ('KV_Element', [
        '../plugin.cpp',
        '../../vst2client.cpp'
    ],
    name_prefix : '',
    install : true,
    install_dir : 'lib' / 'vst',
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_VST=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])
endif

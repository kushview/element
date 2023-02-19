###########
### JRC ###
###########
jrc_sources = ['jrc.cpp']
jrc_deps = [ juce_dep ]

jrc = executable ('jrc', jrc_sources,
    install : false,
    win_subsystem : 'console',
    gnu_symbol_visibility : 'hidden',
    dependencies : [ jrc_deps ],
    include_directories : [ element_includes ],
    cpp_args : [],
    objcpp_args : [],
    link_args : [],
    link_with : [ libelement_juce ])

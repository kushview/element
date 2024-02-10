
#############
### bin2c ###
#############
bin2c_sources = [ 'bin2c.cpp' ]
bin2c_deps = [ juce_dep ]

if cpp.get_id() == 'gcc'
    foreach l : 'uuid wsock32 wininet version ole32 ws2_32 oleaut32 imm32 comdlg32 shlwapi rpcrt4 winmm dxgi'.split()
        bin2c_deps += cpp.find_library (l, required : true)
    endforeach
endif

bin2c_install_dir = get_option ('bindir')
bin2c = executable ('element-bin2c', bin2c_sources,
    install : true,
    install_dir : bin2c_install_dir,
    win_subsystem : 'console',
    gnu_symbol_visibility : 'hidden',
    dependencies : [ bin2c_deps ],
    include_directories : [ element_includes ],
    cpp_args : [],
    objcpp_args : [],
    link_args : [],
    link_with : [ libelement_juce ])

if host_machine.system() == 'darwin'

element_au_sources = [
    '../plugin.cpp',
    '../../auwrapper.mm',
    '../../auclient.mm'
]

element_au_install_dir = 'lib' / 'au'

if host_machine.system() == 'darwin'
    element_au_sources += '../../macvstutils.mm'
    element_au_contents_dir = element_au_install_dir / 'KV_Element.component' / 'Contents'
    element_au_install_dir = element_au_contents_dir / 'MacOS'
endif

element_au = shared_module ('KV_Element',
    element_au_sources,
    name_prefix : '',
    name_suffix : 'bundle',
    install : true,
    install_dir : element_au_install_dir,
    gnu_symbol_visibility : 'hidden',
    dependencies : element_app_deps,
    include_directories : [ '..', libelement_includes ],
    cpp_args : [ '-DJucePlugin_Build_AU=1' ],
    objcpp_args : [ '-DJucePlugin_Build_AU=1' ],
    link_args : element_app_link_args,
    link_with : [ libelement ])

configure_file (
    input: '../../audiounit.plist.in',
    output: 'Info.plist',
    configuration : {
        'BUNDLE_EXECUTABLE'    : 'KV_Element.bundle',
        'BUNDLE_ID'            : 'net.kushview.plugins.Element',
        'BUNDLE_NAME'          : 'KV_Element',
        'BUNDLE_DISPLAY_NAME'  : 'Element',
        'BUNDLE_VERSION'       : meson.project_version(),
    },
    install : true,
    install_dir : element_au_contents_dir
)

# custom_target ('KV_Element_rsrc',
#     input : '../../auclient.r',
#     output : 'KV_Element.rsrc',
#     command :  [ '/usr/bin/rez', '-o', '@OUTPUT@',
#         '-d', 'x86_64_YES', 
#         '-d', 'ppc_\$ppc',  '-d', 'i386_\$i386', '-d', 'ppc64_\$ppc64',
#         '-d', 'SystemSevenOrLater\=1',
#         '-useDF', '-script', 'Roman',
#         '-d', '-define', '-d', 'EL_REZ',
#         '-I', '/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers',
#         '-I', '/Applications/Xcode.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUBase' ,
#         '-I', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/AudioUnit.framework/Headers',
#         '-I', meson.global_build_root() / 'libs/compat',
#         '-I', meson.global_source_root() / 'libs/compat',
#         '-I', meson.global_source_root() / 'libs/JUCE/modules/juce_audio_plugin_client',
#         '-I', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/Headers',
#         '-I', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/CoreServices.framework/Versions/A/Headers',
#         '@INPUT@' ],
#     install : true,
#     install_dir : element_au_contents_dir / 'Resources'
# )

# -o /Users/mfisher/Library/Developer/Xcode/DerivedData/Element-ellybuavgzsnkhdwcukaetelutzb/Build/Intermediates.noindex/Element.build/Debug/Element\ -\ AU.build/ResourceManagerResources/Objects/include_juce_audio_plugin_client_AU.rsrc 
# -d SystemSevenOrLater\=1 
# -useDF 
# -script Roman 
# -define -d -define EL_REZ 
# -d ppc_\$ppc -d i386_\$i386 -d ppc64_\$ppc64 -d x86_64_YES 
# -I /System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers 
# -I /Applications/Xcode.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUBase 
# -I /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/AudioUnit.framework/Headers 
# -arch arm64 -arch x86_64 
# -i /Users/mfisher/workspace/kushview/element-0.46/build/Debug 
# -i /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/AudioUnit.framework/Headers 
# -i /Users/mfisher/workspace/kushview/element-0.46/build/Debug 
# -i /Users/mfisher/workspace/kushview/element-0.46/build/Debug/include 
# -i /Users/mfisher/workspace/kushview/element-0.46/tools/jucer/Element/Builds/MacOSX/../../../../../libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK 
# -

endif

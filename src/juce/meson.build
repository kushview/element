


# JUCE
juce_modules = '''
    juce_audio_basics 
    juce_audio_devices 
    juce_audio_formats
    juce_audio_processors 
    juce_audio_utils 
    juce_core 
    juce_cryptography
    juce_data_structures 
    juce_dsp 
    juce_events 
    juce_graphics 
    juce_gui_basics
    juce_gui_extra 
    juce_osc
'''.split()

juce_conf_data = configuration_data()

juce_ext = host_machine.system() == 'darwin' ? 'mm' : 'cpp'
juce_sources = []
juce_includes = include_directories ('.')

foreach m : juce_modules
    pat = 'include_@0@'.format (m)
    pat = pat + '.@0@'
    juce_file = pat.format (juce_ext)
    if not fs.exists (juce_file)
        juce_file = pat.format ('cpp')
    endif
    juce_sources += juce_file
    juce_conf_data.set10 ('JUCE_MODULE_AVAILABLE_@0@'.format (m), true)
endforeach

if get_option ('debug') or get_option('buildtype') == 'debug'
    juce_conf_data.set10 ('_DEBUG', true)
else
    juce_conf_data.set10 ('NDEBUG', true)
endif

juce_conf_data.set10 ('JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED', true)
juce_conf_data.set10 ('JUCE_DISPLAY_SPLASH_SCREEN', false)
juce_conf_data.set10 ('JUCE_MODAL_LOOPS_PERMITTED', true)
juce_conf_data.set10 ('JUCE_DISABLE_JUCE_VERSION_PRINTING', true)
juce_conf_data.set10 ('JUCE_WEB_BROWSER', false)
juce_conf_data.set10 ('JUCE_USE_LAME_AUDIO_FORMAT', false)
juce_conf_data.set10 ('JUCE_USE_MP3AUDIOFORMAT', true)

juce_asio_includes = []
juce_conf_data.set10 ('JUCE_ASIO', false)
if host_machine.system() == 'windows'
    base = get_option ('asiosdk')
    if base == 'auto'
        # noop
    else
        base += '/@0@'
        foreach d : 'common host host\\pc'.split()
            juce_asio_includes += include_directories (base.format (d))
        endforeach
        foreach h : 'asiosys.h'.split()
            cpp.check_header (h,
                include_directories : juce_asio_includes,
                required : true
            )
        endforeach
        juce_conf_data.set10 ('JUCE_ASIO', true)
    endif
endif

juce_conf_data.set10 ('KV_DOCKING_WINDOWS', true)

juce_vst2_includes = []
if get_option ('vst2sdk') == 'auto'
    # noop
else
    juce_vst2_includes += include_directories (get_option ('vst2sdk'))
endif

aeffect_h_found = cpp.check_header ('pluginterfaces/vst2.x/aeffect.h',
    include_directories : juce_vst2_includes, required : false)
aeffectx_h_found = cpp.check_header ('pluginterfaces/vst2.x/aeffectx.h',
    include_directories : juce_vst2_includes, required : false)
vstfxstore_h_found = cpp.check_header ('pluginterfaces/vst2.x/vstfxstore.h',
    include_directories : juce_vst2_includes, required : false)
juce_conf_data.set10 ('JUCE_PLUGINHOST_VST', aeffect_h_found and aeffectx_h_found)

juce_vst3_includes = []
if get_option('vst3sdk') == 'auto'
    juce_vst3_includes += juce_proj.get_variable ('juce_vst3_includes')
else
    juce_vst3_includes += include_directories (get_option ('vst3sdk'))
endif

juce_conf_data.set10 ('JUCE_VST3_CAN_REPLACE_VST2', vstfxstore_h_found)

have_vst3 = true
if host_machine.system() == 'windows' and cpp.get_id() == 'gcc' and cpp.get_argument_syntax() == 'gcc'
    # compiler errors need worked out inside juce for mingw32 cross
    have_vst3 = false
endif

juce_conf_data.set10 ('JUCE_PLUGINHOST_VST3', have_vst3)

juce_lv2_includes = []
# include_directories (
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lv2',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/serd',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sord',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sord/src',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lilv',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lilv/src',
#     '../JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sratom')
# juce_sources += 'include_juce_audio_processors_lv2_libs.cpp'
juce_conf_data.set10 ('JUCE_PLUGINHOST_LV2', false)
juce_conf_data.set10 ('JUCE_PLUGINHOST_LADSPA', cpp.check_header ('ladspa.h'))

### AudioUnit
juce_au_includes = []

if host_machine.system() == 'linux'
    alsa_dep = dependency ('alsa', required : false)
    juce_conf_data.set10 ('JUCE_ALSA', alsa_dep.found())
    deps += alsa_dep

    # juce lazy loads x11
    x11_deps = [ dependency ('x11', required : true),
        dependency ('xext', required : true),
        dependency ('xrandr', required : true),
        dependency ('xcomposite', required : true),
        dependency ('xinerama', required : true),
        dependency ('xcursor', required : true) ]

    deps += [ 
        dependency ('freetype2', required : true),
    ]

    juce_conf_data.set10 ('JUCE_USE_XRANDR', true)
    juce_conf_data.set10 ('JUCE_USE_XINERAMA', true)
    juce_conf_data.set10 ('JUCE_USE_XSHM', true)
    juce_conf_data.set10 ('JUCE_USE_XRENDER', true)
    juce_conf_data.set10 ('JUCE_USE_XCURSOR', true)
    
    curl_dep = dependency ('libcurl', required : false)
    juce_conf_data.set10 ('JUCE_USE_CURL', curl_dep.found())
    deps += curl_dep

    deps += [
        cpp.find_library ('dl', required : true),
        cpp.find_library ('pthread', required : true) 
    ]
elif host_machine.system() == 'darwin'
    deps += dependency ('appleframeworks', modules : [
        'accelerate', 'audiotoolbox', 'audiounit', 'coreaudio',
        'coreaudiokit', 'coremidi', 'cocoa', 'carbon',
        'discrecording', 'iokit', 'quartzcore', 'metal' ])
    juce_conf_data.set10 ('JUCE_PLUGINHOST_AU', true)
    juce_au_includes += juce_proj.get_variable ('juce_au_includes')
elif host_machine.system() == 'windows'
   juce_conf_data.set10 ('JUCE_USE_DIRECTWRITE', true)
endif

configure_file (output : 'juce_config.h', 
         configuration : juce_conf_data)

libelement_juce = static_library ('element_juce', 
    juce_sources,
    include_directories : [ 
        juce_includes,
        juce_asio_includes, 
        juce_vst3_includes, 
        juce_vst2_includes, 
        juce_lv2_includes,
        juce_au_includes
    ],
    gnu_symbol_visibility : 'hidden',
    dependencies : [ deps, juce_dep ],
    install : false)

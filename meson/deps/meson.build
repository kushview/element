
# Boost
boost_dep = dependency ('boost', version: '>= 1.76.0', required : true)

# LV2
if get_option('lv2static') or get_option ('deploy') or host_machine.system() == 'darwin' or host_machine.system() == 'windows'
    lv2_proj = subproject ('lv2', default_options: [
        'default_library=static',
        'docs=disabled',
        'tests=disabled',
        'plugins=disabled' ])
    lv2_dep = lv2_proj.get_variable ('lv2_dep')

    zix_proj = subproject ('zix', default_options: [
        'default_library=static',
        'benchmarks=disabled',
        'tests=disabled',
        'docs=disabled' ])
    zix_dep = zix_proj.get_variable ('zix_dep')

    serd_proj = subproject ('serd', default_options: [
        'default_library=static',
        'tools=disabled',
        'tests=disabled',
        'docs=disabled'])
    serd_dep = serd_proj.get_variable ('serd_dep')

    sord_proj = subproject ('sord', default_options: [
        'default_library=static',
        'tools=disabled',
        'tests=disabled',
        'docs=disabled'])
    sord_dep = sord_proj.get_variable ('sord_dep')
    
    sratom_proj = subproject ('sratom', default_options: [
        'default_library=static',
        'tests=disabled',
        'docs=disabled' ])
    sratom_dep = sratom_proj.get_variable ('sratom_dep')

    lilv_default_path = '~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2'
    if host_machine.system() == 'darwin'
        lilv_default_path = '~/.lv2:~/Library/Audio/Plug-Ins/LV2:/Library/Audio/Plug-Ins/LV2'
    elif host_machine.system() == 'windows'
        lilv_default_path = ';'.join(['%%APPDATA%%\\\\LV2', '%%COMMONPROGRAMFILES%%\\\\LV2'])
    endif
    lilv_proj = subproject ('lilv', default_options: [
        'default_library=static',
        'tools=disabled',
        'tests=disabled',
        'docs=disabled',
        'default_lv2_path=@0@'.format(lilv_default_path),
        'c_args=@0@'.format(host_machine.system() == 'windows' ? '/wd4090' : '') ])
    lilv_dep = lilv_proj.get_variable ('lilv_dep')

    suil_proj = subproject ('suil', default_options: [
        'default_library=static',
        'tests=disabled',
        'docs=disabled',
        'cocoa=disabled',
        'gtk2=disabled',
        'gtk3=disabled',
        'qt5=disabled',
        'qt6=disabled',
        'x11=disabled'
    ])
    suil_dep = suil_proj.get_variable ('suil_dep')
else
    lv2_dep = dependency ('lv2', version: '>= 1.18.2' , required: true, fallback: [])
    lilv_dep = dependency ('lilv-0', required: true, fallback: [])
    suil_dep = dependency ('suil-0', required: true, fallback: [])
endif

# LVTK
lvtk_proj = subproject ('lvtk', default_options: [ 
    'nobuild=true',
    'demo=disabled',
    'bindings=disabled',
    'test=disabled',
    'doc=disabled',
    'plugins=disabled'
])
lvtk_host_dep = lvtk_proj.get_variable ('lvtk_dep')
add_project_arguments (['-DLVTK_STATIC=1', '-DPUGL_STATIC=1'], 
    language: ['c', 'cpp', 'objc', 'objcpp'])

# JACK
jack_enabled = false
jack_includes = include_directories()

if not get_option('jack').disabled()
    jack_enabled = cpp.check_header ('jack/jack.h', 
        required : get_option('jack').enabled(),
        include_directories : jack_includes)
endif
if jack_enabled
    add_project_arguments ('-DEL_USE_JACK=1', 
        language : [ 'c', 'objc', 'cpp', 'objcpp' ])
endif

# Sol3
if cpp.get_argument_syntax() == 'gcc'
    # silence warnings from sol3
    add_project_arguments (['-Wno-array-bounds'], language : ['cpp'])
endif

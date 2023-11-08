
# Boost
boost_dep = dependency ('boost', required : true)

# LV2
if get_option ('deploy')
    lv2_proj = subproject ('lv2', default_options: [
        'default_library=static',
        'docs=disabled',
        'tests=disabled',
        'plugins=disabled' ])
    lv2_dep = lv2_proj.get_variable ('lv2_dep')

    zix_proj = subproject ('zix', default_options: [
        'default_library=static' ])
    zix_dep = zix_proj.get_variable ('zix_dep')

    serd_proj = subproject ('serd', default_options: [
        'default_library=static' ])
    serd_dep = serd_proj.get_variable ('serd_dep')

    sord_proj = subproject ('sord', default_options: [
        'default_library=static' ])
    sord_dep = sord_proj.get_variable ('sord_dep')
    
    sratom_proj = subproject ('sratom', default_options: [
        'default_library=static' ])
    sratom_dep = sratom_proj.get_variable ('sratom_dep')

    lilv_proj = subproject ('lilv', default_options: [
        'default_library=static' ])
    lilv_dep = lilv_proj.get_variable ('lilv_dep')

    suil_proj = subproject ('suil', default_options: [
        'default_library=static',
        'tests=disabled',
        'docs=disabled',
        'cocoa=disabled',
        'gtk2=disabled',
        'gtk3=disabled',
        'qt5=disabled',
        'x11=disabled'
    ])
    suil_dep = suil_proj.get_variable ('suil_dep')
else
    lv2_dep = dependency ('lv2', version: '>= 1.18.2' , required : false)
    if not lv2_dep.found()
        lv2_proj = subproject ('lv2',
            default_options: [])
        lv2_dep = lv2_proj.get_variable ('lv2_dep')
    endif

    lilv_dep = dependency ('lilv-0', required : false)
    if not lilv_dep.found()
        lilv_proj = subproject ('lilv',
            default_options: [])
        lilv_dep = lilv_proj.get_variable ('lilv_dep')
    endif

    suil_dep = dependency ('suil-0', required : false)
    if not suil_dep.found()
        suil_proj = subproject ('suil',
            default_options: [
                'default_library=static',
                'tests=disabled',
                'docs=disabled',
                'cocoa=disabled',
                'gtk2=disabled',
                'gtk3=disabled',
                'qt5=disabled',
                'x11=disabled'
            ])
        suil_dep = suil_proj.get_variable ('suil_dep')
    endif
endif

# LVTK
lvtk_proj = subproject ('lvtk', default_options: [
    'default_library=static'
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

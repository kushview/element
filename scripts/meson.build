
scripts_gen = generator (bin2c,
    output : [ '@PLAINNAME@.cpp', '@PLAINNAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'scripts' ])
scripts_includes = include_directories ('.', '../scripts')
scripts_sources = scripts_gen.process (
    'ampui.lua', 'channelize.lua', 'amp.lua', 'console.lua')

if not meson.is_subproject()

scripts_install_dir = get_option('datadir') / 'element/scripts'

if get_option ('deploy')
    if host_machine.system() == 'windows'
        scripts_install_dir = packagedir_element + '.lua' / 'data' / 'lua' / 'scripts'
    elif host_machine.system() == 'darwin'
        scripts_install_dir = packagedir_element + '.lua' / 'data' / 'lua' / 'scripts'
    endif
endif

install_data (sources : [
    'amp.lua',
    'ampui.lua',
    'channelize.lua',
    'console.lua',
    'view.lua'
    ],
    install_dir : scripts_install_dir
)

if get_option ('deploy')
    foreach f : 'package.xml component.js'.split()
        configure_file (input: f, output: f,
            install: true, install_dir: packagedir_element + '.lua' / 'meta',
            configuration: { 'VERSION': version_with_build, 
                            'RELEASEDATE': element_release_date})
    endforeach
endif

endif

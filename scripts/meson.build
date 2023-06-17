
if not meson.is_subproject()

scripts_install_dir = get_option('datadir') / 'element/scripts'

if host_machine.system() == 'windows'
    scripts_install_dir = packagedir_element + '.lua' / 'data' / 'scripts'
elif host_machine.system() == 'darwin'
    scripts_install_dir = 'Element.app/Contents/Resources/scripts'
endif

if not get_option ('deploy')
    install_data (
        sources : [
            'amp.lua',
            'ampui.lua',
            'channelize.lua',
            'console.lua'
        ],
        install_dir : scripts_install_dir
    )
endif

scripts_gen = generator (jrc,
    output : [ '@PLAINNAME@.cpp', '@PLAINNAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'scripts' ])
scripts_includes = include_directories ('.', '../scripts')
scripts_sources = scripts_gen.process (
    'ampui.lua', 'channelize.lua', 'amp.lua', 'console.lua')

endif

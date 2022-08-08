
if not meson.is_subproject()

scripts_install_dir = get_option('datadir') / 'element/scripts'
if host_machine.system() == 'windows'
    scripts_install_dir = get_option('bindir') / 'scripts'
elif host_machine.system() == 'darwin'
    scripts_install_dir = 'Contents/Resources/Scripts'
endif

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
    output : [ '@BASENAME@.cpp', '@BASENAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'scripts' ])
scripts_includes = include_directories ('.', '../scripts')
scripts_sources = scripts_gen.process (
    'ampui.lua', 'amp.lua', 'console.lua')

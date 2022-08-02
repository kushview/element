
scripts_install_dir = get_option('datadir') / 'element/scripts'
if host_machine.system() == 'windows'
    scripts_install_dir = get_option('bindir') / 'scripts'
endif

install_data (
    sources : [
        'amp.lua',
        'ampui.lua',
        'channelize.lua',
        'console.lua'
    ], 
    install_dir :scripts_install_dir
)

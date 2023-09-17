install_data ('component.js', install_dir : packagedir_element / 'meta')
install_data ('updatekey.ui', install_dir : packagedir_element / 'meta')

element_operations = ''
if host_machine.system() == 'windows'
    element_operations = ''
elif host_machine.system() == 'darwin'
    element_operations = ''
endif

configure_file (input : 'package.xml',
    install: true, install_dir: packagedir_element / 'meta',
    output: 'package.xml', configuration: {
        'VERSION': version_with_build,
        'RELEASEDATE': element_release_date,
        'OPERATIONS': element_operations
    })

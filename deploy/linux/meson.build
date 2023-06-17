custom_target ('desktop icon',
        command : [ 'cp', '-f', '@INPUT@', '@OUTPUT@' ],
        input   : 'icon_256x256.png',
        output  : 'net.kushview.element.png',
        install : true,
        install_dir : get_option('datadir') / 'icons/hicolor/256x256/apps')
configure_file (
    input: 'element.desktop.in',
    output: 'net.kushview.element.desktop',
    configuration : {
        'ELEMENT_ICON' : 'net.kushview.element',
        'ELEMENT_EXE'  : 'element'
    },
    install : true,
    install_dir : get_option('datadir') / 'applications')

install_data ('component.qs', install_dir : packagedir_root / 'meta')
configure_file (input : 'package.xml',
    install: true, install_dir: packagedir_root / 'meta',
    output: 'package.xml', configuration: {
        'VERSION': version_with_build,
        'RELEASEDATE': element_release_date
    })

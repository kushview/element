packgedir_updater = packagesdir / net_kushview + '.element.updater'
install_data ('component.js', install_dir : packgedir_updater / 'meta')

configure_file (input : 'package.xml',
    install: true, install_dir: packgedir_updater / 'meta',
    output: 'package.xml', configuration: {
        'VERSION': version_with_build,
        'RELEASEDATE': element_release_date,
        'TargetDir': '@TargetDir@'
    })

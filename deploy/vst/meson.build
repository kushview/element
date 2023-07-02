foreach f : 'package.xml component.js'.split()
    configure_file (input :f,
        install: true, install_dir: packagedir_element + '.vst' / 'meta',
        output: f, configuration: {
            'VERSION': version_with_build,
            'RELEASEDATE': element_release_date,
        })
endforeach

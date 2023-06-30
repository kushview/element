foreach f : 'package.xml LICENSE component.js'.split()
    configure_file (input : f,
        install: true, install_dir: packagedir_element + '.lv2' / 'meta',
        output: f, configuration: {
            'RELEASEDATE': element_release_date,
        })
endforeach

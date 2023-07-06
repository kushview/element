ldoc = find_program ('ldoc', version : '>= 1.5.0', required: false)
ldoc_install_dir = get_option ('datadir') / 'doc' / 'element-0' / 'lua'

if get_option ('deploy')
    packagedir_element_doc = packagedir_element + '.doc'
    ldoc_install_dir = packagedir_element_doc / 'data' / 'doc'
    foreach f : 'package.xml component.js'.split()
        configure_file (input : f, output: f,
            install: true, install_dir: packagedir_element_doc / 'meta',
             configuration: {
                'VERSION': version_with_build,
                'RELEASEDATE': element_release_date
            })
    endforeach
endif

if ldoc.found() and not meson.is_subproject()
    custom_target (
        'Lua API Docs',
        build_by_default: true,
        command: [ ldoc, '-f', 'markdown', '-q',
            '-c', '@INPUT@', 
            '-d', meson.current_build_dir() / '..' / '@OUTPUT@',
            meson.project_source_root()],
        input: files('config.ld'),
        output: 'lua',
        install: true,
        install_dir: ldoc_install_dir
    )
endif

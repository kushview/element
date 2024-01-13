if not get_option('ldoc').disabled()
    ldoc = find_program ('ldoc', version : '>= 1.5.0', required: false)
    ldoc_install_dir = get_option ('datadir') / 'doc' / 'element-0' / 'lua'

    if ldoc.found() and not meson.is_subproject()
        custom_target (
            'Lua API Docs',
            build_always_stale: true,
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
endif

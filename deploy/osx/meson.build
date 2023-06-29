
bundle_sh = find_program ('bundle.sh')

Element_bundle = custom_target (
    'Element.app',
    build_by_default: true,
    command: [ bundle_sh ],
    input: element_app,
    output: 'Element.app',
    install: true,
    install_dir: packagedir_element / 'data'
)

if get_option('deploy')
    macdeployqt = find_program ('macdeployqt', required : true)
    command = [ macdeployqt, '@INPUT0@', '-no-plugins', '-dmg',
        '-executable=@INPUT1@', '-always-overwrite', '-verbose=3' ]

    if get_option ('sign')
        ident = get_option ('codesign-identity')
        command += [ '-codesign='+ident, '-hardened-runtime', 
                        '-sign-for-notarization='+ident, '-timestamp' ]
    endif

    Element_dmg = custom_target (
        'Element.dmg',
        build_by_default: true,
        command: command,
        input: [ Element_bundle, element_app ],
        output: 'Element.dmg',
        install: true,
        install_dir: get_option ('prefix') / 'dmgs'
    )

    if get_option ('notarize') and get_option ('sign')
        custom_target (
            'Element.dmg (notarized)',
            input : Element_dmg,
            output : 'Element_notarized',
            command : [ 'xcrun', 'altool', '--notarize-app',
                            '--primary-bundle-id', 'net.kushview.Element',
                            '-u', get_option ('apple-id'),
                            '-p', get_option ('app-password'),
                            '--file', '@INPUT@'
                    ]
        )
    endif
endif

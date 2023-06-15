
packages = [ 'root' ]

if host_machine.system() == 'linux'
elif host_machine.system() == 'darwin'
    macdeployqt = find_program ('macdeployqt', required : false)
    packages += [ 'osx' ]
elif host_machine.system() == 'windows'
endif

foreach f : [ 'config.xml', 'control.qs' ]
    configure_file (input : f , output : f,
        install : true, 
        install_dir : installerdir / 'config',
        configuration: {
            'VERSION': meson.project_version(),
            'TARGETDIR': '/Library/Application Support/Kushview/Element'
        })
endforeach

foreach pkg : packages
    subdir(pkg)
endforeach

if host_machine.system() == 'darwin'
    basename = 'element-osx-@0@'.format (version_with_build)

    extra_args = ''
    if get_option ('sign')
        ident = get_option ('codesign-identity')
        extra_args = '-codesign="@0@" -hardened-runtime -sign-for-notarization="@0@" -timestamp'.format(ident)
    endif

    notary_script = ''
    if get_option('notarize') and get_option('sign')
        notary_script = '''xcrun altool --notarize-app --primary-bundle-id="net.kushview.element.osx"  \
            -u @0@ -p @1@ --file @2@'''.format(
                get_option('apple-id'), 
                get_option ('app-password'),
                basename + '.dmg')
    endif

    archivegen = find_program ('archivegen', required : true)
    binarycreator = find_program ('binarycreator', required : true)
    configure_file (input : 'installer.sh', output : 'installer.sh',
        install : true, install_dir : installerdir,
        configuration: {
            'BINARYCREATOR': binarycreator.full_path(),
            'CONFIGFILE': 'config/config.xml',
            'PACKAGES': 'packages',
            'INSTALLERBASE': basename,
            'MACDEPLOYQT': macdeployqt.full_path(),
            'MACDEPLOYQT_ARGS': extra_args,
            'NOTARYSCRIPT': notary_script,
            'ARCHIVEGEN': archivegen.full_path()
        })
elif host_machine.system() == 'win32'
    output = 'element-win64-@0@'.format (version_with_build)
endif

summary ('Version', version_with_build, section : 'Application')
# summary ('Hash', version_with_build, section : 'Version')

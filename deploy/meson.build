
archivegen = find_program ('archivegen', required : get_option ('deploy'))
binarycreator = find_program ('binarycreator', required : get_option ('deploy'))
macdeployqt = find_program ('macdeployqt', 
    required : host_machine.system() == 'darwin' and get_option ('deploy'))

element_deploy_targetdir = ''
element_release_url = ''
element_repo_url = 'https://cd.kushview.net/element'
element_runprogram = '/usr/bin/element'

if host_machine.system() == 'darwin'
    element_release_url = element_repo_url + '/release/osx'
    element_deploy_targetdir = '@HomeDir@/Library/Application Support/Kushview/Element/installer'
    element_runprogram = '@ApplicationsDir@/Element.app/Contents/MacOS/Element'
elif host_machine.system() == 'windows'
    element_release_url = element_repo_url + '/release/windows'
    element_deploy_targetdir = '@HomeDir@/AppData/Roaming/Kushview/Element/installer'
    element_runprogram = '@ApplicationsDirX64@/Kushview/Element/bin/element.exe'
endif

if get_option ('deploy')
if not meson.is_subproject()

foreach f : [ 'config.xml', 'control.js' ]
    configure_file (input : f , output : f,
        install : true, 
        install_dir : installerdir / 'config',
        configuration: {
            'VERSION': meson.project_version(),
            'TARGETDIR': element_deploy_targetdir,
            'RELEASEREPO': element_release_url,
            'RUNPROGRAM': element_runprogram
        })
endforeach

endif # not subproject

element_packages = 'element lv2'.split()
if not get_option ('element-plugins').disabled()
    element_packages += 'vst vst3'.split()
    if host_machine.system() == 'darwin'
        element_packages += ['au']
    endif
endif

foreach pkg : element_packages
    subdir (pkg)
endforeach

if not meson.is_subproject()

if host_machine.system() == 'darwin'
    basename = 'element-osx-@0@'.format (version_with_build)

    extra_args = ''
    if get_option ('sign')
        ident = get_option ('codesign-identity')
        extra_args = '-codesign="@0@" -hardened-runtime -sign-for-notarization="@0@" -timestamp'.format(ident)
    endif

    notary_script = ''
    if get_option('notarize') and get_option('sign')
        notary_script = '''xcrun altool --notarize-app --primary-bundle-id="net.kushview.element.installer"  \
            -u @0@ -p @1@ --file @2@'''.format(
                get_option('apple-id'), 
                get_option ('app-password'),
                basename + '.dmg')
    endif

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
elif host_machine.system() == 'windows'
    basename = 'element-windows-@0@'.format (version_with_build)

    extra_args = ''
    if get_option ('sign')
       # sign exe
    endif

    configure_file (input : 'installer.bat', output : 'installer.bat',
        install : true, install_dir : installerdir,
        configuration: {
            'BINARYCREATOR': binarycreator.full_path(),
            'CONFIGFILE': 'config\config.xml',
            'PACKAGES': 'packages',
            'INSTALLERBASE': basename,
            'ARCHIVEGEN': archivegen.full_path()
        })
endif

endif # not subproject
endif # is deploy

if not meson.is_subproject()
    if host_machine.system() == 'darwin'
        subdir ('osx')
    elif host_machine.system() == 'windows'
        # subdir ('windows')
    elif host_machine.system() == 'linux'
        subdir ('linux')
    endif
endif

summary ('Version', version_with_build, section : 'Application')
summary ('Deploy', get_option('deploy'), section : 'Application')

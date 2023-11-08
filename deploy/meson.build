
archivegen = find_program ('archivegen', required : get_option ('deploy'))
binarycreator = find_program ('binarycreator', required : get_option ('deploy'))
macdeployqt = find_program ('macdeployqt', 
    required : host_machine.system() == 'darwin' and get_option ('deploy'))

element_deploy_targetdir = ''
element_release_url = ''
element_repo_url = 'https://repo.kushview.net/p/element/0'
# element_repo_url = 'http://localhost:7777/element/v0'
element_runprogram = '/usr/bin/element'

if host_machine.system() == 'darwin'
    element_release_url = element_repo_url + '/stable/osx'
    element_deploy_targetdir = '@HomeDir@/Library/Application Support/Kushview/Element/installer'
    element_runprogram = '@ApplicationsDir@/Element.app/Contents/MacOS/Element'
elif host_machine.system() == 'windows'
    element_release_url = element_repo_url + '/stable/windows'
    element_deploy_targetdir = '@HomeDir@/AppData/Roaming/Kushview/Element/installer'
    element_runprogram = '@ApplicationsDirX64@/Kushview/Element/bin/element.exe'
elif host_machine.system() == 'linux'
    element_release_url = element_repo_url + '/stable/linux'
    element_deploy_targetdir = '@HomeDir@/.config/Kushview/Element/installer'
    element_runprogram = '@HomeDir@/Applications/Element-@0@-x86_64.AppImage'.format(version_with_build)
endif

if get_option ('deploy')
if not meson.is_subproject()

install_data (app_logo_png, install_dir: installerdir / 'config', rename: 'logo.png')
install_data (app_icon, install_dir: installerdir / 'config')

foreach f : [ 'config.xml', 'control.js' ]
    configure_file (input : f , output : f,
        install : true, 
        install_dir : installerdir / 'config',
        configuration: {
            'VERSION': meson.project_version(),
            'TARGETDIR': element_deploy_targetdir,
            'RELEASEREPO': element_release_url,
            'RUNPROGRAM': element_runprogram,
            'APPICON': fs.stem ('@0@'.format (app_icon.get (0)))
        })
endforeach

endif # not subproject

element_packages = 'element'.split()
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
    basename = 'element-win64-@0@'.format (version_with_build)

    signtool_script = ''
    if get_option ('sign')
        signtool_script='''
SignTool  sign /as /v /sha1 @0@ /fd SHA256 /tr http://sha256timestamp.ws.symantec.com/sha256/timestamp @1@
'''.format (get_option('signtool-sha1'), basename + '.exe')
    endif

    configure_file (input : 'installer.bat', output : 'installer.bat',
        install : true, install_dir : installerdir,
        configuration: {
            'BINARYCREATOR': binarycreator.full_path(),
            'CONFIGFILE': 'config\config.xml',
            'PACKAGES': 'packages',
            'INSTALLERBASE': basename,
            'ARCHIVEGEN': archivegen.full_path(),
            'SIGNTOOL': signtool_script
        })
elif host_machine.system() == 'linux'
    basename = 'element-linux64-@0@'.format (version_with_build)
    configure_file (input : 'installer-linux.sh', output : 'installer.sh',
        install : true, install_dir : installerdir,
        configuration: {
            'BINARYCREATOR': binarycreator.full_path(),
            'CONFIGFILE': 'config' / 'config.xml',
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
summary ('Deploy', get_option('deploy'), section : 'Application', bool_yn: true)

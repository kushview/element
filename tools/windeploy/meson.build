fs = import ('fs')
uninstall_exe = 'uninstall.exe'
# this ensures windows style slashes are used for NSIS
uninstaller_to_write = meson.global_build_root() + '\@0@'.format (uninstall_exe)

installer_outfile = run_command (
    '../version.py', '--revision', '--before=element-w64-', '--after=.exe',
    '--cwd=@0@'.format (meson.global_source_root()), capture : true, check : true).stdout().strip()
installer_outfile = meson.global_build_root() + '\@0@'.format (installer_outfile)

configure_file (
    input: 'setup.nsi.in',
    output: 'setup.nsi',
    configuration : {
        'ELEMENT_VERSION' : meson.project_version(),
        'BUILD_ROOT'      : meson.global_build_root(),
        'SOURCE_ROOT'     : meson.global_source_root(),
        'OUTFILE'         : installer_outfile,
        'UNINSTALLER'     : uninstall_exe,
        'UNINSTALLER_TO_WRITE' : uninstaller_to_write
    },
    install : false
)

configure_file (
    input: 'uninstaller.bat.in',
    output: 'uninstaller.bat',
    configuration : {
        'BUILD_ROOT'      : meson.global_build_root()
    },
    install : false
)

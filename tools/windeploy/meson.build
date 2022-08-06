uninstall_exe = 'uninstall.exe'
uninstaller_outfile_exe = 'element-setup.exe'

# this ensures windows style slashes are used for NSIS
uninstaller_to_write = meson.global_build_root() + '\@0@'.format (uninstall_exe)
uninstaller_outfile  = meson.global_build_root() + '\@0@'.format (uninstaller_outfile_exe)

configure_file (
    input: 'setup.nsi.in',
    output: 'setup.nsi',
    configuration : {
        'ELEMENT_VERSION' : meson.project_version(),
        'BUILD_ROOT'      : meson.global_build_root(),
        'SOURCE_ROOT'     : meson.global_source_root(),
        'OUTFILE'         : uninstaller_outfile,
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

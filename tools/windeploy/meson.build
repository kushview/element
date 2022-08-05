uninstall_exe = 'uninstall.exe'
# this ensures windows style slashes are used for NSIS
uninstaller_to_write = meson.global_build_root() + '\@0@'.format (uninstall_exe)

configure_file (
    input: 'setup.nsi.in',
    output: 'setup.nsi',
    configuration : {
        'ELEMENT_VERSION' : meson.project_version(),
        'BUILD_ROOT'      : meson.global_build_root(),
        'SOURCE_ROOT'     : meson.global_source_root(),
        'UNINSTALLER'     : uninstall_exe,
        'UNINSTALLER_TO_WRITE' : uninstaller_to_write
    },
    install : false
)

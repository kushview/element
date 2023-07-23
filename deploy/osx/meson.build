
element_bundles += custom_target (
    'Element.app',
    build_by_default: true,
    command: [ python3, bundle_py, '--binary', '@INPUT@',
                '--output', '@OUTPUT@',
                '--plist', files ('Info.plist'),
                '--resource', files ('Icon.icns') ],
    input: element_app,
    output: 'Element.app',
    install: get_option ('deploy'),
    install_dir: packagedir_element / 'data'
)

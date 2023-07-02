element_bundles += custom_target (
    'KV-Element.vst3',
    build_by_default: true,
    command: element_vst3_bundle_py,
    input: element_vst3,
    output: 'KV-Element.vst3',
    install: get_option ('deploy'),
    install_dir: element_vst3_install_dir
)

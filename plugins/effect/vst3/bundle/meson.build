element_bundles += custom_target (
    'KV-Element-FX.vst3',
    build_by_default: true,
    command: element_fx_vst3_bundle_py,
    input: element_fx_vst3,
    output: 'KV-Element-FX.vst3',
    install: get_option ('deploy'),
    install_dir: element_vst3_install_dir
)


version_data = configuration_data()
version_data.set_quoted ('EL_VERSION_STRING', meson.project_version())
version_data.set ('EL_BUILD_NUMBER', build_number.to_int())
version_data.set_quoted ('EL_GIT_HASH', git_hash)
version_data.set_quoted ('EL_GIT_SHORT_HASH', git_short_hash)
configure_file (output: 'version.h',
    configuration: version_data,
    install: true,
    install_dir: element_include_dir )

install_headers ('element.h', subdir: element_include_subdir)
install_headers ('node.h', subdir: element_include_subdir) 

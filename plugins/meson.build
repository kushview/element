
if get_option('element-plugins').enabled()

getcode_py = find_program ('../util/getcode.py')

element_plugin_hex_verson_str = run_command (
    getcode_py, '--version', meson.project_version()).stdout().strip()
element_plugin_int_version = run_command (
    getcode_py, '--version', meson.project_version(), '--integer').stdout().strip().to_int()

element_manu_code = 'KshV'
element_manu_hex_code_str = run_command (
    getcode_py, '--code', element_manu_code).stdout().strip()

element_code = 'Elmt'
element_fx_code = 'ElFX'
element_mfx_code = 'ElMX'

element_au_install_dir   = (packagedir_element + '.au') / 'data' 
element_vst2_install_dir = (packagedir_element + '.vst') / 'data' 
element_vst3_install_dir = (packagedir_element + '.vst3') / 'data' 

if host_machine.system() == 'windows'
    element_vst3_name_suffix = 'vst3'
    element_vst3_bindir = 'x86_64-win'
elif host_machine.system() == 'linux'
    element_vst3_name_suffix = 'so'
    element_vst3_bindir = run_command ('uname', '-m', capture : true, check : true).stdout().strip()
    element_vst3_bindir += '-linux'
elif host_machine.system() == 'darwin'
    element_vst3_name_suffix = 'bundle'
    element_vst3_bindir = 'MacOS'
endif

subdir ('instrument/au')
subdir ('instrument/vst2')
subdir ('instrument/vst3')

subdir ('effect/au')
subdir ('effect/vst2')
subdir ('effect/vst3')

subdir ('midieffect/au')

endif

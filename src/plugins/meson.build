
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

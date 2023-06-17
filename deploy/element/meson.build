install_data ('component.js', install_dir : packagedir_element / 'meta')

element_operations = ''
if host_machine.system() == 'windows'
    element_operations = '''<Operations>
        <Operation name="CreateShortcut">
            <Argument>@TargetDir@/bin/element.exe</Argument>
            <Argument>@DesktopDir@/Element.lnk</Argument>
            <Argument>workingDirectory=@TargetDir@</Argument>
        </Operation>
        <Operation name="CreateShortcut">
            <Argument>@TargetDir@/bin/element.exe</Argument>
            <Argument>@StartMenuDir@/Element/Element.lnk</Argument>
            <Argument>workingDirectory=@TargetDir@</Argument>
        </Operation>
        <Operation name="CreateShortcut">
            <Argument>@TargetDir@/updater.exe</Argument>
            <Argument>@StartMenuDir@/Element/Updater.lnk</Argument>
            <Argument>workingDirectory=@TargetDir@</Argument>
        </Operation>
    </Operations>'''
elif host_machine.system() == 'darwin'
    element_operations = '''<Operations>
        <Operation name="Extract">
            <Argument>/Applications</Argument>
        </Operation>
    </Operations>'''
endif

configure_file (input : 'package.xml',
    install: true, install_dir: packagedir_element / 'meta',
    output: 'package.xml', configuration: {
        'VERSION': version_with_build,
        'RELEASEDATE': element_release_date,
        'OPERATIONS': element_operations
    })

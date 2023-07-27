function isWindows() { return systemInfo.kernelType === "winnt"; }

function extractDir() {
    var dir = "";
    if (systemInfo.kernelType === "darwin") {
        dir = "/Applications";
    } else if (systemInfo.kernelType === "winnt") {
        dir = installer.value ("ApplicationsDirX64") + "/Kushview/Element";
    }
    return dir;
}

function Component() {}

Component.prototype.createOperations = function () {
    for (var i = 0; i < component.archives.length; ++i) {
        let a = component.archives [i]
        if (! a.endsWith ('7z'))
            continue;
        let target = extractDir();
        if (target.length <= 0)
            continue;
        component.addElevatedOperation ("Extract", a, target)
    }

    if (isWindows()) {
        component.addOperation ('CreateShortcut',
            '@ApplicationsDirX64@/Kushview/Element/bin/element.exe',
            '@DesktopDir@/Element.lnk',
            'workingDirectory=@TargetDir@');
        component.addOperation ('CreateShortcut',
            '@ApplicationsDirX64@/Kushview/Element/bin/element.exe',
            '@StartMenuDir@/Element/Element.lnk',
            'workingDirectory=@TargetDir@');
        component.addOperation ('CreateShortcut',
            '@TargetDir@/updater.exe',
            '@StartMenuDir@/Element/Updater.lnk',
            'workingDirectory=@TargetDir@');

        exePat = installer.value ("ApplicationsDirX64") + "\\Kushview\\Element\\bin\\element.exe"
        component.addOperation ('RegisterFileType', 'els', exePat + " '%1'", 'Element Session');
        component.addOperation ('RegisterFileType', 'elg', exePat + " '%1'", 'Element Graph');
    }
};

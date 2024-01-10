function isWindows() { return systemInfo.kernelType === "winnt"; }

function extractDir() {
    var dir = "/usr/local";
    if (systemInfo.kernelType === "darwin") {
        dir = "/Applications";
    } else if (systemInfo.kernelType === "winnt") {
        dir = installer.value("ApplicationsDirX64") + "/Kushview/Element";
    }
    return dir;
}

function Component() {
    if (installer.isInstaller()) {
        component.loaded.connect(this, Component.prototype.installerLoaded);
    }
}

Component.prototype.installerLoaded = function () {
    gui.pageById(QInstaller.ReadyForInstallation).left
        .connect(this, Component.prototype.handleUninstallPreviousVersion);
}

Component.prototype.createOperations = function () {
    for (var i = 0; i < component.archives.length; ++i) {
        let a = component.archives[i]
        if (!a.endsWith('7z'))
            continue;
        let target = extractDir();
        if (target.length <= 0)
            continue;
        component.addElevatedOperation("Extract", a, target)
    }

    if (isWindows()) {
        component.addOperation('CreateShortcut',
            '@ApplicationsDirX64@/Kushview/Element/bin/element.exe',
            '@DesktopDir@/Element.lnk',
            'workingDirectory=@TargetDir@');
        component.addOperation('CreateShortcut',
            '@ApplicationsDirX64@/Kushview/Element/bin/element.exe',
            '@StartMenuDir@/Element/Element.lnk',
            'workingDirectory=@TargetDir@');
        component.addOperation('CreateShortcut',
            '@TargetDir@/updater.exe',
            '@StartMenuDir@/Element/Updater.lnk',
            'workingDirectory=@TargetDir@');

        exePat = installer.value("ApplicationsDirX64") + "\\Kushview\\Element\\bin\\element.exe"
        component.addOperation('RegisterFileType', 'els', exePat + " '%1'", 'Element Session');
        component.addOperation('RegisterFileType', 'elg', exePat + " '%1'", 'Element Graph');
    }
};

Component.prototype.handleUninstallPreviousVersion = function () {
    if (!isWindows())
        return;
    var dir = installer.value("TargetDir");
    if (installer.fileExists(dir) && installer.fileExists(dir + "/updater.exe")) {
        console.log("running uninstall quietly")
        installer.gainAdminRights();
        installer.execute (dir + '/updater.exe', ['purge', '-c'])
    } else {
        console.log("not running uninstall")
    }
}

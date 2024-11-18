//==============================================================================
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

function kvEncodeURI (str) {
    return str.replace ('@', '%40')
        .replace (':', '%3A');
}

function kvPlatform() {
    if (systemInfo.kernelType == 'darwin')
        return 'osx'
    else if (systemInfo.kernelType == 'winnt')
        return 'windows'
    return 'linux'
}

//==============================================================================
function Component() {    
    if (installer.isInstaller()) {
        // installer.addWizardPage (component, "UpdateKeyWidget", QInstaller.ComponentSelection);
        // installer.addWizardPage (component, "UpdateKeyWidget", QInstaller.Introduction);
        // installer.addWizardPage (component, "UpdateKeyWidget", QInstaller.TargetDirectory);
        component.loaded.connect (this, Component.prototype.installerLoaded);
    }
}

//==============================================================================
Component.prototype.installerLoaded = function () {
    gui.pageById(QInstaller.ReadyForInstallation).left
        .connect(this, Component.prototype.handleUninstallPreviousVersion);

    var widget = component.userInterface ('UpdateKeyWidget');
    if (widget != null) {
        stabilizeUpdateKeyWidget();
        updateRepositories();
        widget.updateKeyType.currentIndexChanged.connect (
            Component.prototype.onUpdateKeyTypeChanged)
        widget.releaseChannel.currentIndexChanged.connect (
            Component.prototype.onReleaseChannelChanged)
    }
}

//==============================================================================
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

//==============================================================================
function updateKeyTypeSlug() {
    var widget = component.userInterface ('UpdateKeyWidget');
    var keytype = widget.updateKeyType.currentText
        .toLowerCase()
        .trim()
        .replace (' ', '-');
    
    if (keytype == 'membership')
        keytype = 'member';

    return keytype;
}

function releaseChannelSlug() {
    var widget = component.userInterface ('UpdateKeyWidget');
    var slug = widget.releaseChannel.currentText
        .toLowerCase()
        .trim();
    return slug;
}

function publicRepoUrl() {
    return 'https://repo.kushview.net/element/1/public/' + kvPlatform();
}

function makeRepoUrl() {
    var widget = component.userInterface ('UpdateKeyWidget');
    let host = 'repo.kushview.net'
    let package = 'element';
    let version = '1';
    let channel = releaseChannelSlug();
    let keytype = updateKeyTypeSlug();

    var url = "https://" +
        kvEncodeURI (widget.username.text) + ':' +
        kvEncodeURI (keytype + ':' + widget.updateKey.text) + '@' + 
        host + '/' + package + '/' + version + '/' + channel + '/' + kvPlatform();
    return url;
}

function stabilizeUpdateKeyWidget() {
    var widget = component.userInterface ('UpdateKeyWidget');
    let showKeyWidgets = widget.releaseChannel.currentText != 'Public';
    widget.username.visible = showKeyWidgets;
    widget.labelUsername.visible = showKeyWidgets;
    widget.updateKeyType.visible = showKeyWidgets;
    widget.labelUpdateKeyType.visible = showKeyWidgets;
    widget.updateKey.visible = showKeyWidgets;
    widget.labelUpdateKey.visible = showKeyWidgets;
}

function updateRepositories() {
    var widget = component.userInterface ('UpdateKeyWidget');

    if (widget.releaseChannel.currentText === 'Public') {
        console.log ('[repo] public repository active');
        installer.setTemporaryRepositories ([], false);
    } else {
        console.log ('[repo] authenticated repository active.')
        console.log ('[repo] username: ' + widget.username.text)
        console.log ('[repo] key:      ' + widget.updateKey.text)
        installer.setTemporaryRepositories ([ makeRepoUrl() ], true);
    }

    installer.cancelMetaInfoJob();
    installer.recalculateAllComponents();
    installer.calculateComponentsToInstall();
    installer.coreNetworkSettingsChanged();
}

Component.prototype.onReleaseChannelChanged = function(index) {
    stabilizeUpdateKeyWidget();
    updateRepositories();
}

Component.prototype.onUpdateKeyTypeChanged = function(index) {
    updateRepositories();
}

//==============================================================================
Component.prototype.handleUninstallPreviousVersion = function () {
    if (!isWindows())
        return;
    var dir = installer.value("TargetDir");
    if (installer.fileExists(dir) && installer.fileExists(dir + "/updater.exe")) {
        console.log("[element] running uninstall quietly")
        installer.gainAdminRights();
        installer.execute (dir + '/updater.exe', ['purge', '-c'])
    } else {
        console.log("[element] not running uninstall")
    }
}

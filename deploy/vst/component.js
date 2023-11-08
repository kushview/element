function extractDir() {
    var dir = "/usr/local/lib/vst";
    if (systemInfo.kernelType === "darwin") {
        dir = "/Library/Audio/Plug-Ins/VST";
    } else if (systemInfo.kernelType === "winnt") {
        dir = installer.value ("ApplicationsDirX64") + "/Steinberg/VstPlugins";
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
};

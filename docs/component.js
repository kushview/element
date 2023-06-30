function extractDir() {
    var dir = "";
    if (systemInfo.kernelType === "darwin") {
        dir = "/Library/Application Support/Kushview/Element";
    } else if (systemInfo.kernelType === "winnt") {
        dir = "@ApplicationsDirX64@/Kushview/Element";
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

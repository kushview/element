
// Skip all pages and go directly to finished page.
// (see also componenterror example)
function cancelInstaller (message)
{
    installer.setDefaultPageVisible (QInstaller.Introduction, false);
    installer.setDefaultPageVisible (QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible (QInstaller.ReadyForInstallation, false);
    installer.setDefaultPageVisible (QInstaller.StartMenuSelection, false);
    installer.setDefaultPageVisible (QInstaller.PerformInstallation, false);
    installer.setDefaultPageVisible (QInstaller.LicenseCheck, false);

    var abortText = "<font color='red'>" + message +"</font>";
    installer.setValue ("FinishedText", abortText);
}

// Returns the major version number as int
//   string.split(".", 1) returns the string before the first '.',
//   parseInt() converts it to an int.
function majorVersion (str)
{
    return parseInt (str.split (".", 1));
}

function Component()
{
    if (installer.isInstaller()) {
        // installer.loaded.connect(this, Component.prototype.installerLoaded);
    }
    
    // start installer with -v to see debug output
    console.log ("OS: " + systemInfo.productType);
    console.log ("Kernel: " + systemInfo.kernelType + "/" + systemInfo.kernelVersion);

    var validOs = false;

    if (systemInfo.kernelType === "winnt") {
        if (majorVersion (systemInfo.kernelVersion) >= 6)
            validOs = true;
    } else if (systemInfo.kernelType === "darwin") {
        if (majorVersion (systemInfo.kernelVersion) >= 11)
            validOs = true;
    } else {
        if (systemInfo.productType !== "opensuse"
                || systemInfo.productVersion !== "13.2") {
            QMessageBox["warning"]("os.warning", "Installer",
                                   "Note that the binaries are only tested on OpenSUSE 13.2.",
                                   QMessageBox.Ok);
        }
        validOs = true;
    }

    if (! validOs) {
        cancelInstaller ("Installation on " + systemInfo.prettyProductName + " is not supported");
        return;
    }

    console.log ("CPU Architecture: " +  systemInfo.currentCpuArchitecture);

    if (systemInfo.kernelType === "winnt") {
        installer.componentByName("net.kushview.element.win64").setValue ("Virtual", "false");
    } else if (systemInfo.kernelType === "darwin") {
        installer.componentByName("net.kushview.element.osx").setValue ("Virtual", "false");
    }
}

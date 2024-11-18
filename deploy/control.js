function Controller()
{
    installer.setDefaultPageVisible (QInstaller.TargetDirectory, false);
}

//=============================================================================
Controller.prototype.IntroductionPageCallback = function() {}

Controller.prototype.TargetDirectoryPageCallback = function() {}

Controller.prototype.ComponentSelectionPageCallback = function() {
    installer.recalculateAllComponents();
    installer.calculateComponentsToInstall();
    installer.coreNetworkSettingsChanged();
}

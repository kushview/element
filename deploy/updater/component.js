function Component() {}

Component.prototype.createOperationsForArchive = function (archive) {
    let target = installer.value ('TargetDir');
    component.addElevatedOperation ("Extract", archive, target, 'UNDOOPERATION', '');
};


@BINARYCREATOR@ -v -i "net.kushview.element,net.kushview.element.lv2,net.kushview.element.lua" -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@ || exit
rmdir /s /q archives
mkdir archives
cd packages
@ARCHIVEGEN@ -c 9 -f tar.gz ../archives/element-windows-archives.tar.gz net.kushview.element net.kushview.element.lv2 net.kushview.element.lua || exit

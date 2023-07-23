
@BINARYCREATOR@ -v -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@ || exit
rmdir archives /s /q
mkdir archives
cd packages
@ARCHIVEGEN@ -c 9 -f tar.gz ../archives/element-windows-archives.tar.gz^
  net.kushview.element^
  net.kushview.element.lua^
  net.kushview.element.vst^
  net.kushview.element.vst3 || exit
cd ..

call "C:\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\Auxiliary\\Build\\vcvars64.bat"
@SIGNTOOL@

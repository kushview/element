
rootdir="$(pwd)"                           # root build directory
blddir="${rootdir}/deploy/osx"             # dest dir within build dir
srcdir="$(dirname "$rootdir")/deploy/osx"  # src dir in code base
Element_exe="${rootdir}/Element"
Element_app="${blddir}/Element.app"

# echo $rootdir
# echo $srcdir
# echo $blddir
# echo $Element_exe
# echo $Element_app
# exit 0

set -e
rm -rf "$Element_app"
mkdir -p "${Element_app}/Contents/MacOS"
mkdir -p "${Element_app}/Contents/Resources"
mkdir -p "${Element_app}/Contents/Frameworks"
mkdir -p "${Element_app}/Contents/PlugIns"

cp "${Element_exe}" "${Element_app}/Contents/MacOS/Element"
cp "${srcdir}/Info.plist" "${Element_app}/Contents/Info.plist"
cp "${srcdir}/PkgInfo" "${Element_app}/Contents/PkgInfo"
cp "${srcdir}/Icon.icns" "${Element_app}/Contents/Resources/Icon.icns"
exit 0

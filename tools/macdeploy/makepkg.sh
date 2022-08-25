#!/bin/bash

set -e
rm -rf build/stage build/dist build/dmg dist
meson install -C build --destdir="."

python tools/macdeploy/appbundle.py build/stage/lib/vst/KV-Element.vst -verbose 0 -distdir="build/dist" -no-clean
python tools/macdeploy/appbundle.py build/stage/lib/vst/KV-Element-FX.vst -verbose 0 -distdir="build/dist" -no-clean

python tools/macdeploy/appbundle.py build/stage/lib/vst3/KV-Element.vst3 -verbose 0  -distdir="build/dist" -no-clean
python tools/macdeploy/appbundle.py build/stage/lib/vst3/KV-Element-FX.vst3 -verbose 0  -distdir="build/dist" -no-clean

python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element.component -verbose 0  -distdir="build/dist" -no-clean
python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element-FX.component -verbose 0  -distdir="build/dist" -no-clean
python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element-MFX.component -verbose 0  -distdir="build/dist" -no-clean

python tools/macdeploy/appbundle.py build/stage/Element.app -verbose 0  -distdir="build/dist" -no-clean

/usr/local/bin/packagesbuild -v --build-folder="`pwd`/build/dmg" tools/macdeploy/element.pkgproj
dmgname=$(python tools/version.py --before="element-osx-" --revision --after=".dmg")
hdiutil create -srcfolder "build/dmg" -format "UDZO" -volname "Element" -ov "build/$dmgname"
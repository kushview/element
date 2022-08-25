#!/bin/bash

set -e
rm -rf build/stage build/dist build/dmg dist
meson install -C build --destdir="."

appbundle_opts="-verbose 1 -distdir=build/dist -no-clean"
if [ -n "${CODESIGN_IDENTITY}" ]; then
    export CODESIGNARGS="--sign \"${CODESIGN_IDENTITY}\" --timestamp --deep"
    appbundle_opts+=" -sign"
fi

python tools/macdeploy/appbundle.py build/stage/lib/vst/KV-Element.vst ${appbundle_opts}
python tools/macdeploy/appbundle.py build/stage/lib/vst/KV-Element-FX.vst ${appbundle_opts}

python tools/macdeploy/appbundle.py build/stage/lib/vst3/KV-Element.vst3 ${appbundle_opts}
python tools/macdeploy/appbundle.py build/stage/lib/vst3/KV-Element-FX.vst3 ${appbundle_opts}

python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element.component ${appbundle_opts}
python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element-FX.component ${appbundle_opts}
python tools/macdeploy/appbundle.py build/stage/lib/au/KV-Element-MFX.component ${appbundle_opts}

python tools/macdeploy/appbundle.py build/stage/Element.app ${appbundle_opts}

packages_opts=""
if [ -z "INSTALLER_IDENTITY" ]; then
    packages_opts+=" --identity=\"$INSTALLER_IDENTITY\""
fi

/usr/local/bin/packagesbuild -v --build-folder="`pwd`/build/dmg" \
    ${packages_opts} tools/macdeploy/element.pkgproj

dmgname=$(python tools/version.py --before="element-osx-" --revision --after=".dmg")
hdiutil create -srcfolder "build/dmg" -format "UDZO" -volname "Element" -ov "build/$dmgname"

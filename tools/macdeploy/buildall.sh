set -e
here=`pwd`

export ARCHS="x86_64 arm64"

version=`./waf version`-`git rev-parse --short HEAD`
dependsroot=`dirname ${here}`/depends
dependsdir="${dependsroot}/`sh ./config.guess`"
./waf configure --depends="${dependsdir}" build $@
python tools/macdeploy/appbundle.py \
    build/Applications/Element.app \
    -volname "Element" \
    -dmg "element-osx-${version}" -verbose 1
mv *.dmg dist/

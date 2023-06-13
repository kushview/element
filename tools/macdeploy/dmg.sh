export CODESIGNARGS="-s \"Developer ID Application: Kushview, LLC (XXXXXX)\" \"../tools/macdeploy/app.entitlements\" --options=runtime --deep --timestamp"
export NOTARYPASS="XXXXXX"
export BUNDLE="stage/Element.app"

xcrun stapler staple "${BUNDLE}"

find . -name "*.dmg" -delete
python ../tools/macdeploy/appbundle.py -verbose 2 \
    -no-plugins -dmg "$(python ../tools/version.py --before=element-osx- --revision --cwd=..)" \
    -sign \
    -volname Element \
    "${BUNDLE}"

xcrun altool --notarize-app \
    --primary-bundle-id "net.kushview.Element" \
    -u "info@kushview.net" \
    -p "${NOTARYPASS}" \
    --file $(find . -name "element-osx*.dmg")

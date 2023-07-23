echo "Building branch $GIT_BRANCH"

setup_args="${SETUP_ARGS}"
set -e

if [ "$CLEAN" = "true" ] || [ ! -d "build" ]; then
	echo "Setting up for clean build"
	rm -rf build
    setup_args=""
fi

mkdir -p ~/agent/tmp
meson subprojects update --reset
meson setup $setup_args \
	--native-file=meson/subs.ini \
	--native-file=meson/msvc.ini \
    -Dbuildtype=release \
    -Delement-plugins=enabled \
    -Ddefault_library=static \
    build
meson compile -C build -j4
meson test -C build

# Building Element
A simple guide on building Element with meson.  Pease see [mesonbuild.com](https://mesonbuild.com/Getting-meson.html) for how to install meson on your platform.

## Debian/Ubuntu
__Dependencies__

The following packages are needed...
```
sudo apt-get install python git build-essential pkg-config libboost-dev libfreetype6-dev libx11-dev libxext-dev libxrandr-dev libxcomposite-dev libxinerama-dev libxcursor-dev libasound2-dev lv2-dev liblilv-dev libsuil-dev ladspa-sdk libcurl4-openssl-dev fonts-roboto clang clang++
```

__Compiling__
```
meson setup build
meson compile -C build
```

If meson gives errors about missing packages, namely the LV2 related ones, then you might need to also setup subprojects.

```
meson subprojects update --reset
```

__Installing__
```
sudo meson install -C builddir
sudo ldconfig
```

## Arch Linux
Install these packages, then run the `meson` commands described above.

```
sudo pacman -S git lilv suil lv2 ladspa boost ttf-mswin10
```

## Mac OSX
__Dependencies__

Install [Boost](https://www.boost.org/) using [Homebrew](https://docs.brew.sh/).
```
brew install boost
```

__Build__
```
BOOST_ROOT="/usr/local/include" meson setup \
    --native-file="meson/subs.ini" \
    --native-file="meson/osx.ini" \
    build
meson compile -C build
```

This will make an app bundle somwhere in the `build` dir.  Run it...
```
open $(find build -name "Element.app")
```

## Windows (MSVC)

```
meson setup --native-file="meson/subs.ini" --native-file="meson/msvc.ini" build
meson compile -C build
```

After this, you should have `build/element.exe`.  If it complains about missing boost
and vstsdk paths, copy the `msvc.ini`, edit the paths, then use it in `meson setup`.


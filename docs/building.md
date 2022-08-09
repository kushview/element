# Building Element
A simple guide on building Element with meson.  Pease see [mesonbuild.com](https://mesonbuild.com/Getting-meson.html) for how to install meson on your platform.

__Sub Modules__

This project uses submodules. Be sure to do this on a fresh clone, or when pulling changes.
```
git submodule update --init
```

## Debian/Ubuntu
__Dependencies__

The following packages are needed...
```
sudo apt-get install python git build-essential pkg-config libboost-dev libfreetype6-dev libx11-dev libxext-dev libxrandr-dev libxcomposite-dev libxinerama-dev libxcursor-dev libasound2-dev lv2-dev liblilv-dev libsuil-dev ladspa-sdk libcurl4-openssl-dev fonts-roboto clang clang++
```

__Compiling__
```
meson setup builddir
meson compile -C builddir
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
BOOST_ROOT="/usr/local/include" meson setup --native-file="tools/machine/osx.ini" builddir
meson compile -C builddir
# install is needed to assemble the app bundle.
meson install --destdir="." -C builddir
```

The command above will produce a universal binary as `builddir/tmp/Element.app`

## Windows (MSVC)

```
meson setup --native-file="tools/machine/msvc.ini" builddir
meson compile -C builddir
```

After this, you should have `builddir/element.exe`.  If it complains about missing boost
and vstsdk paths, copy the `msvc.ini`, edit the paths, then use it in `meson setup`. 

If you don't have the vstsdk2.4, it can be removed from the .ini file, but there won't be
support for VST2

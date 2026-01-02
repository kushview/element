# Building Element
A simple guide on building Element with CMake.  Pease see [cmake.org](https://cmake.org/install/) for how to install CMake on your platform.

## Submodules
This project uses git submodules. To get them, run:
`git submodule update --init --recursive`

## Debian/Ubuntu
__Dependencies__

The following packages are needed...
```bash
sudo apt-get install git build-essential pkg-config libboost-dev \
    libfreetype-dev libx11-dev libxext-dev libxrandr-dev libxcomposite-dev \
    libxinerama-dev libxrender-dev libxcursor-dev libxrender-dev libasound2-dev \
    ladspa-sdk libcurl4-openssl-dev fonts-roboto clang clang++
```

__Compiling__
```
cmake -B build -G Ninja
cmake --build build
```

__Installing__
```
sudo cmake --install build
sudo ldconfig
```

## Arch Linux
Install these packages, then run the `cmake` commands described above.

```bash
sudo pacman -S git base-devel cmake ninja pkgconf boost \
    freetype2 fontconfig libx11 libxext libxrandr libxcomposite \
    libxinerama libxrender libxcursor alsa-lib jack2 \
    ladspa curl ttf-roboto clang
```

## Mac OSX
__Dependencies__

Install [Boost](https://www.boost.org/) using [Homebrew](https://docs.brew.sh/).
```
brew install boost
```

__Build__
```
cmake -B build
cmake --build build
```

This will make an app bundle somwhere in the `build` dir.  Run it...
```
open $(find build -name "Element.app")
```

## Windows (MSVC)

```
cmake -B build
cmake --build build
```

After this, you should have an `Element.exe` inside the `build` directory.

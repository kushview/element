# Building Element with Waf
The instructions below are for a Debian based distro such as Ubuntu. Usage of `sudo` below is optional depending on your system. If you're on Mac/PC and prefer Xcode/MSVC, check the [projucer projects](../tools/jucer).

#### Sub Modules
This project uses submodules
```
git submodule update --init
```

#### Dependencies
The following packages are needed...
```
sudo apt-get install python git build-essential pkg-config libboost-signals-dev libfreetype6-dev libx11-dev libxext-dev libxrandr-dev libxcomposite-dev libxinerama-dev libxcursor-dev libasound2-dev lv2-dev liblilv-dev libsuil-dev ladspa-sdk libcurl4-openssl-dev

# Optional
sudo apt-get install clang
```

#### Compiling
```
./waf configure
./waf build
```

#### Running
```
LD_LIBRARY_PATH="`pwd`/build/lib" build/bin/element
```

#### Installing
```
sudo ./waf install
```

#### Arch Linux
Quick Start.
Install these packages, then run the `waf` commands described for Ubuntu

```
sudo pacman -S git lilv suil lv2 ladspa boost
```


#### macOS

Install [Boost](https://www.boost.org/) using [Homebrew](https://docs.brew.sh/).
```
brew install boost
```

Build
```
./waf configure
./waf build
```

Test
```
DYLD_LIBRARY_PATH="`pwd`/build/lib" build/bin/test-element
```

Release build
```
./waf install
```

This produces `build/Applications/Element.app`.

```
open build/Applications/Element.app
```

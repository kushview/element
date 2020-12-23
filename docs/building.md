# Building Element with Waf
The instructions below are for *nix based system such as Ubuntu. Usage of `sudo` is optional depending on your system. There are also [projucer projects](../tools/jucer), if you prefer.

__Sub Modules__
This project uses submodules. Be sure to do this on a fresh clone, or when pulling changes.
```
git submodule update --init
```

__Dependencies__

The following packages are needed...
```
sudo apt-get install python git build-essential pkg-config libboost-signals-dev libfreetype6-dev libx11-dev libxext-dev libxrandr-dev libxcomposite-dev libxinerama-dev libxcursor-dev libasound2-dev lv2-dev liblilv-dev libsuil-dev ladspa-sdk libcurl4-openssl-dev
```

__Optional__

Waf will use clang by default if installed
```
sudo apt-get install clang
```

__Compiling__
```
./waf configure
./waf build
```

__Testing__
```
./waf check
```

__Running__
```
LD_LIBRARY_PATH="`pwd`/build/lib" build/bin/element
```

__Installing__
```
sudo ./waf install
sudo ldconfig
```

### Arch Linux
Install these packages, then run the `waf` commands described above.

```
sudo pacman -S git lilv suil lv2 ladspa boost
```

### Mac OSX
__Dependencies__
Install [Boost](https://www.boost.org/) using [Homebrew](https://docs.brew.sh/).
```
brew install boost
```
If you want LV2 support on OSX, you'll also need....
```
brew install pkg-config lilv suil
```

__Build__
```
./waf configure
./waf build
```
This produces `build/Applications/Element.app`. 
```
# To run it...
open build/Applications/Element.app
```

__Test__
```
./waf check
```

__Install__
```
./waf install
```
Install the app to `/Applications` or another prefix of your choice. see `./waf --help`

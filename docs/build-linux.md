# Building Element on Linux
The instructions below are for a Debian based distro such as Ubuntu. Usage of `sudo` below is optional depending on your system.

#### Dependencies
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
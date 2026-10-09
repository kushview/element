# Building Element
A simple guide on building Element with CMake.  Please see [cmake.org](https://cmake.org/download/) for how to install CMake on your platform.

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
    ladspa-sdk lv2-dev liblilv-dev libsuil-dev libcurl4-openssl-dev fonts-roboto clang
```

Optional, to build and install the user manual, put the Python packages from
`docs/manual/requirements.txt` on your PATH (for example in a virtualenv):
```bash
python3 -m venv ~/.venvs/element-docs
~/.venvs/element-docs/bin/pip install -r docs/manual/requirements.txt
export PATH=~/.venvs/element-docs/bin:$PATH
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

The user manual is optional; see the virtualenv note under Debian/Ubuntu.

### Checking With Docker

You can also build in a Docker container without installing packages on your system:

```bash
# Build the Arch Linux environment image
docker build -f Dockerfile.archlinux -t element:archlinux .

# Build the project with your source mounted as a volume
docker run --rm --user $(id -u):$(id -g) -v $(pwd):/workspace element:archlinux bash -c "
  git config --global --add safe.directory /workspace && \
  git submodule update --init --recursive && \
  cmake -B build-arch -G Ninja -DCMAKE_BUILD_TYPE=Release -DELEMENT_BUILD_PLUGINS=ON && \
  cmake --build build-arch && \
  ctest --test-dir build-arch --output-on-failure
"
```

Or run interactively:
```bash
docker run --rm -it --user $(id -u):$(id -g) -v $(pwd):/workspace element:archlinux
# Then run cmake commands manually inside the container
```


## macOS
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

This will make an app bundle somewhere in the `build` dir.  Run it...
```
open $(find build -name "Element.app")
```

## Windows (MSVC)

```
cmake -B build
cmake --build build
```

After this, you should have an `Element.exe` inside the `build` directory.

## Documentation
With the Python packages above on your PATH, `cmake --build build --target docs`
builds the HTML manual into `build/docs/manual/html` (on Linux it is also part of
the default build and installed). `--target docs-manual-pdf` typesets the manual
as a PDF book; it needs a TeX distribution with `xelatex` (and `latexmk` if
available) and the
packages Sphinx relies on (`tabulary capt-of needspace framed titlesec varwidth
wrapfig fncychap newunicodechar` and the Latin Modern OpenType fonts). A full
TeX Live has all of them; on BasicTeX run `sudo tlmgr install latexmk tabulary
capt-of needspace framed titlesec varwidth wrapfig fncychap newunicodechar`.
`--target docs-lua` builds the Lua API reference with `ldoc`.

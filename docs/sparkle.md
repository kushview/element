# Sparkle Development
Tips and tricks when working on the Sparkle updater for macOS

## Setup

## Clearning Settings

**Forcing Info.plist generation**

Some of the Sparkle settings live in Element's `Info.plist`. To force it's recreation and be assured it actually did so.  Delete the artefacts created by JUCE, run cmake, and rebuild.

```sh
cd build
rm -rf element_app_artefacts && cmake .. && ninja
```

**Make sparkle think it is a "first run"**

```sh
defaults delete net.kushview.Element
rm -rf ~/Library/Caches/net.kushview.Element
```

# Win Sparkle Development
Tips and tricks when working on the WinSparkle updater for Windows

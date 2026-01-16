[![REUSE status](https://api.reuse.software/badge/github.com/kushview/element)](https://api.reuse.software/info/github.com/kushview/element) [![Build and Test](https://github.com/kushview/element/actions/workflows/build.yml/badge.svg)](https://github.com/kushview/element/actions/workflows/build.yml) [![Generate Lua Documentation](https://github.com/kushview/element/actions/workflows/ldoc.yml/badge.svg)](https://github.com/kushview/element/actions/workflows/ldoc.yml)

# Element
![Element Screenshot](data/screenshot.png)

### ADVANCED AUDIO PLUGIN HOST
This is the community version of Element, a modular AU/LV2/VST/VST3/CLAP audio plugin host. Create powerful effects, racks and instruments by connecting nodes to one another. Integrates with your existing hardware via standard protocols such as MIDI. See [kushview.net](https://kushview.net/element/) for more information and [pre-built binaries](https://kushview.net/element/download/). 

_See also:_ [Element User Manual](https://element.readthedocs.io)

### Compatibility
Element currently loads most major plugin formats.

| OS       | Version       | Formats         |
| -------- |:-------------:| ---------------:|
| Linux*   |       -       | LADSPA/LV2/VST3/CLAP |
| Mac OSX  | 10.8 and up   | AU/VST/VST3/LV2/CLAP |
| Windows  | XP and up     | VST/VST3/LV2/CLAP    |

_*Ubuntu is the most tested, but should run on any major distribution_

### Features
* Runs standalone or as a plugin in your DAW**
* Route Audio and MIDI from anywhere to anywhere
* Play virtual instruments and effects live
* Create re-usable instruments and effect graphs
* External Sync with MIDI Clock
* Sub Graphing – Nest Graphs within each other
* Custom Keyboard Shortcuts
* Placeholder Nodes
* Built In Virtual Keyboard
* Multiple Undo/Redo
* Scripting - Custom DSP and DSP UI's
* Embed plugin UIs directly in Graphs
* And more...

### Building 
See [building.md](./docs/building.md) for instructions and dependency details.

### Contributing
If you'd like to contribute code please review the [code style](./docs/cppstyle.md) and [contributor notes](CONTRIBUTING.md) before submitting pull requests.  You may also want to join the [#element](https://discord.gg/fAsQ5fMuHy) channel on the Kushview [Discord](https://discord.gg/fAsQ5fMuHy) server.

### Issue Reporting
Please report bugs and feature requests on Gitlab. [Element issue tracker](https://gitlab.com/kushview/element/-/issues).

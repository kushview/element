// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>
#define EL_TAG(n) static const juce::Identifier n = #n;

namespace element {
namespace types {
EL_TAG (DSP)
EL_TAG (View)
EL_TAG (GraphView)
EL_TAG (Anonymous)
EL_TAG (Control)
EL_TAG (Controller)
EL_TAG (Graph)
EL_TAG (Node)
EL_TAG (Script)
EL_TAG (Session)
} // namespace types

namespace tags {
static const juce::Identifier asset = "asset";
static const juce::Identifier assetId = "assetId";
static const juce::Identifier attack = "attack";
static const juce::Identifier block = "block";
static const juce::Identifier bpm = "bpm";
static const juce::Identifier category = "category";
static const juce::Identifier channel = "channel";
static const juce::Identifier clip = "clip";
static const juce::Identifier decay = "decay";
static const juce::Identifier file = "file";
static const juce::Identifier fsid = "fsid";
static const juce::Identifier events = "events";
static const juce::Identifier graph = "graph";
static const juce::Identifier group = "group";
static const juce::Identifier id = "id";
static const juce::Identifier index = "index";
static const juce::Identifier keyId = "keyId";
static const juce::Identifier keyMap = "keyMap";
static const juce::Identifier length = "length";
static const juce::Identifier media = "media";
static const juce::Identifier name = "name";
static const juce::Identifier nodeId = "nodeId";
static const juce::Identifier note = "note";
static const juce::Identifier offset = "offset";
static const juce::Identifier path = "path";
static const juce::Identifier pattern = "pattern";
static const juce::Identifier parent = "parent";
static const juce::Identifier pitch = "pitch";
static const juce::Identifier ppq = "ppq";
static const juce::Identifier release = "release";
static const juce::Identifier sampler = "sampler";
static const juce::Identifier sequence = "sequence";
static const juce::Identifier start = "start";
static const juce::Identifier sustain = "sustain";
static const juce::Identifier tempo = "tempo";
static const juce::Identifier track = "track";
static const juce::Identifier type = "type";
static const juce::Identifier velocity = "velocity";
static const juce::Identifier version = "version";
static const juce::Identifier volume = "volume";
static const juce::Identifier dock = "dock";
static const juce::Identifier panel = "panel";
static const juce::Identifier item = "item";
static const juce::Identifier area = "area";
static const juce::Identifier vertical = "vertical";
static const juce::Identifier bounds = "bounds";
static const juce::Identifier barSize = "barSize";
static const juce::Identifier sizes = "sizes";
static const juce::Identifier active = "active";
static const juce::Identifier arc = "arc";
static const juce::Identifier arcs = "arcs";
static const juce::Identifier bypass = "bypass";
static const juce::Identifier control = "control";
static const juce::Identifier controller = "controller";
static const juce::Identifier controllers = "controllers";
static const juce::Identifier collapsed = "collapsed";
static const juce::Identifier delayCompensation = "delayCompensation";
static const juce::Identifier displayMode = "displayMode";
static const juce::Identifier enabled = "enabled";
static const juce::Identifier gain = "gain";
static const juce::Identifier graphs = "graphs";
static const juce::Identifier hiddenPorts = "hiddenPorts";
static const juce::Identifier mappingData = "mappingData";
static const juce::Identifier map = "map";
static const juce::Identifier maps = "maps";
static const juce::Identifier missing = "missing";
static const juce::Identifier mute = "mute";
static const juce::Identifier node = "node";
static const juce::Identifier nodes = "nodes";
static const juce::Identifier notes = "notes";
static const juce::Identifier oversamplingFactor = "oversamplingFactor";
static const juce::Identifier persistent = "persistent";
static const juce::Identifier placeholder = "placeholder";
static const juce::Identifier port = "port";
static const juce::Identifier ports = "ports";
static const juce::Identifier preset = "preset";
static const juce::Identifier program = "program";
static const juce::Identifier sourceNode = "sourceNode";
static const juce::Identifier sourcePort = "sourcePort";
static const juce::Identifier sourceChannel = "sourceChannel";
static const juce::Identifier destNode = "destNode";
static const juce::Identifier destPort = "destPort";
static const juce::Identifier destChannel = "destChannel";

static const juce::Identifier identifier = "identifier";
static const juce::Identifier symbol = "symbol";
static const juce::Identifier format = "format";
static const juce::Identifier flow = "flow";
static const juce::Identifier input = "input";
static const juce::Identifier inputDevice = "inputDevice";

static const juce::Identifier object = "object";
static const juce::Identifier output = "output";

static const juce::Identifier session = "session";
static const juce::Identifier state = "state";
static const juce::Identifier programState = "programState";
static const juce::Identifier beatsPerBar = "beatsPerBar";
static const juce::Identifier beatDivisor = "beatDivisor";
static const juce::Identifier midiChannel = "midiChannel";
static const juce::Identifier midiChannels = "midiChannels";
static const juce::Identifier midiProgram = "midiProgram";
static const juce::Identifier midiProgramsEnabled = "midiProgramsEnabled";
static const juce::Identifier globalMidiPrograms = "globalMidiPrograms";
static const juce::Identifier midiProgramsState = "midiProgramsState";
static const juce::Identifier renderMode = "renderMode";

static const juce::Identifier staticPos = "staticPos";

static const juce::Identifier plugin = "plugin";

static const juce::Identifier windowOnTop = "windowOnTop";
static const juce::Identifier windowVisible = "windowVisible";
static const juce::Identifier x = "x";
static const juce::Identifier y = "y";
static const juce::Identifier windowX = "windowX";
static const juce::Identifier windowY = "windowY";
static const juce::Identifier relativeX = "relativeX";
static const juce::Identifier relativeY = "relativeY";
static const juce::Identifier width = "width";
static const juce::Identifier height = "height";
static const juce::Identifier pluginName = "pluginName";
static const juce::Identifier pluginIdentifierString = "pluginIdentifierString";
static const juce::Identifier uuid = "uuid";
static const juce::Identifier ui = "ui";
static const juce::Identifier parameter = "parameter";
static const juce::Identifier offline = "offline";

static const juce::Identifier transpose = "transpose";
static const juce::Identifier keyStart = "keyStart";
static const juce::Identifier keyEnd = "keyEnd";

static const juce::Identifier velocityCurveMode = "velocityCurveMode";
static const juce::Identifier workspace = "workspace";

static const juce::Identifier externalSync = "externalSync";

static const juce::Identifier updater = "updater";
static const juce::Identifier code = "code";
static const juce::Identifier source = "source";
static const juce::Identifier scripts = "scripts";
} // namespace tags

} // namespace element
#undef EL_TAG

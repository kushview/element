
libelement_sources = '''
    bindings.cpp
    context.cpp
    module.cpp
    scripting.cpp
    strings.cpp
    el/audio.c
    el/AudioBuffer32.cpp
    el/AudioBuffer64.cpp
    el/Bounds.cpp
    el/bytes.c
    el/CommandManager.cpp
    el/Desktop.cpp
    el/DocumentWindow.cpp
    el/File.cpp
    el/Globals.cpp
    el/Graphics.cpp
    el/midi.c
    el/MidiBuffer.cpp
    el/MidiMessage.cpp
    el/MouseEvent.cpp
    el/Node.cpp
    el/Parameter.cpp
    el/Point.cpp
    el/Range.cpp
    el/Rectangle.cpp
    el/round.c
    el/Session.cpp
    el/Slider.cpp
    el/TextButton.cpp
    el/Widget.cpp
    capslock.cpp
    commands.cpp
    controllers/AppController.cpp
    controllers/Controller.cpp
    controllers/DevicesController.cpp
    controllers/EngineController.cpp
    controllers/GraphController.cpp
    controllers/GraphManager.cpp
    controllers/GuiController.cpp
    controllers/MappingController.cpp
    controllers/OSCController.cpp
    controllers/PresetsController.cpp
    controllers/ScriptingController.cpp
    controllers/SessionController.cpp
    datapath.cpp
    db/Database.cpp
    documents/GraphDocument.cpp
    documents/SessionDocument.cpp
    engine/AudioEngine.cpp
    engine/GraphBuilder.cpp
    engine/GraphNode.cpp
    engine/GraphPort.cpp
    engine/InternalFormat.cpp
    engine/IONode.cpp
    engine/MappingEngine.cpp
    engine/MidiClock.cpp
    engine/MidiEngine.cpp
    engine/MidiPipe.cpp
    engine/NodeFactory.cpp
    engine/NodeObject.cpp
    engine/nodes/AudioFilePlayerNode.cpp
    engine/nodes/AudioMixerProcessor.cpp
    engine/nodes/AudioProcessorNode.cpp
    engine/nodes/AudioRouterNode.cpp
    engine/nodes/CompressorProcessor.cpp
    engine/nodes/EQFilterProcessor.cpp
    engine/nodes/LuaNode.cpp
    engine/nodes/MediaPlayerProcessor.cpp
    engine/nodes/MidiChannelSplitterNode.cpp
    engine/nodes/MidiDeviceProcessor.cpp
    engine/nodes/MidiFilterNode.cpp
    engine/nodes/MidiMonitorNode.cpp
    engine/nodes/MidiProgramMapNode.cpp
    engine/nodes/MidiRouterNode.cpp
    engine/nodes/OSCReceiverNode.cpp
    engine/nodes/OSCSenderNode.cpp
    engine/nodes/ScriptNode.cpp
    engine/Oversampler.cpp
    engine/Parameter.cpp
    engine/RootGraph.cpp
    engine/Transport.cpp
    globals.cpp
    gui/AboutComponent.cpp
    gui/AssetTreeView.cpp
    gui/AudioIOPanelView.cpp
    gui/BlockComponent.cpp
    gui/Buttons.cpp
    gui/ChannelStripComponent.cpp
    gui/ConnectionGrid.cpp
    gui/ContentComponent.cpp
    gui/ContextMenus.cpp
    gui/GraphEditorComponent.cpp
    gui/Icons.cpp
    gui/LookAndFeel.cpp
    gui/LuaTokeniser.cpp
    gui/MainMenu.cpp
    gui/MainWindow.cpp
    gui/NavigationConcertinaPanel.cpp
    gui/NodeEditorFactory.cpp
    gui/NodeIOConfiguration.cpp
    gui/nodes/AudioRouterEditor.cpp
    gui/nodes/CompressorNodeEditor.cpp
    gui/nodes/EQFilterNodeEditor.cpp
    gui/nodes/GenericNodeEditor.cpp
    gui/nodes/KnobsComponent.cpp
    gui/nodes/LuaNodeEditor.cpp
    gui/nodes/MidiMonitorNodeEditor.cpp
    gui/nodes/MidiProgramMapEditor.cpp
    gui/nodes/MidiRouterEditor.cpp
    gui/nodes/NodeEditorComponent.cpp
    gui/nodes/OSCReceiverNodeEditor.cpp
    gui/nodes/OSCSenderNodeEditor.cpp
    gui/nodes/ScriptNodeEditor.cpp
    gui/nodes/VolumeNodeEditor.cpp
    gui/PluginManagerComponent.cpp
    gui/PluginWindow.cpp
    gui/PreferencesComponent.cpp
    gui/properties/NodeProperties.cpp
    gui/properties/NodePropertyPanel.cpp
    gui/RackContentView.cpp
    gui/SequencerClipItem.cpp
    gui/SequencerComponent.cpp
    gui/SessionImportWizard.cpp
    gui/SessionTreePanel.cpp
    gui/StandardContentComponent.cpp
    gui/SystemTray.cpp
    gui/TransportBar.cpp
    gui/TreeviewBase.cpp
    gui/ViewHelpers.cpp
    gui/views/ContentView.cpp
    gui/views/ControllerDevicesView.cpp
    gui/views/ControllerMapsView.cpp
    gui/views/GraphEditorView.cpp
    gui/views/GraphMixerView.cpp
    gui/views/GraphSettingsView.cpp
    gui/views/KeymapEditorView.cpp
    gui/views/LuaConsoleView.cpp
    gui/views/NavigationView.cpp
    gui/views/NodeChannelStripView.cpp
    gui/views/NodeEditorContentView.cpp
    gui/views/NodeMidiContentView.cpp
    gui/views/NodePortsTable.cpp
    gui/views/PluginsPanelView.cpp
    gui/views/ScriptEditorView.cpp
    gui/views/SessionSettingsView.cpp
    gui/views/SessionTreeContentView.cpp
    gui/views/VirtualKeyboardView.cpp
    gui/widgets/AudioDeviceSelectorComponent.cpp
    gui/widgets/Console.cpp
    gui/widgets/HorizontalListBox.cpp
    gui/widgets/LuaConsole.cpp
    gui/widgets/MidiBlinker.cpp
    gui/widgets/NodeMidiProgramComponent.cpp
    gui/widgets/SessionGraphsListBox.cpp
    gui/Window.cpp
    gui/WindowManager.cpp
    gui/workspace/PanelTypes.cpp
    gui/workspace/PluginsPanel.cpp
    gui/Workspace.cpp
    gui/WorkspacesContentComponent.cpp
    messages.cpp
    plugins/PluginEditor.cpp
    plugins/PluginProcessor.cpp
    scripting/DSPScript.cpp
    scripting/DSPUIScript.cpp
    scripting/JuceBindings.cpp
    scripting/LuaBindings.cpp
    scripting/LuaLib.cpp
    scripting/Script.cpp
    scripting/ScriptDescription.cpp
    scripting/ScriptingEngine.cpp
    scripting/ScriptManager.cpp
    session/Asset.cpp
    session/AssetTree.cpp
    session/ControllerDevice.cpp
    session/DeviceManager.cpp
    session/Graph.cpp
    session/MediaManager.cpp
    session/MidiClip.cpp
    session/Node.cpp
    session/Note.cpp
    session/NoteSequence.cpp
    session/PluginManager.cpp
    session/Sequence.cpp
    session/Session.cpp
    session/SessionTrack.cpp
    settings.cpp
    utils.cpp
    version.cpp
    ../libs/compat/BinaryData.cpp
'''.split()

libelement_includes =  [ 
    include_directories (
        '../libs/sol',
        '../include',
        '../scripts',
        'el',
        '.'
    ),
    lua_includes, 
    juce_includes, 
    vst3_includes, 
    vst2_includes, 
    lv2_includes 
]

host_cpp_args = []
if host_machine.system() == 'linux'
    absdatadir = get_option('prefix') / get_option('datadir')
    host_cpp_args += [
        '-DEL_LUADIR="@0@"'.format (absdatadir / 'element' / 'lua'),
        '-DEL_SCRIPTSDIR="@0@"'.format (absdatadir / 'element' / 'scripts')
    ]
endif

if host_machine.system() == 'windows'
    libelement_sources += [ 'dlfcn-win32.c' ]
endif

luael_gen = generator (jrc,
    output : [ '@BASENAME@.cpp', '@BASENAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'BinaryData' ])
libelement_sources += luael_gen.process (
    'el/AudioBuffer.lua',
    'el/command.lua',
    'el/object.lua',
    'el/script.lua',
    'el/slug.lua'
)

libelement_includes += scripts_includes
libelement_sources  += scripts_sources
libelement_cpp_args  = host_cpp_args

libelement = static_library ('element', 
    libelement_sources, 
    include_directories : libelement_includes,
    gnu_symbol_visibility : 'hidden',
    dependencies : deps,
    cpp_args : libelement_cpp_args,
    install : false,
    objects : [ libelement_lua.extract_all_objects (recursive : false),
                libelement_juce.extract_all_objects (recursive : false) ])

if meson.is_subproject()
    element_dep = declare_dependency (
        include_directories : libelement_includes,
        link_with : libelement)
endif

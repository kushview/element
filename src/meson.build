
libelement_sources = '''
    arc.cpp
    bindings.cpp
    commands.cpp
    context.cpp
    controller.cpp
    datapath.cpp
    graph.cpp
    gzip.cpp
    lv2.cpp
    matrixstate.cpp
    messages.cpp
    model.cpp
    module.cpp
    node.cpp
    ringbuffer.cpp
    scripting.cpp
    semaphore.cpp
    settings.cpp
    services.cpp
    strings.cpp
    script.cpp
    timescale.cpp
    utils.cpp

    plugineditor.cpp
    pluginprocessor.cpp

    services/deviceservice.cpp
    services/engineservice.cpp
    services/guiservice.cpp
    services/mappingservice.cpp
    services/oscservice.cpp
    services/presetservice.cpp
    services/sessionservice.cpp
    
    engine/ionode.cpp
    engine/oversampler.cpp
    engine/graphmanager.cpp
    engine/internalformat.cpp
    engine/midiengine.cpp
    engine/mappingengine.cpp
    engine/processor.cpp
    engine/midipipe.cpp
    engine/nodes/ScriptNode.cpp
    engine/nodes/MidiProgramMapNode.cpp
    engine/nodes/AudioRouterNode.cpp
    engine/nodes/MidiRouterNode.cpp
    engine/nodes/MediaPlayerProcessor.cpp
    engine/nodes/OSCReceiverNode.cpp
    engine/nodes/MidiMonitorNode.cpp
    engine/nodes/MidiFilterNode.cpp
    engine/nodes/MidiChannelSplitterNode.cpp
    engine/nodes/CompressorProcessor.cpp
    engine/nodes/MidiDeviceProcessor.cpp
    engine/nodes/AudioMixerProcessor.cpp
    engine/nodes/EQFilterProcessor.cpp
    engine/nodes/AudioFilePlayerNode.cpp
    engine/nodes/OSCSenderNode.cpp
    engine/nodes/AudioProcessorNode.cpp
    
    engine/graphnode.cpp
    engine/transport.cpp
    engine/graphbuilder.cpp
    engine/parameter.cpp
    engine/midiclock.cpp
    engine/nodefactory.cpp
    engine/audioengine.cpp
    engine/portbuffer.cpp
    engine/rootgraph.cpp
    engine/shuttle.cpp

    lv2/logfeature.cpp
    lv2/module.cpp
    lv2/workthread.cpp
    lv2/workerfeature.cpp
    lv2/world.cpp
    lv2/native.cpp

    gui/AudioIOPanelView.cpp
    gui/Buttons.cpp
    gui/ChannelStripComponent.cpp
    gui/ConnectionGrid.cpp
    gui/ContextMenus.cpp
    gui/FileComboBox.cpp
    gui/GraphEditorComponent.cpp
    gui/Icons.cpp
    gui/LuaTokeniser.cpp
    gui/MainMenu.cpp
    gui/MainWindow.cpp
    gui/NodeEditorFactory.cpp
    gui/NodeIOConfiguration.cpp
    gui/PatchMatrixComponent.cpp
    gui/nodes/AudioRouterEditor.cpp
    gui/nodes/CompressorNodeEditor.cpp
    gui/nodes/EQFilterNodeEditor.cpp
    gui/nodes/GenericNodeEditor.cpp
    gui/nodes/KnobsComponent.cpp
    gui/nodes/MidiMonitorNodeEditor.cpp
    gui/nodes/MidiProgramMapEditor.cpp
    gui/nodes/MidiRouterEditor.cpp
    
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
    
    gui/SystemTray.cpp
    gui/TransportBar.cpp
    gui/TreeviewBase.cpp
    gui/ViewHelpers.cpp
    gui/views/scriptview.cpp
    gui/views/ContentView.cpp
    gui/views/ControllersView.cpp
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
    gui/widgets/FileTreeView.cpp
    gui/widgets/HorizontalListBox.cpp
    gui/widgets/LuaConsole.cpp
    gui/widgets/MidiBlinker.cpp
    gui/widgets/NodeMidiProgramComponent.cpp
    gui/widgets/SessionGraphsListBox.cpp
    gui/Window.cpp
    gui/WindowManager.cpp
   
    scripting/dspscript.cpp
    scripting/dspuiscript.cpp
    scripting/bindings.cpp
    scripting/scriptloader.cpp
    scripting/scriptmanager.cpp

    session/devicemanager.cpp
    session/pluginmanager.cpp
    session/session.cpp

    ui/aboutscreen.cpp
    ui/block.cpp
    ui/content.cpp
    ui/capslock.cpp
    ui/decibelscale.cpp
    ui/designer.cpp
    ui/grapheditor.cpp
    ui/meterbridge.cpp
    ui/navigation.cpp
    ui/nodeeditor.cpp
    ui/sessiondocument.cpp
    ui/simplemeter.cpp
    ui/standard.cpp
    ui/style_v1.cpp
    ui/updater.cpp

    el/audio.c
    el/AudioBuffer32.cpp
    el/AudioBuffer64.cpp
    el/Bounds.cpp
    el/bytes.c
    el/Commands.cpp
    el/Content.cpp
    el/Desktop.cpp
    el/Context.cpp
    el/Graph.cpp
    el/GraphEditor.cpp
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
    el/View.cpp
    el/Widget.cpp
    juce/BinaryData.cpp
'''.split()

libelement_includes =  [
    include_directories (
        'lua/sol',
        '../scripts',
        'el',
        '.'
    ),
    element_includes,
    lua_includes, 
    juce_includes, 
    juce_vst3_includes, 
    juce_vst2_includes, 
    juce_lv2_includes,
    jack_includes 
]

if jack_enabled
    libelement_sources += files ('''
        engine/jack.cpp
    '''.split())
endif

host_cpp_args = []
if host_machine.system() == 'linux'
    absdatadir = get_option('prefix') / get_option('datadir')
    host_cpp_args += [
        '-DEL_LUADIR="@0@"'.format (absdatadir / 'element' / 'lua'),
        '-DEL_SCRIPTSDIR="@0@"'.format (absdatadir / 'element' / 'scripts')
    ]
elif host_machine.system() == 'windows'
    libelement_sources += [ 'dlfcn-win32.c' ]
elif host_machine.system() == 'darwin'
    libelement_sources += [ 'ui/nsviewwithparent.mm']
endif

luael_gen = generator (bin2c,
    output : [ '@PLAINNAME@.cpp', '@PLAINNAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'LuaLib' ])
libelement_sources += luael_gen.process (
    'el/AudioBuffer.lua',
    'el/command.lua',
    'el/object.lua',
    'el/color.lua',
    'el/script.lua',
    'el/session.lua',
    'el/slug.lua'
)

libelement_includes += scripts_includes
libelement_sources  += scripts_sources
libelement_cpp_args  = host_cpp_args

libelement = static_library ('element',
    libelement_sources,
    include_directories : libelement_includes,
    gnu_symbol_visibility : 'hidden',
    dependencies : [ deps, juce_dep, lvtk_host_dep ],
    cpp_args : libelement_cpp_args,
    install : false,
    objects : [ libelement_lua.extract_all_objects (recursive : false),
                libelement_juce.extract_all_objects (recursive : false) ])

element_dep = declare_dependency (
    include_directories : libelement_includes,
    link_with : libelement,
    dependencies : [ deps, juce_dep ])

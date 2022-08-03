project ('element', ['c', 'cpp' ] + meson.get_external_property ('extra_languages', []),
    version : '0.47.0',
    meson_version : '>= 0.56.0',
    default_options : [ 'c_std=c11', 'cpp_std=c++17' ])

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# Global Arguments
add_global_arguments ([

], language : 'cpp')

# Boost
boost_dep = dependency ('boost', required : true)
deps = [ boost_dep ]

subdir ('libs/lua')
subdir ('libs/compat')
subdir ('scripts')

# libelement
libelement_sources = '''
    src/bindings.cpp
    src/context.cpp
    src/module.cpp
    src/scripting.cpp
    src/strings.cpp
    src/el/audio.c
    src/el/AudioBuffer32.cpp
    src/el/AudioBuffer64.cpp
    src/el/Bounds.cpp
    src/el/bytes.c
    src/el/CommandManager.cpp
    src/el/Desktop.cpp
    src/el/DocumentWindow.cpp
    src/el/File.cpp
    src/el/Globals.cpp
    src/el/Graphics.cpp
    src/el/midi.c
    src/el/MidiBuffer.cpp
    src/el/MidiMessage.cpp
    src/el/MouseEvent.cpp
    src/el/Node.cpp
    src/el/Parameter.cpp
    src/el/Point.cpp
    src/el/Range.cpp
    src/el/Rectangle.cpp
    src/el/round.c
    src/el/Session.cpp
    src/el/Slider.cpp
    src/el/TextButton.cpp
    src/el/Widget.cpp
    src/CapsLock.cpp
    src/Commands.cpp
    src/controllers/AppController.cpp
    src/controllers/Controller.cpp
    src/controllers/DevicesController.cpp
    src/controllers/EngineController.cpp
    src/controllers/GraphController.cpp
    src/controllers/GraphManager.cpp
    src/controllers/GuiController.cpp
    src/controllers/MappingController.cpp
    src/controllers/OSCController.cpp
    src/controllers/PresetsController.cpp
    src/controllers/ScriptingController.cpp
    src/controllers/SessionController.cpp
    src/DataPath.cpp
    src/db/Database.cpp
    src/documents/GraphDocument.cpp
    src/documents/SessionDocument.cpp
    src/engine/AudioEngine.cpp
    src/engine/GraphBuilder.cpp
    src/engine/GraphNode.cpp
    src/engine/GraphPort.cpp
    src/engine/InternalFormat.cpp
    src/engine/IONode.cpp
    src/engine/MappingEngine.cpp
    src/engine/MidiClock.cpp
    src/engine/MidiEngine.cpp
    src/engine/MidiPipe.cpp
    src/engine/NodeFactory.cpp
    src/engine/NodeObject.cpp
    src/engine/nodes/AudioFilePlayerNode.cpp
    src/engine/nodes/AudioMixerProcessor.cpp
    src/engine/nodes/AudioProcessorNode.cpp
    src/engine/nodes/AudioRouterNode.cpp
    src/engine/nodes/CompressorProcessor.cpp
    src/engine/nodes/EQFilterProcessor.cpp
    src/engine/nodes/LuaNode.cpp
    src/engine/nodes/MediaPlayerProcessor.cpp
    src/engine/nodes/MidiChannelSplitterNode.cpp
    src/engine/nodes/MidiDeviceProcessor.cpp
    src/engine/nodes/MidiFilterNode.cpp
    src/engine/nodes/MidiMonitorNode.cpp
    src/engine/nodes/MidiProgramMapNode.cpp
    src/engine/nodes/MidiRouterNode.cpp
    src/engine/nodes/OSCReceiverNode.cpp
    src/engine/nodes/OSCSenderNode.cpp
    src/engine/nodes/ScriptNode.cpp
    src/engine/Oversampler.cpp
    src/engine/Parameter.cpp
    src/engine/RootGraph.cpp
    src/engine/Transport.cpp
    src/Globals.cpp
    src/gui/AboutComponent.cpp
    src/gui/AssetTreeView.cpp
    src/gui/AudioIOPanelView.cpp
    src/gui/BlockComponent.cpp
    src/gui/Buttons.cpp
    src/gui/ChannelStripComponent.cpp
    src/gui/ConnectionGrid.cpp
    src/gui/ContentComponent.cpp
    src/gui/ContextMenus.cpp
    src/gui/GraphEditorComponent.cpp
    src/gui/Icons.cpp
    src/gui/LookAndFeel.cpp
    src/gui/LuaTokeniser.cpp
    src/gui/MainMenu.cpp
    src/gui/MainWindow.cpp
    src/gui/NavigationConcertinaPanel.cpp
    src/gui/NodeEditorFactory.cpp
    src/gui/NodeIOConfiguration.cpp
    src/gui/nodes/AudioRouterEditor.cpp
    src/gui/nodes/CompressorNodeEditor.cpp
    src/gui/nodes/EQFilterNodeEditor.cpp
    src/gui/nodes/GenericNodeEditor.cpp
    src/gui/nodes/KnobsComponent.cpp
    src/gui/nodes/LuaNodeEditor.cpp
    src/gui/nodes/MidiMonitorNodeEditor.cpp
    src/gui/nodes/MidiProgramMapEditor.cpp
    src/gui/nodes/MidiRouterEditor.cpp
    src/gui/nodes/NodeEditorComponent.cpp
    src/gui/nodes/OSCReceiverNodeEditor.cpp
    src/gui/nodes/OSCSenderNodeEditor.cpp
    src/gui/nodes/ScriptNodeEditor.cpp
    src/gui/nodes/VolumeNodeEditor.cpp
    src/gui/PluginManagerComponent.cpp
    src/gui/PluginWindow.cpp
    src/gui/PreferencesComponent.cpp
    src/gui/properties/NodeProperties.cpp
    src/gui/properties/NodePropertyPanel.cpp
    src/gui/RackContentView.cpp
    src/gui/SequencerClipItem.cpp
    src/gui/SequencerComponent.cpp
    src/gui/SessionImportWizard.cpp
    src/gui/SessionTreePanel.cpp
    src/gui/StandardContentComponent.cpp
    src/gui/SystemTray.cpp
    src/gui/TransportBar.cpp
    src/gui/TreeviewBase.cpp
    src/gui/ViewHelpers.cpp
    src/gui/views/ContentView.cpp
    src/gui/views/ControllerDevicesView.cpp
    src/gui/views/ControllerMapsView.cpp
    src/gui/views/GraphEditorView.cpp
    src/gui/views/GraphMixerView.cpp
    src/gui/views/GraphSettingsView.cpp
    src/gui/views/KeymapEditorView.cpp
    src/gui/views/LuaConsoleView.cpp
    src/gui/views/NavigationView.cpp
    src/gui/views/NodeChannelStripView.cpp
    src/gui/views/NodeEditorContentView.cpp
    src/gui/views/NodeMidiContentView.cpp
    src/gui/views/NodePortsTableView.cpp
    src/gui/views/PluginsPanelView.cpp
    src/gui/views/ScriptEditorView.cpp
    src/gui/views/SessionSettingsView.cpp
    src/gui/views/SessionTreeContentView.cpp
    src/gui/views/VirtualKeyboardView.cpp
    src/gui/widgets/AudioDeviceSelectorComponent.cpp
    src/gui/widgets/Console.cpp
    src/gui/widgets/HorizontalListBox.cpp
    src/gui/widgets/LuaConsole.cpp
    src/gui/widgets/MidiBlinker.cpp
    src/gui/widgets/NodeMidiProgramComponent.cpp
    src/gui/widgets/SessionGraphsListBox.cpp
    src/gui/Window.cpp
    src/gui/WindowManager.cpp
    src/gui/workspace/PanelTypes.cpp
    src/gui/workspace/PluginsPanel.cpp
    src/gui/Workspace.cpp
    src/gui/WorkspacesContentComponent.cpp
    src/Messages.cpp
    src/plugins/PluginEditor.cpp
    src/plugins/PluginProcessor.cpp
    src/scripting/DSPScript.cpp
    src/scripting/DSPUIScript.cpp
    src/scripting/JuceBindings.cpp
    src/scripting/LuaBindings.cpp
    src/scripting/LuaLib.cpp
    src/scripting/Script.cpp
    src/scripting/ScriptDescription.cpp
    src/scripting/ScriptingEngine.cpp
    src/scripting/ScriptManager.cpp
    src/session/Asset.cpp
    src/session/AssetTree.cpp
    src/session/ControllerDevice.cpp
    src/session/DeviceManager.cpp
    src/session/Graph.cpp
    src/session/MediaManager.cpp
    src/session/MidiClip.cpp
    src/session/Node.cpp
    src/session/Note.cpp
    src/session/NoteSequence.cpp
    src/session/PluginManager.cpp
    src/session/Sequence.cpp
    src/session/Session.cpp
    src/session/SessionTrack.cpp
    src/Settings.cpp
    src/Utils.cpp
    src/Version.cpp
    libs/compat/BinaryData.cpp
'''.split()

libelement_includes =  [ 
    include_directories (
        'libs/sol',
        'include',
        'src',
        'src/el'       
    ),
    lua_includes, 
    juce_includes, 
    vst3_includes, 
    vst2_includes, 
    lv2_includes 
]

if host_machine.system() == 'windows'
    libelement_sources += [ 'src/dlfcn-win32.c' ]
endif

modules_install_dir = get_option('datadir') / 'element/lua/el'
if host_machine.system() == 'windows'
    modules_install_dir = get_option('bindir') / 'lua/el'
elif host_machine.system() == 'darwin'
    modules_install_dir = 'Contents/Resources/Modules/el'
endif

install_data (
    sources : [
        'src/el/AudioBuffer.lua',
        'src/el/command.lua',
        'src/el/object.lua',
        'src/el/script.lua',
        'src/el/slug.lua'
    ], 
    install_dir : modules_install_dir
)

libelement = static_library ('element', 
    libelement_sources, 
    include_directories : libelement_includes,
    gnu_symbol_visibility : 'hidden',
    dependencies : deps,
    install : false,
    objects : [ libelement_lua.extract_all_objects (recursive : false),
                libelement_juce.extract_all_objects (recursive : false) ])

# Element Program
element_app = executable ('element', 'src/Main.cc',
    install : true,
    win_subsystem : 'windows',
    dependencies : deps,
    include_directories : libelement_includes,
    gnu_symbol_visibility : 'hidden',
    link_with : [ libelement ])

if host_machine.system() == 'darwin'
    install_data ('data/Icon.icns', install_dir : 'Contents/Resources')
    install_data ('tools/macdeploy/Info.plist', install_dir : 'Contents')
    meson.add_install_script ('tools/macdeploy/appbundle.py', 
        'tmp/Element.app', 
        '-volname', 'Element',
        '-dmg', 'element-osx-@0@'.format (meson.project_version()), 
        '-verbose', '0', 
        '-distdir', 'dist')
endif

project ('element', ['c', 'cpp'],
    version : '0.47.0',
    meson_version : '>= 0.56.0',
    default_options : [ 'c_std=c11', 'cpp_std=c++17' ])

fs = import ('fs')
cpp = meson.get_compiler ('cpp')

# Global Arguments
add_global_arguments ([], language : 'cpp')

# Boost
boost_dep = dependency ('boost', required : true)

# Lua
libelement_lua = static_library ('lua', '''
    libs/element/lua/src/lauxlib.c
    libs/element/lua/src/liolib.c
    libs/element/lua/src/lopcodes.c
    libs/element/lua/src/lstate.c
    libs/element/lua/src/lobject.c
    libs/element/lua/src/lmathlib.c
    libs/element/lua/src/loadlib.c
    libs/element/lua/src/lvm.c
    libs/element/lua/src/lfunc.c
    libs/element/lua/src/lstrlib.c
    libs/element/lua/src/linit.c
    libs/element/lua/src/lstring.c
    libs/element/lua/src/lundump.c
    libs/element/lua/src/lctype.c
    libs/element/lua/src/ltable.c
    libs/element/lua/src/ldump.c
    libs/element/lua/src/loslib.c
    libs/element/lua/src/lgc.c
    libs/element/lua/src/lzio.c
    libs/element/lua/src/ldblib.c
    libs/element/lua/src/lutf8lib.c
    libs/element/lua/src/lmem.c
    libs/element/lua/src/lcorolib.c
    libs/element/lua/src/lcode.c
    libs/element/lua/src/ltablib.c
    libs/element/lua/src/lapi.c
    libs/element/lua/src/lbaselib.c
    libs/element/lua/src/ldebug.c
    libs/element/lua/src/lparser.c
    libs/element/lua/src/llex.c
    libs/element/lua/src/ltm.c
    libs/element/lua/src/ldo.c
    '''.split(),
    install : false,
    gnu_symbol_visibility : 'default')

# JUCE
juce_modules = '''
    juce_audio_basics 
    juce_audio_devices 
    juce_audio_formats
    juce_audio_processors 
    juce_audio_utils 
    juce_core 
    juce_cryptography
    juce_data_structures 
    juce_dsp juce_events 
    juce_graphics 
    juce_gui_basics
    juce_gui_extra 
    juce_osc
    kv_core 
    kv_engines 
    kv_gui 
    kv_models
'''.split()

juce_conf_data = configuration_data()

juce_ext = host_machine.system() == 'darwin' ? 'mm' : 'cpp'
juce_sources = []
foreach m : juce_modules
    pat = 'libs/compat/include_@0@'.format (m)
    pat = pat + '.@0@'
    juce_file = pat.format (juce_ext)
    if not fs.exists (juce_file)
        juce_file = pat.format ('cpp')
    endif
    juce_sources += juce_file
    juce_conf_data.set10 ('JUCE_MODULE_AVAILABLE_@0@'.format (m), true)
endforeach

juce_conf_data.set10 ('JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED', true)
juce_conf_data.set10 ('JUCE_STANDALONE_APPLICATION', true)
juce_conf_data.set10 ('JUCE_DISPLAY_SPLASH_SCREEN', false)
juce_conf_data.set10 ('JUCE_MODAL_LOOPS_PERMITTED', true)
juce_conf_data.set10 ('JUCE_WEB_BROWSER', false)
juce_conf_data.set10 ('JUCE_ASIO', false)

juce_conf_data.set10 ('KV_DOCKING_WINDOWS', true)

vst2_includes = []
if get_option ('vst2sdk') == 'auto'
    # noop
else
    vst2_includes += include_directories (get_option ('vst2sdk'))
endif

aeffect_h_found = cpp.check_header ('pluginterfaces/vst2.x/aeffect.h',
    include_directories : vst2_includes, required : false)
aeffectx_h_found = cpp.check_header ('pluginterfaces/vst2.x/aeffectx.h',
    include_directories : vst2_includes, required : false)
vstfxstore_h_found = cpp.check_header ('pluginterfaces/vst2.x/vstfxstore.h',
    include_directories : vst2_includes, required : false)
juce_conf_data.set10 ('JUCE_PLUGINHOST_VST', aeffect_h_found and aeffectx_h_found)

vst3_includes = include_directories ('libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK')
juce_conf_data.set10 ('JUCE_VST3_CAN_REPLACE_VST2', vstfxstore_h_found)
juce_conf_data.set10 ('JUCE_PLUGINHOST_VST3', true)
lv2_includes = include_directories (
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lv2',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/serd',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sord',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sord/src',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lilv',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lilv/src',
    'libs/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/sratom')
juce_sources += 'libs/compat/include_juce_audio_processors_lv2_libs.cpp'
juce_conf_data.set10 ('JUCE_PLUGINHOST_LV2', true)
juce_conf_data.set10 ('JUCE_PLUGINHOST_LADSPA', cpp.check_header ('ladspa.h'))

deps = [ boost_dep ]
if host_machine.system() == 'linux'
    alsa_dep = dependency ('alsa', required : false)
    juce_conf_data.set10 ('JUCE_ALSA', alsa_dep.found())
    deps += alsa_dep

    deps += [ 
        dependency ('freetype2', required : true),
        dependency ('x11', required : true),
        dependency ('xext', required : true),
        dependency ('xrandr', required : true),
        dependency ('xcomposite', required : true),
        dependency ('xinerama', required : true),
        dependency ('xcursor', required : true)
    ]
    juce_conf_data.set10 ('JUCE_USE_XRANDR', true)
    juce_conf_data.set10 ('JUCE_USE_XINERAMA', true)
    juce_conf_data.set10 ('JUCE_USE_XSHM', true)
    juce_conf_data.set10 ('JUCE_USE_XRENDER', true)
    juce_conf_data.set10 ('JUCE_USE_XCURSOR', true)
    
    curl_dep = dependency ('libcurl', required : false)
    juce_conf_data.set10 ('JUCE_USE_CURL', curl_dep.found())
    deps += curl_dep

    deps += [
        cpp.find_library ('dl', required : true),
        cpp.find_library ('pthread', required : true) 
    ]
endif

configure_file (output : 'juce_config.h', 
                configuration : juce_conf_data)

# libelement
libelement_sources = '''
    libs/element/src/bindings.cpp
    libs/element/src/context.cpp
    libs/element/src/module.cpp
    libs/element/src/scripting.cpp
    libs/element/src/strings.cpp
    libs/element/lua/el/audio.c
    libs/element/lua/el/AudioBuffer32.cpp
    libs/element/lua/el/AudioBuffer64.cpp
    libs/element/lua/el/Bounds.cpp
    libs/element/lua/el/bytes.c
    libs/element/lua/el/CommandManager.cpp
    libs/element/lua/el/Desktop.cpp
    libs/element/lua/el/DocumentWindow.cpp
    libs/element/lua/el/File.cpp
    libs/element/lua/el/Globals.cpp
    libs/element/lua/el/Graphics.cpp
    libs/element/lua/el/midi.c
    libs/element/lua/el/MidiBuffer.cpp
    libs/element/lua/el/MidiMessage.cpp
    libs/element/lua/el/MouseEvent.cpp
    libs/element/lua/el/Node.cpp
    libs/element/lua/el/Parameter.cpp
    libs/element/lua/el/Point.cpp
    libs/element/lua/el/Range.cpp
    libs/element/lua/el/Rectangle.cpp
    libs/element/lua/el/round.c
    libs/element/lua/el/Session.cpp
    libs/element/lua/el/Slider.cpp
    libs/element/lua/el/TextButton.cpp
    libs/element/lua/el/Widget.cpp
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
libelement_sources += juce_sources

libelement_includes = '''
    libs/compat
    libs/element/include
    libs/element/lua
    libs/element/lua/src
    libs/element/lua/el
    libs/element/src
    libs/JUCE/modules
    libs/kv/modules
    src
    '''.split()
libelement_includes += vst3_includes
libelement_includes += vst2_includes
libelement_includes += lv2_includes

if host_machine.system() == 'windows'
    libelement_sources += [ 'libs/element/src/dlfcn-win32.c' ]
endif

libelement = static_library ('element', 
    libelement_sources, 
    include_directories : libelement_includes,
    gnu_symbol_visibility : 'hidden',
    dependencies : deps,
    install : false)

# Element Program
element_app = executable ('element', 'src/Main.cc',
    win_subsystem : 'windows',
    dependencies : deps,
    include_directories : libelement_includes,
    link_with : [ libelement, libelement_lua ])

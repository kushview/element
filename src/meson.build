
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

    nodes/audiofileplayer.cpp
    nodes/audiomixer.cpp
    nodes/audioprocessornode.cpp
    nodes/audiorouter.cpp
    nodes/audioroutereditor.cpp
    nodes/compressor.cpp
    nodes/compressoreditor.cpp
    nodes/eqfilter.cpp
    nodes/eqfiltereditor.cpp
    nodes/genericeditor.cpp
    nodes/knobs.cpp
    nodes/mediaplayer.cpp
    nodes/midichannelsplitter.cpp
    nodes/mididevice.cpp
    nodes/midifilter.cpp
    nodes/midimonitor.cpp
    nodes/midimonitoreditor.cpp
    nodes/midiprogrammap.cpp
    nodes/midiprogrammapeditor.cpp
    nodes/midirouter.cpp
    nodes/midiroutereditor.cpp
    nodes/oscreceiver.cpp
    nodes/oscreceivereditor.cpp
    nodes/oscsender.cpp
    nodes/oscsendereditor.cpp
    nodes/scriptnode.cpp
    nodes/scriptnodeeditor.cpp
    nodes/volumeeditor.cpp

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
    
    scripting/dspscript.cpp
    scripting/dspuiscript.cpp
    scripting/bindings.cpp
    scripting/scriptloader.cpp
    scripting/scriptmanager.cpp
    
    session/devicemanager.cpp
    session/pluginmanager.cpp
    session/session.cpp

    ui/aboutscreen.cpp
    ui/audiodeviceselector.cpp
    ui/audioiopanelview.cpp
    ui/block.cpp
    ui/buttons.cpp
    ui/capslock.cpp
    ui/channelstrip.cpp
    ui/connectiongrid.cpp
    ui/console.cpp
    ui/content.cpp
    ui/contextmenus.cpp
    ui/controllermapsview.cpp
    ui/controllersview.cpp
    ui/decibelscale.cpp
    ui/designer.cpp
    ui/filecombobox.cpp
    ui/filetreeview.cpp
    ui/graphnodeeditor.cpp
    ui/grapheditorcomponent.cpp
    ui/grapheditorview.cpp
    ui/graphmixerview.cpp
    ui/graphsettingsview.cpp
    ui/horizontallistbox.cpp
    ui/icons.cpp
    ui/keymapeditorview.cpp
    ui/luaconsole.cpp
    ui/luaconsoleview.cpp
    ui/luatokenizer.cpp
    ui/mainmenu.cpp
    ui/mainwindow.cpp
    ui/meterbridge.cpp
    ui/midiblinker.cpp
    ui/navigation.cpp
    ui/navigationview.cpp
    ui/nodechannelstripview.cpp
    ui/nodeeditor.cpp
    ui/nodeeditorfactory.cpp
    ui/nodeeditorview.cpp
    ui/nodeioconfiguration.cpp
    ui/nodemidiprogramcomponent.cpp
    ui/nodeportstable.cpp
    ui/nodeproperties.cpp
    ui/nodepropertiesview.cpp
    ui/nodepropertypanel.cpp
    ui/patchmatrix.cpp
    ui/pluginmanagercomponent.cpp
    ui/pluginspanelview.cpp
    ui/pluginwindow.cpp
    ui/preferences.cpp
    ui/rackview.cpp
    ui/scripteditorview.cpp
    ui/scriptview.cpp
    ui/sessiondocument.cpp
    ui/sessionimportwizard.cpp
    ui/sessionsettingsview.cpp
    ui/sessiontreepanel.cpp
    ui/sessiontreeview.cpp
    ui/simplemeter.cpp
    ui/standard.cpp
    ui/style_v1.cpp
    ui/systemtray.cpp
    ui/transportbar.cpp
    ui/treeviewbase.cpp
    ui/updater.cpp
    ui/viewhelpers.cpp
    ui/virtualkeyboardview.cpp
    ui/window.cpp
    ui/windowmanager.cpp

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
    libelement_sources += [ 'lv2/platform.cpp' ]
    absdatadir = get_option('prefix') / get_option('datadir')
    host_cpp_args += [
        '-DEL_LUADIR="@0@"'.format (absdatadir / 'element' / 'lua'),
        '-DEL_SCRIPTSDIR="@0@"'.format (absdatadir / 'element' / 'scripts')
    ]
elif host_machine.system() == 'windows'
    libelement_sources += [ 'lv2/platform.cpp' ]
    libelement_sources += [ 'dlfcn-win32.c' ]
elif host_machine.system() == 'darwin'
    libelement_sources += [ 'lv2/platform.mm' ]
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

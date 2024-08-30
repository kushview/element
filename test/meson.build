test_element_sources = '''
    atomtests.cpp
    datapathtests.cpp
    GraphNodeTests.cpp  
    NodeFactoryTests.cpp  
    OversamplerTests.cpp    
    PortListTests.cpp   
    TestMain.cpp
    IONodeTests.cpp     
    NodeObjectTests.cpp   
    PluginManagerTests.cpp  
    RootGraphTests.cpp
    NodeTests.cpp
    MidiProgramMapTests.cpp
    shuttletests.cpp

    engine/VelocityCurveTest.cpp
    engine/MidiChannelMapTest.cpp
    engine/togglegridtest.cpp
    engine/LinearFadeTest.cpp
    
    scripting/dspscripttest.cpp
    scripting/scriptinfotest.cpp
    scripting/scriptloadertest.cpp
    scripting/scriptmanagertest.cpp
    scripting/scriptplayground.cpp
    scripting/bytestest.cpp

    updatetests.cpp
    porttypetests.cpp
'''.split()
test_element_cpp_args = [
    '-DEL_TEST_SOURCE_ROOT="@0@"'.format (meson.project_source_root())
]

test_element_link_args = element_app_link_args

if cpp.get_argument_syntax() == 'gcc'
    test_element_cpp_args += [ '-Wno-unused-function', '-Wno-unused-variable' ]
endif

test_element_app = executable ('test_element', 
    test_element_sources,
    include_directories : [ '.' ],
    dependencies : [ element_app_deps, juce_dep, element_dep ],
    link_with : [],
    gnu_symbol_visibility : 'hidden',
    cpp_args : [ test_element_cpp_args ],
    link_args : [ test_element_link_args ],
    install : false
)

test ('Atoms',          test_element_app, args: [ '-t', 'AtomTests' ])
test ('DataPath',       test_element_app, args: [ '-t', 'DataPathTests' ])
test ('GraphNode',      test_element_app, args: [ '-t', 'GraphNodeTests' ])
test ('RootGraph',      test_element_app, args: [ '-t', 'RootGraphTests' ])
test ('IONode',         test_element_app, args: [ '-t', 'IONodeTests' ])

test ('NodeFactory',    test_element_app, args: [ '-t', 'NodeFactoryTests' ])
test ('Oversampler',    test_element_app, args: [ '-t', 'OversamplerTests' ])
test ('PortList',       test_element_app, args: [ '-t', 'PortListTests' ])
test ('PortType',       test_element_app, args: [ '-t', 'PortTypeTests' ])
test ('PluginManager',  test_element_app, args: [ '-t', 'PluginManagerTests' ])
test ('Updates',        test_element_app, args: [ '-t', 'UpdateTests' ])

test ('Node',           test_element_app, args: [ '-t', 'NodeTests' ], suite: 'model')

test ('LinearFade',     test_element_app, args: [ '-t', 'LinearFadeTest'],      suite: 'engine' )
test ('MidiChannelMap', test_element_app, args: [ '-t', 'MidiChannelMapTest'],  suite: 'engine' )
test ('MidiProgramMap', test_element_app, args: [ '-t', 'MidiProgramMapTests'], suite: 'engine' )
test ('Processor',      test_element_app, args: [ '-t', 'NodeObjectTests' ],    suite: 'engine')
test ('Shuttle',        test_element_app, args: [ '-t', 'ShuttleTests' ],       suite: 'engine')
test ('ToggleGrid',     test_element_app, args: [ '-t', 'ToggleGridTest'],      suite: 'engine' )
test ('VelocityCurve',  test_element_app, args: [ '-t', 'VelocityCurveTest'],   suite: 'engine' )

test ('Bytes',          test_element_app, args: [ '-t', 'BytesTest' ],          suite: 'lua')
test ('DSPScript',      test_element_app, args: [ '-t', 'DSPScriptTest' ],      suite: 'lua')
test ('ScriptInfo',     test_element_app, args: [ '-t', 'ScriptInfoTest' ],     suite: 'lua')
test ('ScriptManager',  test_element_app, args: [ '-t', 'ScriptManagerTest' ],  suite: 'lua')
test ('ScriptLoader',   test_element_app, args: [ '-t', 'ScriptLoaderTest' ],   suite: 'lua')
test ('ScriptPlayground', test_element_app, args: [ '-t', 'ScriptPlayground' ], suite: 'lua')

test_element_sources = '''
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

    engine/VelocityCurveTest.cpp
    engine/MidiChannelMapTest.cpp
    engine/ToggleGridTests.cpp
    engine/LinearFadeTest.cpp
    
    scripting/ScriptDescriptionTests.cpp
    scripting/ScriptManagerTests.cpp
    updatetests.cpp
'''.split()
test_element_cpp_args = [
    '-DEL_TEST_SOURCE_ROOT="@0@"'.format (meson.project_source_root())
]

if cpp.get_argument_syntax() == 'gcc'
    test_element_cpp_args += [ '-Wno-unused-function', '-Wno-unused-variable' ]
endif

test_element_app = executable ('test_element', 
    test_element_sources,
    include_directories : [ '.', libelement_includes ],
    dependencies : [ element_app_deps, juce_dep ],
    link_with : [ libelement ],
    gnu_symbol_visibility : 'hidden',
    cpp_args : test_element_cpp_args,
    install : false
)

test ('GraphNode',      test_element_app, args : [ '-t', 'GraphNodeTests' ])
test ('RootGraph',      test_element_app, args : [ '-t', 'RootGraphTests' ])
test ('IONode',         test_element_app, args : [ '-t', 'IONodeTests' ])

test ('NodeFactory',    test_element_app, args : [ '-t', 'NodeFactoryTests' ])
test ('Oversampler',    test_element_app, args : [ '-t', 'OversamplerTests' ])
test ('PortList',       test_element_app, args : [ '-t', 'PortListTests' ])
test ('Processor',     test_element_app, args : [ '-t', 'NodeObjectTests' ])
test ('PluginManager',  test_element_app, args : [ '-t', 'PluginManagerTests' ])
test ('Updates',        test_element_app, args : [ '-t', 'UpdateTests' ])

test ('Node',           test_element_app, args : [ '-t', 'NodeTests' ], suite: 'model')

test ('LinearFade',     test_element_app, args : [ '-t', 'LinearFadeTest'], suite: 'engine' )
test ('MidiChannelMap', test_element_app, args : [ '-t', 'MidiChannelMapTest'], suite: 'engine' )
test ('MidiProgramMap', test_element_app, args : [ '-t', 'MidiProgramMapTests'], suite: 'engine' )
test ('Processor',      test_element_app, args : [ '-t',  'NodeObjectTests' ], suite : 'engine')
test ('ToggleGrid',     test_element_app, args : [ '-t', 'ToggleGridTests'], suite: 'engine' )

test ('VelocityCurve',  test_element_app, args : [ '-t', 'VelocityCurveTest'], suite: 'engine' )

test ('ScriptDescription', test_element_app, args : [ '-t', 'ScriptDescriptionTests' ], suite : 'scripting')
test ('ScriptManager',  test_element_app,    args : [ '-t', 'ScriptManagerTests' ], suite : 'scripting')

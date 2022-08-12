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

    scripting/ScriptDescriptionTests.cpp
    scripting/ScriptManagerTests.cpp
'''.split()
test_element_cpp_args = []

if cpp.get_argument_syntax() == 'gcc'
    test_element_cpp_args += [ '-Wno-unused-function', '-Wno-unused-variable' ]
endif

test_element_app = executable ('test_element', 
    test_element_sources,
    include_directories : [ '.', libelement_includes ],
    dependencies : element_app_deps,
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
test ('NodeObject',     test_element_app, args : [ '-t', 'NodeObjectTests' ])
test ('PluginManager',  test_element_app, args : [ '-t', 'PluginManagerTests' ])

test ('ScriptDescription', test_element_app, args : [ '-t', 'ScriptDescriptionTests' ],
    suite : 'scripting')
test ('ScriptManager', test_element_app, args : [ '-t', 'ScriptManagerTests' ],
    suite : 'scripting')

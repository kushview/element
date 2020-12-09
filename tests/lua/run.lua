package.cpath = "./build/lib/lua/?.so;"..package.cpath
package.path  = "./tests/lua/?.lua;"..package.path
package.path =  "./libs/?.lua;"..package.path

luaunit = require ('lua-kv/tests/luaunit')

local tests = {
    'lua-kv/tests/test_byte',
    'lua-kv/tests/test_midi',
    'lua-kv/tests/test_object',
    'lua-kv/tests/TestAudioBuffer',
    'lua-kv/tests/TestBounds',
    'lua-kv/tests/TestMidiBuffer',
    'lua-kv/tests/TestMidiMessage',
    'lua-kv/tests/TestPoint'
}
for _,t in ipairs (tests) do 
    require (t)
end

local exit_code = luaunit.LuaUnit.run (...)
collectgarbage()
os.exit (exit_code)

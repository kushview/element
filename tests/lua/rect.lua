
local kv = require ('kv')
local el = element

function init()
end

function run()
    begintest ("rect")
    local r = el.Rect (0, 5, 100, 100)
    expect (r.w == 100, tostring (r))
    expect (r.h == 100, tostring (r))
    expect (r.x == 0, tostring (r))
    expect (r.y == 5, tostring (r))

    print (kv.dbtogain)
end

function shutdown()
end

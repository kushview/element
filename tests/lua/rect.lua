
function init()
end

function run()
    begintest ("rect")
    local r = kv.Rect (0, 5, 100, 100)
    expect(r.w == 100, tostring(r))
    expect (r.h == 100, tostring (r))
    expect (r.x == 0, tostring (r))
    expect (r.y == 5, tostring (r))
end

function shutdown()
end

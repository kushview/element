local bytes = require ('el.bytes')
local data = bytes.new (1024)

BOOST_REQUIRE (data ~= nil)
bytes.rawset (data, 3, 0xfd)
BOOST_REQUIRE (bytes.rawget (data, 3) == 0xfd)

return


-- bytes.set (data, 1, 0xff)
-- bytes.set (data, 2, 0xfe)


-- BOOST_REQUIRE (bytes.size (data) == 1024)
-- BOOST_REQUIRE (bytes.get (data, 1) == 0xff)
-- BOOST_REQUIRE (bytes.get (data, 2) == bytes.rawget (data, 2))
-- BOOST_REQUIRE (bytes.get (data, 3) ~= bytes.rawget (data, 2))
-- BOOST_REQUIRE (bytes.get (data, 2) ~= bytes.rawget (data, 1))

-- bytes.free (data)

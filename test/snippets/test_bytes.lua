-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

local bytes = require ('el.bytes')
local data = bytes.new (1024)

BOOST_REQUIRE (data ~= nil)

bytes.set (data, 1, 0xff)
bytes.set (data, 2, 0xfe)
bytes.set (data, 3, 0xfd)

BOOST_REQUIRE (bytes.size (data) == 1024)
BOOST_REQUIRE (bytes.get (data, 1) == 0xff)
BOOST_REQUIRE (bytes.get (data, 2) == bytes.get (data, 2))
BOOST_REQUIRE (bytes.get (data, 3) ~= bytes.get (data, 2))
BOOST_REQUIRE (bytes.get (data, 2) ~= bytes.get (data, 1))

local raw = bytes.toraw (data)
bytes.rawset (raw, 1, 0x10)
bytes.rawset (raw, 2, 0x11)
bytes.rawset (raw, 3, 0x12)
BOOST_REQUIRE (bytes.rawget (raw, 1) == 0x10)
BOOST_REQUIRE (bytes.rawget (raw, 2) == bytes.get (data, 2))
BOOST_REQUIRE (bytes.rawget (raw, 3) ~= bytes.get (data, 2))
BOOST_REQUIRE (bytes.rawget (raw, 2) ~= bytes.get (data, 1))

bytes.free (data)

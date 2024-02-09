// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// Manage raw byte arrays.
// @author Michael Fisher
// @module el.bytes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lualib.h>

#include <element/element.h>
#include "bytes.h"
#include "packed.h"

void element_bytes_init (EL_Bytes* b, size_t size)
{
    b->data = NULL;
    b->size = size;
    if (size > 0)
    {
        b->data = (uint8_t*) malloc (size + 1);
        b->size = size;
        memset (b->data, 0, b->size);
    }
}

void element_bytes_free (EL_Bytes* b)
{
    b->size = 0;
    if (b->data != NULL)
    {
        free (b->data);
        b->data = NULL;
    }
}

uint8_t element_bytes_get (EL_Bytes* b, lua_Integer index)
{
    return b->data[index];
}

void element_bytes_set (EL_Bytes* b, lua_Integer index, uint8_t value)
{
    b->data[index] = value;
}

/// Create a new byte array.
// @function new
// @int size Size in bytes to allocate
// @treturn kv.ByteArray The new byte array.
static int f_new (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_newuserdata (L, sizeof (EL_Bytes));
    luaL_setmetatable (L, EL_MT_BYTE_ARRAY);
    size_t size = lua_isnumber (L, 1) ? (size_t) lua_tonumber (L, 1) : 0;
    element_bytes_init (b, size);
    return 1;
}

/// Free used memory.
// @function free
// @param bytes The array to free
static int f_free (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    element_bytes_free (b);
    return 0;
}

/// Get a byte from the array.
// @function get
// @param bytes Bytes to get from
// @int index Index in the array
static int f_get (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    lua_Integer index = luaL_checkinteger (L, 2);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    luaL_argcheck (L, index >= 1 && index <= b->size, 2, "index out of range");
    lua_pushinteger (L, (lua_Integer) element_bytes_get (b, index - 1));
    return 1;
}

/// Get a byte from the array.
// This is the same as bytes.get() but for speed does not check for valid
// argments.
// @function rawget
// @param bytes Bytes to get from
// @int index Index in the array
static int f_rawget (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    lua_pushinteger (L, (lua_Integer) b->data[lua_tointeger (L, 2) - 1]);
    return 1;
}

/// Set a byte in the array.
// @function set
// @param bytes Target bytes
// @int index Index in the array
// @int value Value to set in the range 0x00 to 0xFF inclusive
static int f_set (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    lua_Integer index = luaL_checkinteger (L, 2);
    lua_Integer value = luaL_checkinteger (L, 3);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    luaL_argcheck (L, index >= 1 && index <= b->size, 2, "index out of range");
    element_bytes_set (b, index - 1, (uint8_t) value);
    return 1;
}

/// Set a byte in the array.
// This is the same as bytes.set() but for speed does not check for valid
// arguments.
// @function set
// @param bytes Target bytes
// @int index Index in the array
// @int value Value to set in the range 0x00 to 0xFF inclusive
static int f_rawset (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    b->data[lua_tointeger (L, 2) - 1] = (uint8_t) lua_tointeger (L, 3);
    return 1;
}

/// Returns the size in bytes.
// @function size
// @param bytes Target bytes
// @treturn int The size in bytes.
static int f_size (lua_State* L)
{
    EL_Bytes* b = (EL_Bytes*) lua_touserdata (L, 1);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    lua_pushinteger (L, (lua_Integer) b->size);
    return 1;
}

/// Pack 4 bytes in a 64bit integer.
// Undefined params are treated as zero. This function does not check arguments
// and therefor can crash if client code passes in bad data.
// @function pack
// @int b1 First byte
// @int b2 Second byte
// @int b3 Third byte
// @int b4 Fourth byte
// @treturn int Packed lua_Integer
static int f_pack (lua_State* L)
{
    kv_packed_t msg = { .packed = 0x0 };

    switch (lua_gettop (L))
    {
        case 3:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = (uint8_t) lua_tointeger (L, 3);
            msg.data[3] = 0x00;
            break;

        case 0:
            break;

        case 1:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = msg.data[2] = msg.data[3] = 0x00;
            break;

        case 2:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = msg.data[3] = 0x00;
            break;

        case 4: // >= 4
        default:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = (uint8_t) lua_tointeger (L, 3);
            msg.data[3] = (uint8_t) lua_tointeger (L, 4);
            break;
    }

    lua_pushinteger (L, msg.packed);
    return 1;
}

static const luaL_Reg bytes_f[] = {
    { "new", f_new },

    { "free", f_free },
    { "size", f_size },

    { "get", f_get },
    { "rawget", f_rawget },

    { "set", f_set },
    { "rawset", f_rawset },

    { "pack", f_pack },
    { NULL, NULL }
};

static const luaL_Reg bytes_m[] = {
    { "__gc", f_free },
    { NULL, NULL }
};

EL_PLUGIN_EXPORT
int luaopen_el_bytes (lua_State* L)
{
    if (luaL_newmetatable (L, EL_MT_BYTE_ARRAY))
    {
        lua_pushvalue (L, -1); /* duplicate the metatable */
        lua_setfield (L, -2, "__index"); /* mt.__index = mt */
        luaL_setfuncs (L, bytes_m, 0);
        lua_pop (L, 1);
    }

    luaL_newlib (L, bytes_f);
    return 1;
}

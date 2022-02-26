/*
Copyright 2019-2020 Michael Fisher <mfisher@kushview.net>

Permission to use, copy, modify, and/or distribute this software for any 
purpose with or without fee is hereby granted, provided that the above 
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
PERFORMANCE OF THIS SOFTWARE.
*/

/// Manage raw byte arrays.
// @author Michael Fisher
// @module kv.bytes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lua-kv.h"
#include "bytes.h"
#include "packed.h"

void kv_bytes_init (kv_bytes_t* b, size_t size) {
    b->data = NULL;
    b->size = size;
    if (size > 0) {
        b->data = (uint8_t*) malloc (size + 1);
        b->size = size;
        memset (b->data, 0, b->size);
    }
}

void kv_bytes_free (kv_bytes_t* b) {
    b->size = 0;
    if (b->data != NULL) {
        free (b->data);
        b->data = NULL;
    }
}

uint8_t kv_bytes_get (kv_bytes_t* b, lua_Integer index) {
    return b->data [index];
}

void kv_bytes_set (kv_bytes_t* b, lua_Integer index, uint8_t value) {
    b->data[index] = value;
}

/// Create a new byte array.
// @function new
// @int size Size in bytes to allocate
// @treturn kv.ByteArray The new byte array.
static int f_new (lua_State* L) {
    kv_bytes_t* b = (kv_bytes_t*) lua_newuserdata (L, sizeof (kv_bytes_t));
    luaL_setmetatable (L, LKV_MT_BYTE_ARRAY);
    size_t size = lua_isnumber (L, 1) ? (size_t) lua_tonumber (L, 1) : 0;
    kv_bytes_init (b, size);
    return 1;
}

/// Free used memory.
// @function free
// @param bytes The array to free
static int f_free (lua_State* L) {
    kv_bytes_t* b = (kv_bytes_t*) lua_touserdata (L, 1);
    kv_bytes_free (b);
    return 0;
}

/// Get a byte from the array.
// @function get
// @param bytes Bytes to get from
// @int index Index in the array
static int f_get (lua_State* L) {
    kv_bytes_t* b = (kv_bytes_t*) lua_touserdata (L, 1);
    lua_Integer index = luaL_checkinteger (L, 2);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    luaL_argcheck (L, index >= 1 && index <= b->size, 2, "index out of range");
    lua_pushinteger (L, (lua_Integer) kv_bytes_get (b, index - 1));
    return 1;
}

/// Set a byte in the array.
// @function set
// @param bytes Target bytes
// @int index Index in the array
// @int value Value to set in the range 0x00 to 0xFF inclusive
static int f_set (lua_State* L) {
    kv_bytes_t* b = (kv_bytes_t*) lua_touserdata (L, 1);
    lua_Integer index = luaL_checkinteger (L, 2);
    lua_Integer value = luaL_checkinteger (L, 3);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    luaL_argcheck (L, index >= 1 && index <= b->size, 2, "index out of range");
    kv_bytes_set (b, index - 1, (uint8_t) value);
    return 1;
}

/// Returns the size in bytes.
// @function size
// @param bytes Target bytes
// @treturn int The size in bytes.
static int f_size (lua_State* L) {
    kv_bytes_t* b = (kv_bytes_t*) lua_touserdata (L, 1);
    luaL_argcheck (L, b != NULL, 1, "`bytes' expected");
    lua_pushinteger (L, (lua_Integer) b->size);
    return 1;
}

/// Pack 4 bytes in a 64bit integer.
// Undefined params are treated as zero
// @function pack
// @int b1 First byte
// @int b2 Second byte
// @int b3 Third byte
// @int b4 Fourth byte
// @treturn int Packed integer
static int f_pack (lua_State* L) {
    kv_packed_t msg = { .packed = 0x0 };

    switch (lua_gettop (L)) {
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
    { "new",    f_new },
    { "free",   f_free },
    { "size",   f_size },
    { "get",    f_get },
    { "set",    f_set },
    { "pack",   f_pack },
    { NULL, NULL }
};

static const luaL_Reg bytes_m[] = {
    { "__gc",   f_free },
    { NULL, NULL }
};

LKV_EXPORT
int luaopen_el_bytes (lua_State* L) {
    if (luaL_newmetatable (L, LKV_MT_BYTE_ARRAY)) {
        lua_pushvalue (L, -1);               /* duplicate the metatable */
        lua_setfield (L, -2, "__index");     /* mt.__index = mt */
        luaL_setfuncs (L, bytes_m, 0);
        lua_pop (L, 1);
    }

    luaL_newlib (L, bytes_f);
    return 1;
}

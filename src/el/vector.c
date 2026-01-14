// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#if 0

/// A vector of `kv_sample_t`'s suitable for realtime
// @module el.vector
#include <stdlib.h>
#include <string.h>
#include "luainc.h"
#include "vector.h"
#include "util.h"
#include "element/element.h"

struct kv_vector_impl_t {
    kv_sample_t*   values;
    lua_Integer     size;
    lua_Integer     used;
};

typedef struct kv_vector_impl_t        Vector;

//=============================================================================
kv_vector_t* kv_vector_new (lua_State* L, int size) {
    Vector* vec = lua_newuserdata (L, sizeof (Vector));
    luaL_setmetatable (L, EL_MT_VECTOR);

    if (size > 0) {
        vec->size = vec->used = size;
        vec->values = malloc (sizeof (kv_sample_t) * size);
        memset (vec->values, 0, sizeof (kv_sample_t) * size);
    } else {
        vec->size = vec->used   = 0;
        vec->values = NULL;
    }

    return vec;
}

static void kv_vector_free_values (kv_vector_t* vec) {
    if (vec->values != NULL) {
        free (vec->values);
        vec->values = NULL;
    }
    vec->size = vec->used = 0;
}

size_t kv_vector_size (kv_vector_t* vec) {
    return (size_t) vec->used;
}

size_t kv_vector_capacity (kv_vector_t* vec) {
    return (size_t) vec->size;
}

kv_sample_t* kv_vector_values (kv_vector_t* vec) {
    return vec->values;
}

kv_sample_t kv_vector_get (kv_vector_t* vec, int index) {
    return vec->values [index];
}

void kv_vector_set (kv_vector_t* vec, int index, kv_sample_t value) {
    vec->values [index] = value;
}

void kv_vector_clear (kv_vector_t* vec) {
    if (vec->used > 0) {
        memset (vec->values, 0, sizeof(kv_sample_t) * vec->used);
    }
}

void kv_vector_resize (kv_vector_t* vec, int size) {
    size = MAX (0, size);
    if (size <= vec->size) {
        vec->used = size;
    } else {
        vec->used = vec->size = size;
        vec->values = realloc (vec->values, sizeof(kv_sample_t) * vec->size);
    }
}

static int vector_gc (lua_State* L) {
    kv_vector_free_values (lua_touserdata (L, 1));
    return 0;
}

static int vector_len (lua_State* L) {
    Vector* vec = lua_touserdata (L, 1);
    lua_pushinteger (L, vec->size);
    return 1;
}

static int vector_index (lua_State* L) {
    Vector* vec = lua_touserdata (L, 1);
    lua_Integer i = lua_tointeger (L, 2) - 1;
    if (i >= 0 && i < vec->used) {
        lua_pushnumber (L, vec->values[i]);
    } else {
        lua_pushnil (L);
    }
    return 1;
}

static int vector_newindex (lua_State* L) {
    Vector* vec = lua_touserdata (L, 1);
    lua_Integer i = lua_tointeger (L, 2) - 1;
    lua_Number  v = lua_tonumber (L, 3);
    if (i >= 0 && i < vec->used) {
        vec->values[i] = v;
    }
    return 0;
}

static int vector_tostring (lua_State* L) {
    Vector* vec = lua_touserdata (L, 1);
    lua_pushfstring (L, "Vector: size=%d capacity=%d", vec->used, vec->size);
    return 1;
}

static const luaL_Reg vector_m[] = {
    { "__gc",       vector_gc },
    { "__len",      vector_len },
    { "__index",    vector_index },
    { "__newindex", vector_newindex },
    { "__tostring", vector_tostring },
    { NULL, NULL }
};

//=============================================================================

/// Creates a new vector
// @param size Number of elements
// @function new
static int f_new (lua_State* L) {
    if (NULL == kv_vector_new (L, MAX (0, lua_tointeger (L, 1))))
        lua_pushnil (L);
    return 1;
}

/// Clears a vector
// @param vec The vector to clear
// @function clear
static int f_clear (lua_State* L) {
    kv_vector_t* vec = luaL_checkudata (L, 1, EL_MT_VECTOR);
    if (vec->used <= 0 || vec->values == NULL) {
        return 0;
    }

    switch (lua_gettop (L)) {
        default: {
            memset (vec->values, 0, sizeof(kv_sample_t) * vec->used);
            break;
        }
    }
    
    return 0;
}

/// Reserve a number of elements
// @param vec The vector to operate on
// @param size Number of elements to allocate memory for
// @function reserve
static int f_reserve (lua_State* L) {
    return 0;
}

/// Resize a vector
// @param vec The vector to clear
// @param size The new number of elements to allocate
// @function resize
static int f_resize (lua_State* L) {
    return 0;
}

static const luaL_Reg vector_f[] = {
    { "new",        f_new },
    { "clear",      f_clear },
    { "reserve",    f_reserve },
    { "resize",     f_resize },
    { NULL, NULL }
};

void kv_vector_metatable (lua_State* L) {
    if (0 != luaL_newmetatable (L, EL_MT_VECTOR)) {
        luaL_setfuncs (L, vector_m, 0);
    }
}

EL_PLUGIN_EXPORT 
int luaopen_el_vector (lua_State* L) {
    kv_vector_metatable (L);
    luaL_newlib (L, vector_f);
    return 1;
}

#endif

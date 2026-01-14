// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EL_LUA_BYTES_H
#define EL_LUA_BYTES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EL_Bytes
{
    size_t size;
    uint8_t* data;
} EL_Bytes;

#ifdef __cplusplus
}
#endif

#endif

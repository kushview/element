// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#ifndef LKV_BYTES_H
#define LKV_BYTES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _kv_bytes_t
{
    size_t size;
    uint8_t* data;
} kv_bytes_t;

#ifdef __cplusplus
}
#endif

#endif

/*
Copyright 2019 Michael Fisher <mfisher@kushview.net>

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

#ifndef LKV_H
#define LKV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>

#if (LUA_VERSION_NUM < 503)
#pragma error "Lua KV requires Lua v5.3.5 or higher"
#pragma GCC error "Lua KV requires Lua v5.3.5 or higher"
#endif

#ifndef LKV_FORCE_FLOAT32
#define LKV_FORCE_FLOAT32 0
#endif

#if LKV_FORCE_FLOAT32
typedef float kv_sample_t;
#else
typedef lua_Number kv_sample_t;
#endif

typedef struct kv_audio_buffer_impl_t kv_audio_buffer_t;
typedef struct kv_midi_message_impl_t kv_midi_message_t;
typedef struct kv_midi_buffer_impl_t kv_midi_buffer_t;
typedef void* kv_midi_buffer_iter_t;
typedef struct kv_midi_pipe_impl_t kv_midi_pipe_t;
typedef struct kv_vector_impl_t kv_vector_t;

/** Creates a new vector leaving it on the stack */
kv_vector_t* kv_vector_new (lua_State*, int);

/** Returns the number of elements used by the vector */
size_t kv_vector_size (kv_vector_t*);

/** Returns the total number of elements allocated.
    This is NOT the number of elements currently used.
    see `kv_vector_size`
*/
size_t kv_vector_capacity (kv_vector_t*);

/** Clears the vector */
void kv_vector_clear (kv_vector_t*);

/** Returns a pointer to value array */
kv_sample_t* kv_vector_values (kv_vector_t*);

/** Returns a value from the vector */
kv_sample_t kv_vector_get (kv_vector_t*, int);

/** Sets a values in the vector */
void kv_vector_set (kv_vector_t*, int, kv_sample_t);

/** Resizes the vector.  Does not allocate memory if the new size
    is less than the total capacity. see `kv_vector_capacity`
*/
void kv_vector_resize (kv_vector_t*, int);

//=============================================================================
/** Adds a new audio buffer to the lua stack
    @param L                The lua state
    @param num_channels     Total audio channels
    @param num_frames       Number of samples in each channel
*/
kv_audio_buffer_t* kv_audio_buffer_new (lua_State* L,
                                        int num_channels,
                                        int num_frames);

/** Refer the given buffer to a set of external audio channels
    @param buffer       The audio buffer
    @param data         External data to refer to
    @param nchannels    Number of channels is external data
    @param nframes      Number of samples in each external data channel
*/
void kv_audio_buffer_refer_to (kv_audio_buffer_t* buffer,
                               kv_sample_t* const* data,
                               int nchannels,
                               int nframes);

/** Returs this buffer's channel count */
int kv_audio_buffer_channels (kv_audio_buffer_t*);

/** Returns this buffer's length in samples */
int kv_audio_buffer_length (kv_audio_buffer_t*);

/** Returns an array of channels. DO NOT keep a reference to this. */
kv_sample_t** kv_audio_buffer_array (kv_audio_buffer_t*);

/** Returns a single channel of samples */
kv_sample_t* kv_audio_buffer_channel (kv_audio_buffer_t*, int channel);

/** Resize this buffer.
    @param buffer       Buffer to resize
    @param nchannels    New channel count
    @param nframes      New sample count
    @param preserve     Keep existing content if possible
    @param clear        Clear extra space
    @param norealloc    Avoid re-allocating if possible
*/
void kv_audio_buffer_resize (kv_audio_buffer_t* buffer,
                             int nchannels,
                             int nframes,
                             bool preserve,
                             bool clear,
                             bool norealloc);

void kv_audio_buffer_duplicate (kv_audio_buffer_t* buffer,
                                const kv_sample_t* const* source,
                                int nchannels,
                                int nframes);

void kv_audio_buffer_duplicate_32 (kv_audio_buffer_t* buffer,
                                   const float* const* source,
                                   int nchannels,
                                   int nframes);

//=============================================================================
kv_midi_message_t* kv_midi_message_new (lua_State* L, uint8_t* data, size_t size, bool push);
void kv_midi_message_free (kv_midi_message_t* msg);
uint8_t* kv_midi_message_data (kv_midi_message_t* msg);
void kv_midi_message_reset (kv_midi_message_t* msg);
void kv_midi_message_update (kv_midi_message_t* msg,
                             uint8_t* data,
                             lua_Integer size);

//=============================================================================
/** Adds a new midi buffer to the stack */
kv_midi_buffer_t* kv_midi_buffer_new (lua_State* L, size_t size);

/** Free the buffer */
void kv_midi_buffer_free (kv_midi_buffer_t* buf);

size_t kv_midi_buffer_capacity (kv_midi_buffer_t* buf);
void kv_midi_buffer_swap (kv_midi_buffer_t* a, kv_midi_buffer_t* b);

/** Clears the buffer */
void kv_midi_buffer_clear (kv_midi_buffer_t*);

/** Inserts some MIDI data in the buffer */
void kv_midi_buffer_insert (kv_midi_buffer_t* buf, const uint8_t* bytes, size_t len, int frame);

/** Returns the start iterator
    Do not modify the retured iterator in any way
*/
kv_midi_buffer_iter_t kv_midi_buffer_begin (kv_midi_buffer_t*);

/** Returns the end iterator
    Do not modify the retured iterator in any way
*/
kv_midi_buffer_iter_t kv_midi_buffer_end (kv_midi_buffer_t*);

/** Returns the next event iterator
    Do not modify the retured iterator in any way
*/
kv_midi_buffer_iter_t kv_midi_buffer_next (kv_midi_buffer_t*, kv_midi_buffer_iter_t);

/** Loop through all midi events */
#define kv_midi_buffer_foreach(b, i)                             \
    for (kv_midi_buffer_iter_t (i) = kv_midi_buffer_begin ((b)); \
         i < kv_midi_buffer_end ((b));                           \
         i = kv_midi_buffer_next ((b), (i)))

/** Returns the total size in bytes of the iterator */
#define kv_midi_buffer_iter_total_size(i) (lua_Integer) (sizeof (int32_t) + sizeof (uint16_t) + *(uint16_t*) ((i) + sizeof (int32_t)))

/** Returns the frame index of this event */
#define kv_midi_buffer_iter_frame(i) *(int32_t*) i

/** Returns the data size of this event */
#define kv_midi_buffer_iter_size(i) (lua_Integer) (*(uint16_t*) ((uint8_t*) i + sizeof (int32_t)))

/** Returns the raw MIDI data of this event
    Do not modify in any way
*/
#define kv_midi_buffer_iter_data(i) (uint8_t*) i + (sizeof (int32_t) + sizeof (uint16_t))

//=============================================================================
/** Create a new midi pipe on the stack */
kv_midi_pipe_t* kv_midi_pipe_new (lua_State* L, int nbuffers);

void kv_midi_pipe_free (lua_State* L, kv_midi_pipe_t* pipe);
int kv_midi_pipe_size (kv_midi_pipe_t*);

/** Clear buffers in the pipe */
void kv_midi_pipe_clear (kv_midi_pipe_t*, int);

/** Change the number of buffers contained */
void kv_midi_pipe_resize (lua_State* L, kv_midi_pipe_t*, int);

/** Returns a buffer from the list */
kv_midi_buffer_t* kv_midi_pipe_get (kv_midi_pipe_t*, int);

//=============================================================================
/** Open all libraries
    @param L    The Lua state
    @param glb  Set 1 to assign a global variable for each library
*/
void kv_openlibs (lua_State* L, int glb);

#ifdef __cplusplus
}
#endif
#endif

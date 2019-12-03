
#include <stdbool.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

typedef float audio_sample_t;

typedef struct _AudioBuffer {
    int nframes;
    int nchannels;
    size_t size;
    char* data;
    float** channels;
    float* prealloc [32];
    bool cleared;
} AudioBuffer;

//=============================================================================
static void audiobuffer_init (AudioBuffer* buf) {
    buf->nframes = buf->nchannels = 0;
    buf->size = 0;
    buf->data = NULL;
    memset (buf->channels, 0, sizeof(float*) * 32);
    buf->channels = (float**) buf->prealloc;
    buf->cleared = false;
}

static int audiobuffer_new (lua_State* L) {
    AudioBuffer* buf = lua_newuserdata (L, sizeof (AudioBuffer));
    audiobuffer_init (buf);
    return 1;
}

//=============================================================================
static void audiobuffer_allocate_data (AudioBuffer* buf) {
    size_t list_size = (size_t)(buf->nchannels + 1) * sizeof(float*);
    size_t required_alignment = (size_t) alignof (float);
    size_t alignment_overflow = list_size % required_alignment;
    if (alignment_overflow > 0)
        list_size += alignment_overflow - required_alignment;

    buf->size = (size_t)buf->nchannels * (size_t)buf->nframes * sizeof(float) + list_size + 32;
    buf->data = malloc (buf->size);
    buf->channels = (float**) buf->data;
    float* chan = (float*)(buf->data + list_size);

    int i;
    for (i = 0; i < buf->nchannels; ++i) {
        buf->channels[i] = chan;
        chan += buf->nframes;
    }

    buf->channels[buf->nchannels] = NULL;
    buf->cleared = false;
}

static void audiobuffer_allocate_channels (AudioBuffer* buf) {
}

//=============================================================================
static void audiobuffer_clear_all (AudioBuffer* buf) {
    if (buf->cleared)
        return;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c], 0, sizeof(float) * (size_t)buf->nframes);
    buf->cleared = true;
}

static void audiobuffer_clear_range (AudioBuffer* buf, int start, int count) {
    if (buf->cleared)
        return;
    if (start == 0 && count == buf->nframes)
        buf->cleared = true;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c] + start, 0, sizeof(float) * (size_t)buf->nframes);
    buf->cleared = true;
}

static void audiobuffer_clear_channel (AudioBuffer* buf, int channel, int start, int count) {
    if (buf->cleared)
        return;
    memset (buf->channels[channel] + start, 0, sizeof(float) * count);
}

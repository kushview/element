// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later
#include <clap/clap.h>
#include <stdlib.h>
#include <string.h>

static const char *features[] = { CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, NULL };
static const clap_plugin_descriptor_t descriptor = {
    CLAP_VERSION, "net.kushview.test.transport", "Transport Probe", "Kushview",
    "", "", "", "1", "Reports transport presence in its output", features
};
static bool CLAP_ABI init(const clap_plugin_t *p) { return true; }
static void CLAP_ABI destroy(const clap_plugin_t *p) { free((void *)p); }
static bool CLAP_ABI activate(const clap_plugin_t *p, double rate, uint32_t min, uint32_t max) { return true; }
static void CLAP_ABI noop(const clap_plugin_t *p) {}
static bool CLAP_ABI start(const clap_plugin_t *p) { return true; }
static clap_process_status CLAP_ABI process(const clap_plugin_t *p, const clap_process_t *ctx) {
    for (uint32_t i = 0; i < ctx->frames_count; ++i) {
        ctx->audio_outputs[0].data32[0][i] = ctx->audio_inputs[0].data32[0][i];
        ctx->audio_outputs[0].data32[1][i] = ctx->transport ? (float)ctx->transport->tempo : -1.0f;
    }
    return CLAP_PROCESS_CONTINUE;
}
static uint32_t CLAP_ABI count(const clap_plugin_t *p, bool input) { return 1; }
static bool CLAP_ABI port(const clap_plugin_t *p, uint32_t index, bool input, clap_audio_port_info_t *info) {
    if (index != 0) return false;
    memset(info, 0, sizeof(*info));
    info->id = 0; strcpy(info->name, "Stereo");
    info->flags = CLAP_AUDIO_PORT_IS_MAIN; info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO; info->in_place_pair = CLAP_INVALID_ID;
    return true;
}
static const clap_plugin_audio_ports_t ports = { count, port };
static const void *CLAP_ABI extension(const clap_plugin_t *p, const char *id) {
    return strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0 ? &ports : NULL;
}
static uint32_t CLAP_ABI plugin_count(const clap_plugin_factory_t *f) { return 1; }
static const clap_plugin_descriptor_t *CLAP_ABI describe(const clap_plugin_factory_t *f, uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}
static const clap_plugin_t *CLAP_ABI create(const clap_plugin_factory_t *f, const clap_host_t *host, const char *id) {
    if (strcmp(id, descriptor.id) != 0) return NULL;
    clap_plugin_t *p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    *p = (clap_plugin_t){ &descriptor, NULL, init, destroy, activate, noop, start, noop, noop, process, extension, noop };
    return p;
}
static const clap_plugin_factory_t factory = { plugin_count, describe, create };
static bool CLAP_ABI entry_init(const char *path) { return true; }
static void CLAP_ABI entry_deinit(void) {}
static const void *CLAP_ABI get_factory(const char *id) {
    return strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &factory : NULL;
}
CLAP_EXPORT const clap_plugin_entry_t clap_entry = { CLAP_VERSION, entry_init, entry_deinit, get_factory };

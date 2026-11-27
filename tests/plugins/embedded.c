/*
 * embedded.c — a plugin linked with its own copy of liblightc.a.
 *
 * Its copy of lightc keeps its own globals, such as the count of TLS keys
 * made, but finds its per-thread block through the thread pointer, which
 * is the host's: the first key it makes is the host's key 0, and its heap
 * runs on the host's per-thread heap state. plugin_tls_init makes a key and
 * sets it, and plugin_allocate allocates from the plugin's heap. The loader
 * must refuse the plugin, which defines lc_tls_* and lc_heap_* functions.
 */

#include <lightc/heap.h>
#include <lightc/thread.h>

static lc_tls_key key;

int plugin_tls_init(void) {
    if (!lc_tls_key_create(&key)) return -1;
    lc_tls_set(key, (void *)0x1234);
    return (int)key;
}

void *plugin_allocate(size_t size) {
    lc_result_ptr r = lc_heap_allocate(size);
    return r.error ? 0 : r.value;
}

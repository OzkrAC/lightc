/*
 * tls_ie.c — a plugin with thread-local storage of the initial-exec model.
 *
 * The plugin has a PT_TLS header, and plugin_tls_bump finds tls_counter at
 * the thread pointer plus an offset that an R_X86_64_TPOFF64 or
 * R_AARCH64_TLS_TPREL64 relocation puts in its GOT. Unrelocated, that
 * offset is 0, so the increment hits the word at the thread pointer, which
 * is lightc's own per-thread data.
 */

static __thread int tls_counter __attribute__((tls_model("initial-exec")));
int plugin_tls_bump(void) { return ++tls_counter; }

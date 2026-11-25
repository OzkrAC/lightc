/*
 * tls_gd.c — a plugin with thread-local storage of the general-dynamic
 * model, the default for -fPIC.
 *
 * The plugin has a PT_TLS header, and plugin_tls_bump finds tls_counter
 * through __tls_get_addr on x86_64, with R_X86_64_DTPMOD64 and DTPOFF64
 * relocations, and through a TLS descriptor on aarch64 (R_AARCH64_TLSDESC).
 */

__thread int tls_counter;
int plugin_tls_bump(void) { return ++tls_counter; }

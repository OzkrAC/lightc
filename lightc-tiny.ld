/*
 * lightc — tiny linker script
 * Two segments, minimal alignment waste, plus a non-executable stack.
 */

ENTRY(_start)

PHDRS
{
    text  PT_LOAD FLAGS(5);          /* R+X */
    data  PT_LOAD FLAGS(6);          /* R+W */
    stack PT_GNU_STACK FLAGS(6);     /* non-executable stack */
}

SECTIONS
{
    . = 0x400000 + SIZEOF_HEADERS;

    .text   : { *(.text) *(.text.*) } :text
    .rodata : { *(.rodata) *(.rodata.*) } :text

    /* Next max-page in the address space, same offset within the page: the RW
     * segment never shares a (up to 64 KiB) page with R+X text, yet needs no
     * file padding. */
    . = ALIGN(CONSTANT(MAXPAGESIZE)) + (. & (CONSTANT(MAXPAGESIZE) - 1));

    .data : { *(.data) *(.data.*) } :data
    .bss  : { *(.bss) *(.bss.*) *(COMMON) } :data

    /* No PT_TLS, constructors or destructors: these must stay empty. */
    .tdata         : { *(.tdata) *(.tdata.*) } :data
    .tbss          : { *(.tbss) *(.tbss.*) *(.tcommon) } :data
    .preinit_array : { KEEP(*(.preinit_array)) } :data
    .init_array    : { KEEP(*(.init_array)) KEEP(*(.init_array.*))
                       KEEP(*(.ctors)) KEEP(*(.ctors.*)) } :data
    .fini_array    : { KEEP(*(.fini_array)) KEEP(*(.fini_array.*))
                       KEEP(*(.dtors)) KEEP(*(.dtors.*)) } :data

    ASSERT(SIZEOF(.tdata) + SIZEOF(.tbss) == 0,
           "lightc: thread_local is not supported (no PT_TLS); use lc_tls_* instead")
    ASSERT(SIZEOF(.preinit_array) + SIZEOF(.init_array) == 0,
           "lightc: constructors are not supported (_start never runs .preinit_array or .init_array)")
    ASSERT(SIZEOF(.fini_array) == 0,
           "lightc: destructors are not supported (nothing runs .fini_array)")

    /DISCARD/ : {
        *(.note.*)
        *(.comment)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.gnu.hash)
        *(.gnu.property)
    }
}

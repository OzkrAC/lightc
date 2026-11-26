/*
 * version.c — the ABI tag, lc_abi_MAJOR_MINOR, that every object compiled
 * with lightc's headers refers to: see <lightc/version.h>.
 *
 * It is an absolute symbol, with no storage, so linking it costs a program
 * nothing. A program refers to it from each of its objects, so the linker
 * takes this object from liblightc.a whatever else the program uses.
 */

#include <lightc/version.h>

__asm__(".globl " LC_ABI_STRING(LC_ABI_TAG) "\n\t"
        ".set " LC_ABI_STRING(LC_ABI_TAG) ", 0");

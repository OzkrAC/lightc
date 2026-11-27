/*
 * lightc — syscall wrappers for x86_64
 *
 * Linux x86_64 syscall convention:
 *   syscall number: %rax
 *   arguments:      %rdi, %rsi, %rdx, %r10, %r8, %r9
 *   return value:   %rax
 *   clobbered:      %rcx, %r11
 *
 * C calling convention (System V AMD64 ABI):
 *   arguments:      %rdi, %rsi, %rdx, %rcx, %r8, %r9
 *
 * Note: arg4 moves from %rcx (C ABI) to %r10 (syscall ABI).
 */

/*
 * The faultinject build, which configure's LC_FAULTINJECT selects, names
 * these lc_fault_raw_syscall0 to lc_fault_raw_syscall6: src/fault.c then
 * defines lc_syscall0 to lc_syscall6 as C functions that check the armed
 * fault rules before they call these. Every other build names them
 * lc_syscall0 to lc_syscall6, as here.
 */
#ifdef LC_FAULTINJECT
#define lc_syscall0 lc_fault_raw_syscall0
#define lc_syscall1 lc_fault_raw_syscall1
#define lc_syscall2 lc_fault_raw_syscall2
#define lc_syscall3 lc_fault_raw_syscall3
#define lc_syscall4 lc_fault_raw_syscall4
#define lc_syscall5 lc_fault_raw_syscall5
#define lc_syscall6 lc_fault_raw_syscall6
#endif

.section .text

.global lc_syscall0
.type lc_syscall0, @function
lc_syscall0:
    movq    %rdi, %rax
    syscall
    ret
.size lc_syscall0, . - lc_syscall0

.global lc_syscall1
.type lc_syscall1, @function
lc_syscall1:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    syscall
    ret
.size lc_syscall1, . - lc_syscall1

.global lc_syscall2
.type lc_syscall2, @function
lc_syscall2:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    movq    %rdx, %rsi
    syscall
    ret
.size lc_syscall2, . - lc_syscall2

.global lc_syscall3
.type lc_syscall3, @function
lc_syscall3:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    movq    %rdx, %rsi
    movq    %rcx, %rdx
    syscall
    ret
.size lc_syscall3, . - lc_syscall3

.global lc_syscall4
.type lc_syscall4, @function
lc_syscall4:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    movq    %rdx, %rsi
    movq    %rcx, %rdx
    movq    %r8,  %r10
    syscall
    ret
.size lc_syscall4, . - lc_syscall4

.global lc_syscall5
.type lc_syscall5, @function
lc_syscall5:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    movq    %rdx, %rsi
    movq    %rcx, %rdx
    movq    %r8,  %r10
    movq    %r9,  %r8
    syscall
    ret
.size lc_syscall5, . - lc_syscall5

.global lc_syscall6
.type lc_syscall6, @function
lc_syscall6:
    movq    %rdi, %rax
    movq    %rsi, %rdi
    movq    %rdx, %rsi
    movq    %rcx, %rdx
    movq    %r8,  %r10
    movq    %r9,  %r8
    movq    8(%rsp), %r9
    syscall
    ret
.size lc_syscall6, . - lc_syscall6

/* No executable stack: without this note, ld assumes this object needs one. */
.section .note.GNU-stack,"",@progbits

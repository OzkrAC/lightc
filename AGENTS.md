# Working on lightc

lightc is a freestanding C runtime for Linux on x86_64 and aarch64: no libc, and everything from `_start` to exit is made of raw system calls. This file is for people and agents who change lightc or build on it. `README.md` has the whole story; this is the short version, and `ci/agents-check.sh` fails CI when a path, ninja target, configure mode, function or macro named here does not exist.

## Why there is no libc

A program on lightc loads nothing at run time: no NSS or gconv modules, which matters inside a container's root. The child of a raw clone holds no libc state, and a fork duplicates no stdio buffers. So the library and its tests include no libc header and call no libc function. The only headers from outside are the compiler's freestanding ones, such as `<stdint.h>`, `<stddef.h>` and `<stdatomic.h>`.

## Look before you write a wrapper

`api/index.md` lists every public function, one row each: its header, signature and purpose, how it reports an error, its layer, the system calls it can make and whether the child of a raw clone can call it. `api/syscalls.tsv` lists the same system calls, one line per function and call, so `grep accept4 api/syscalls.tsv` finds every function that makes accept4. Both are generated, so they are always current, and `test_index` runs the functions of the main headers under a seccomp filter of just those calls, to check them.

When lightc lacks something general, add it to lightc rather than to the program that needs it.

## Conventions

- **Two error conventions.** The `lc_kernel_*` wrappers and `lc_syscall0` to `lc_syscall6` return `lc_sysret`, the kernel's result: a negative errno on failure. The modules return `lc_result` or `lc_result_ptr`, whose `.error` holds a positive errno, or a library code from 4096 up, and is 0 on success. `lc_result_from_sysret` converts the first to the second. The Errors column of `api/index.md` gives each function's.
- **Two layers.** `lc_kernel_*` wrap system calls directly, inline in `include/lightc/syscall.h`. The modules, `lc_<module>_*` and lightio's `lio_*`, are built on them. The Layer column gives each function's.
- **Every public function has a comment right above its declaration**, whose first sentence says what it does. That sentence is its purpose in the index, and `tools/api-snapshot.sh` stops on a public function without one.
- **A result that must be checked is `[[nodiscard]]`.**
- **A new wrapper passes every argument the system call takes.**
- **Kernel values are checked against the kernel.** Each macro of the public headers and of `src/kernel.h` is in one list of `tests/abi_check.c`, which compares it with the UAPI header or says why it cannot, and `sh ci/abilint.sh` fails on one in no list. Each struct shared with the kernel has its size and every field checked there too.

## Adding a system call

1. Add a `syscall NAME X86_64 AARCH64 ARGS` line to `syscalls.def`, with the kernel's numbers for both arches, `-` for an arch without it.
2. Run `sh tools/gensyscalls.sh` and commit what it changes. CI fails when that changes a file.
3. Write the wrapper in `include/lightc/syscall.h` as a `static inline` function that calls `lc_syscallN(SYS_NAME, ...)`, with a comment above it. Constants and structs that only the library's sources use go in `src/kernel.h`.
4. List each new macro and struct in `tests/abi_check.c`, and run `sh ci/abilint.sh`. To compare with the UAPI headers, compile it for both arches: `gcc -std=c2x -ffreestanding -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only tests/abi_check.c`, and the same with `aarch64-linux-gnu-gcc`.
5. Test it, in `tests/test_syscall.c` or the suite of its module.
6. Run `sh tools/api-snapshot.sh` and commit `api/` with the change. It finds the new call in the sources, so nothing else lists it.

## Fixing a defect

A fixed defect gets a regress program, `tests/regress/<id>.c`, named by the defect's id. It is written as a suite is, with `tests/test.h`, and it fails while the defect is there. `tests/regress/manifest` lists it as `gating`, so `ninja regress` runs it, or as `pending` while the defect is still open; that file says how.

## Building and testing

```sh
./configure debug          # or release, tiny, ubsan; ./configure clean removes build/
ninja                      # the library and the examples
ninja test                 # build and run the test suites
ninja regress              # run the regress programs of fixed defects
ninja smoke                # run the examples that check their own results
ninja api-check            # check that api/ matches the tree
CC=aarch64-linux-gnu-gcc ./configure release && ninja all tests   # aarch64 cross build
./configure faultinject    # system calls that fail on request, for tests
```

What `ninja regress` and `ninja test` need of the machine:

- **Thread ids that wrap.** `HEAP-tid-reuse-tls` skips above a `kernel.pid_max` of 32768, which fails the run. Lower it with `sudo sysctl -w kernel.pid_max=32768`, or name the program in `LIGHTC_TEST_ALLOW_SKIP`.
- **io_uring.** Without it, `test_async` and `test_lightio` skip every test, which fails the run unless `LIGHTC_TEST_ALLOW_SKIP` names them.
- **User namespaces.** `test_sandbox` runs each test in user, mount, pid and UTS namespaces of its own, made with `tests/sandbox.h`. Where they cannot be made, or AppArmor's `kernel.apparmor_restrict_unprivileged_userns=1` gives them no capabilities, every test skips, which fails the run unless `LIGHTC_TEST_ALLOW_SKIP` names it. Lift that with `sudo sysctl -w kernel.apparmor_restrict_unprivileged_userns=0`.
- **lld on aarch64.** `LIB-relr` needs a linker that packs relative relocations. GNU ld 2.42 does not on aarch64, so there `configure` links that plugin with lld, and without lld `LIB-relr` fails.

In a tree built with `./configure faultinject`, every system call of the library first checks a table of fault rules, so a test can make the Nth call of one fail with the errno it chooses, and chaos points that `LC_CHAOS` seeds yield or spin in `lc_spinlock_acquire` and in the heap's remote frees: `<lightc/fault.h>` says how. Only that mode builds `tests/test_fault.c`, and `ninja test` runs it there; no other mode compiles any of it.

The other checks CI runs: `sh ci/headers.sh` compiles each public header alone, `sh ci/elfcheck.sh build` checks the program headers of every binary built, and `sh ci/agents-check.sh` checks this file.

## How a change lands

- **Generated files.** Commit `api/` and the files `tools/gensyscalls.sh` writes with the change that needs them.
- **CHANGELOG.** A change that users can see gets an entry in the Unreleased section of `CHANGELOG.md`.

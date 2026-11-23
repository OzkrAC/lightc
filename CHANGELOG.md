# Changelog

## Unreleased

Changes since 0.1.1. Seven more defects from the November 2026 code review of 0.1.0 are fixed, all of low severity: one in the library, the others in the test runner, the socket tests, an example and the docs. The Id column under Fixed is each defect's label in that review. The public API only grows, with `lc_kernel_getsockname`, two `SYS_*` numbers and an `LC_ERR_*` code for every Linux errno, and objects compiled against 0.1.1 headers need not be recompiled. The rest is test and build work: `ninja test` runs every suite with a time limit and ends with a summary, `ninja regress` and `ninja smoke` are new, `./configure ubsan` builds with undefined behaviour checks that trap, and CI runs every build mode with GCC 13 and 14 and the test suites on native aarch64.

### Upgrading: run configure again

- Run `./configure` again. ninja does not regenerate `build.ninja`, and one written by 0.1.1 has no `regress` or `smoke` target and runs the suites the old way.
- Objects compiled against 0.1.1 headers need not be recompiled, since the header code they hold behaves the same. `lc_kernel_fork` passes the same 17, now spelled `SIGCHLD`. `lc_kernel_futex_wait` and `lc_kernel_futex_wake` now pass all six futex arguments, the added ones 0, and the kernel ignores those for `FUTEX_WAIT` and `FUTEX_WAKE`. `lc_error_name` is inline too, so it gives the new names only in code compiled against the new `<lightc/result.h>`; see Behaviour changes.
- `configure` now adds `CFLAGS` and `LDFLAGS` from the environment after its own flags, uses `STRIP` from it for release and tiny links and `OBJCOPY` for tiny ones, and passes `SYSROOT` as `--sysroot`. 0.1.1 ignored all five. Unset them, or set them empty, when they are meant for another project.
- `configure` takes the arch from `$CC -dumpmachine` instead of `uname -m`, and stops when that fails or prints nothing. `ARCH` is still read, and `configure`'s header now calls it deprecated. It must be the arch `CC` targets: `ARCH=aarch64` with an x86_64 `gcc` now stops `configure` with `ARCH=aarch64, but gcc targets ...`, where 0.1.1 went on and failed in ninja.
- Where no io_uring ring can be created, as under qemu-user or with io_uring disabled, `test_async` and `test_lightio` skip every test, and `ninja test` now fails on a suite that skips every test; in 0.1.1 they passed with no test run. On such machines set `LIGHTC_TEST_ALLOW_SKIP="test_async test_lightio"`.

### Behaviour changes

- **`lc_error_name` names every Linux errno.** It returned `"UNKNOWN"` for the 112 errno values that had no `LC_ERR_*` code, such as 9, and now returns their names, such as `"EBADF"`. A value with two names gives the first: `"EAGAIN"` for 11 and `"EDEADLK"` for 35. Every value it named before keeps its name. Check any code that tests its result for `"UNKNOWN"`.

### Fixed

| Id | Severity | What users see |
| --- | --- | --- |
| IO-dirent-align | low | `lc_directory_next` read each entry through a `struct linux_dirent64` pointer 4 bytes off that struct's alignment, which C leaves undefined. x86_64 and aarch64 load the fields it read at any alignment, so other builds got the right entries, but a build with alignment checks, such as the new ubsan mode, trapped at the first entry. It now reads the fields at their offsets in the record. `<lightc/io.h>` and the layout of `lc_directory` are unchanged. |
| RT-ninja-test-deps | low | `ninja test` stopped at the first suite that failed, so the suites after it never ran. `ninja tests` did not build `test_plugin.so`, `build/test_library` found it only when run from the tree root, and editing a linker script relinked nothing. Now every suite runs, `ninja tests` builds the plugin, `test_library` finds it from any directory, and editing the build's linker script relinks every program linked with it. |
| TEST-socket-port | low | `test_socket` and the socket example used fixed ports: 19001 and 19002, and 18234 to 18236. When another process held one, they waited forever, so `ninja test` hung, and two runs on one machine could collide. They now bind port 0, read the port they got with the new `lc_kernel_getsockname`, and fail within 5 s when a server cannot start. |
| EX-heap-doublefree | low | `heap.example`'s heap_reclamation check expected reused blocks to be zero, which nothing guarantees, stopped at the first that was not, then freed all 8000 blocks, 7999 of them a second time. A debug build died with `lightc: heap corruption detected`, and a release build printed FAIL and exited 0. The check now frees only the blocks it got. A new check, heap_pages_reclaimed, reads the heap statistics, so it skips in release and tiny, which keep none. |
| DOC-accuracy | low | The docs said that a plain `ninja` builds the tests, that `types.h` defines its types without `<stdint.h>` and that the large-block cache is LRU. `ninja` builds the library and the examples, and `ninja tests` builds the suites. `types.h` includes the compiler's own freestanding `<stdint.h>` and `<stddef.h>`. The cache hands out the smallest mapping that fits, and when it is full it unmaps the smaller of the freed mapping and its smallest one. README.md and `docs/` now say so. |
| DOC-log-example | low | The begin/commit example in `<lightc/log.h>` did not compile: it assigned the result of `lc_log_begin`, which takes a pointer to the entry and a level and returns nothing. The example now declares the entry and passes `&entry`, and a note says not to copy the entry between begin and commit. |
| DOC-readme-regs | low | README.md said the aarch64 context switch saves 13 registers. It saves 21: x19-x28, x29, x30, sp and d8-d15. README.md now lists the registers for both arches. |

### Added

- `lc_kernel_getsockname(fd, addr, addr_length)` in `<lightc/syscall.h>`. It copies a socket's local address, so after a bind to port 0 it gives the port the kernel picked.
- `SYS_getsockname` (51 on x86_64, 204 on aarch64) and `SYS_setitimer` (38 on x86_64, 103 on aarch64) in `<lightc/arch/*/syscall_nr.h>`. Remove any copies you defined yourself.
- An `LC_ERR_*` code in `<lightc/result.h>` for every errno of Linux's `asm-generic/errno-base.h` and `asm-generic/errno.h`, with the kernel's name and value: 114 new names, such as `LC_ERR_BADF` (9), `LC_ERR_FAULT` (14) and `LC_ERR_BUSY` (16). As in the kernel, `LC_ERR_WOULDBLOCK` is `LC_ERR_AGAIN` and `LC_ERR_DEADLOCK` is `LC_ERR_DEADLK`. Remove any of these you defined yourself: an enumerator declared twice does not compile.
- Nothing in the public headers was removed or renamed, and no value changed on either arch: the 19 errno codes of 0.1.1 and the library codes from 4096 keep their names and values, and every `SYS_*` number is the same. The `SYS_*` lines are now generated, in number order, each with a comment giving the syscall's argument count.
- `syscalls.def` lists each syscall lightc's headers name, with its number on each arch and its argument count, and every Linux errno. `sh tools/gensyscalls.sh` generates the two `syscall_nr.h` files and the errno lines of `result.h` from it, so edit `syscalls.def`, not them.
- `./configure ubsan` builds with `-O1 -g -fsanitize=undefined -fsanitize-trap=all`, which makes each undefined behaviour check a trap instruction that needs no sanitizer runtime. Code built this way that hits undefined behaviour stops at the check with SIGILL on x86_64 or SIGTRAP on aarch64. lightc's safety checks stay on, as in debug, the binaries are not stripped, and `ninja install` writes the same `lightc.pc` as in the other modes.
- `configure` now also reads `STRIP`, `OBJCOPY`, `CFLAGS`, `LDFLAGS` and `SYSROOT` from the environment, besides `CC`, `AR` and `ARCH`, which 0.1.1 read too; the comment at its top says what each does. The target is that of `CC`, so `CC=aarch64-linux-gnu-gcc ./configure` alone sets up an aarch64 cross build, in any mode. `AR`, `STRIP` and `OBJCOPY` default to the tools with `CC`'s prefix when those are installed, so release and tiny cross builds no longer strip with the host's `strip`, which fails on aarch64 binaries on Ubuntu. `SYSROOT` is passed as `--sysroot`.
- `ninja regress` builds and runs the programs that `tests/regress/manifest` lists as gating, each named by its defect's id, as in the Id column under Fixed, and fails when one fails. So far these are RT-main-exit, RT-fchmodat-flags, FMT-print-double-oob and IO-dirent-align. `ninja regress-pending` runs the programs of defects not fixed yet, so far TIME-sleep-eintr, and lists each with PASS or FAIL without failing on their results. Neither is part of `ninja test`.
- `ninja smoke` runs the 13 examples that check their own results, and `lightio_test`, as `ninja test` runs the suites. Each prints a summary in the suites' format and exits 1 when a check fails; the 13 used to end with "all passed" and exit 0 even after a check had printed FAIL. When io_uring_setup fails with ENOSYS or EPERM, `async.example` and `lightio_test` skip, so name them in `LIGHTC_TEST_ALLOW_SKIP` there. `async.example`'s async_peek check, which could fail on a busy machine, now polls for up to a second.

### Build and CI

- `ninja test` runs every suite through `tests/run.sh`, also after one fails, and ends with a count of the suites and tests that passed, failed and skipped, and the reason each failed suite failed. It exits non-zero on any failure. A suite still running after `LIGHTC_TEST_TIMEOUT` seconds (default 120, 0 for no limit) is killed and fails; without a `timeout` that takes `-k`, the runner says so and sets no limit. A suite that skips every test fails the run unless `LIGHTC_TEST_ALLOW_SKIP` names it.
- `ninja test` runs 20 suites. The new `test_harness` checks `tests/test.h`, which gains `TEST_REQUIRE_*` checks that end the test when they fail, `TEST_SKIP`, and `TEST_ASSERT_EQ` and `TEST_ASSERT_NEQ` failures that print both values. A suite binary given test names runs only those, as in `build/test_heap test_heap_free_null`.
- A plain `ninja` builds on aarch64 too: `examples/bench_heap.c` uses `rdtsc` on x86_64 only, and prints ns/op on aarch64.
- On x86_64, CI builds debug, release, tiny and ubsan with GCC 13 and with GCC 14, and all eight jobs run `ninja test`, `ninja regress` and `ninja smoke`. Debug and release also run natively on aarch64 (`ubuntu-24.04-arm`), where no test may skip, and the qemu-user job stays. A new job cross-builds aarch64 release and tiny. A clang build of debug and release and the pending regress programs report without gating. When a step fails, the x86_64 and native aarch64 jobs print where a crashed program stopped, with `ci/crash-report.sh`.
- The UAPI ABI check now also checks every errno value, every macro of the public headers and of the new internal `src/kernel.h` that a UAPI header defines, and the size and field offsets of every struct lightc shares with the kernel. It fails when `syscalls.def` misses an errno or a file generated from it is out of date. `ci/abilint.sh` checks that every macro of those headers is in one of the lists of `tests/abi_check.c`, which check it or say why it cannot be checked, and that each `lc_syscallN` call passes the argument count `syscalls.def` gives.
- `ci/headers.sh` compiles each public header alone. `ci/elfcheck.sh` checks the program headers of every binary CI builds: no writable and executable segment and a non-executable `PT_GNU_STACK`, and in an executable also no `PT_TLS` and no page (64 KiB on aarch64) that holds both code and writable data.
- The public headers use C23's `[[attribute]]` syntax, so a program compiled as C17 needs GCC 14 or newer, or clang, which accept it there as an extension; GCC 13 rejects it before C23. This was already so in 0.1.1. The headers check compiles as C17 only with the compilers that accept it.
- CI reports the test totals of each x86_64 job, and the size of the tiny hello, as notices on each run.

### Notes for lightbox

- lightbox needs no change. Its sources compile against the new headers for both arches with no new warning, and to the same code as against 0.1.1's.
- lightbox runs lightc's `configure` with `ARCH` taken from the same `$(CC) -dumpmachine` field, so `ARCH` always matches `CC`. CI's aarch64 cross job runs lightbox's `ARCH`, `CC` and `AR` line for aarch64. Once its lightc is past 0.1.1, lightbox can drop `ARCH=` from that line, since `configure` takes the arch from `CC`.
- From `20bbd96` on, lightbox's Makefile clears `CFLAGS` and `LDFLAGS` for lightc's `configure`, so lightc is built as before. An older lightbox run with `CFLAGS` or `LDFLAGS` set in the environment hands its own flags to `configure`, which now adds them to lightc's. `liblightc.a` still builds, with lightbox's flags added. A later plain `ninja` in the submodule then fails to link, because those `LDFLAGS` pass `-T lightc/lightc.ld`, a path that does not exist from inside the submodule, until `configure` is run again without them.
- lightbox's Makefile does not rebuild `lightc/build/liblightc.a` when the submodule moves. One left from 0.1.1 still works with lightbox, because the library changed only in `lc_directory_next` and in the futex calls of the thread functions, and lightbox calls neither. Delete it to build the library from the new sources.

## 0.1.1

A hotfix for 0.1.0. A fresh clone builds again, and 11 defects found in a November 2026 code review of 0.1.0 are fixed: 8 high, 2 medium and 1 low. The Id column under Fixed is each defect's label in that review. CI again builds and tests every push and pull request to main or master, and now also runs the test suites on aarch64 under qemu-user.

### Upgrading: rebuild, do not just relink

- Several fixes are in header code that gets compiled into your own objects. In `<lightc/syscall.h>` these are the `static inline` `lc_kernel_mount`, `lc_kernel_fchmodat` and `lc_kernel_reset_signal`, the `lc_kernel_sigaction` struct (32 bytes on aarch64, 24 before) and the `O_*` macros. The `SYS_*` numbers are in `<lightc/arch/*/syscall_nr.h>`, which it includes. An object compiled against 0.1.0 headers keeps the old code, so recompile every file that includes a lightc header.
- Rebuild lightc itself from a fresh `./configure`. ninja does not regenerate `build.ninja`, and a 0.1.0 `build.ninja` does not parse.
- Link against the new `liblightc.a` and the new linker scripts. An old `liblightc.a` still ends `main` with `SYS_exit`. 0.1.0 did not install the linker scripts, so replace any copy kept in your own tree: an old script keeps the aarch64 page bug, emits no `PT_GNU_STACK` and runs no link checks.
- With pkg-config, `--libs` now gives `-static -nostdlib -nostartfiles` and `-T` with `lightc.ld`. Drop any of these you added for 0.1.0. A second `-T` is added to the first, not used in its place, and doubles the program headers. Put the `--libs` flags after your sources, since `-llightc` is a static archive.
- `configure` now reads `ARCH` from the environment. A value other than `x86_64` or `aarch64`, such as the kernel's `arm64`, stops it with `Unsupported architecture`, so unset `ARCH` if something else sets it.

### Behaviour changes

- **`lc_kernel_mount` mounts on x86_64.** In 0.1.0 it ran sendfile, so nothing was mounted. Calls that did nothing before now mount, so check that each caller still means it and handles the result.
- **`lc_kernel_fchmodat` with flags needs Linux 6.6.** Non-zero flags go to fchmodat2. Before Linux 6.6 that call fails, and the wrapper returns `-EOPNOTSUPP`. It never falls back to fchmodat, which would drop the flags. `AT_SYMLINK_NOFOLLOW` on a symlink returns `-EOPNOTSUPP` on every kernel, since Linux has no symlink modes. Flags 0 behave as before. `docs/guide.html` describes the `O_PATH` fallback for refusing symlinks on older kernels.
- **`thread_local`, constructors and destructors fail to link.** lightc has no TLS segment, and its start code never runs the program's `.preinit_array`, `.init_array` or `.fini_array`, so such code used to link and then misbehave. Libraries opened with `lc_library_open` still get their constructors and destructors run. Links that use `lightc.ld` or `lightc-tiny.ld` now stop with a `lightc:` message; links with other scripts get no check. pkg-config's `--libs` uses `lightc.ld`. Use the `lc_tls_*` keys instead of `thread_local`, and call setup code from `main`.
- **Returning from `main` ends every thread.** `_start` now calls exit_group, not exit. A program whose threads are still running when `main` returns exits at once with `main`'s value, and those threads stop wherever they are. In 0.1.0 the process kept running until its last thread ended. Join any thread whose work must finish before you return from `main`. A plain return still runs no atexit handlers.

### Fixed

| Id | Severity | What users see |
| --- | --- | --- |
| RT-mount-nr | high | x86_64 `SYS_mount` was 40, which is sendfile there, so `lc_kernel_mount` never mounted and returned `-EBADF` or `-EFAULT`. It is now 165. |
| RT-a64-odirectory | high | On aarch64 `O_DIRECTORY` had the x86_64 value, which is `O_DIRECT` there. Directory opens could fail with `-EINVAL`, and the directory check passed regular files. `O_DIRECTORY` is now defined per arch, as are the new `O_NOFOLLOW` and `O_DIRECT`. |
| RT-fchmodat-flags | high | `lc_kernel_fchmodat` dropped its flags, so `AT_SYMLINK_NOFOLLOW` still changed the symlink's target. See Behaviour changes. |
| RT-main-exit | high | Returning from `main` ended only the main thread, so a program with a live thread did not exit until that thread ended, or ever if it never did. |
| FMT-print-double-oob | high | From about 1e25 up, or about -1e24 down, `lc_print_double` printed stack memory from beyond its 32-byte buffer: 276 extra bytes for 1e300. From about 1e24 (or -1e23) up to that point it printed a NUL in place of the last character. It now prints the full value. |
| RT-a64-segment-pages | high | On aarch64 the writable segment could share a 16 KiB or 64 KiB page with code, and kernels with those page sizes then mapped that page without execute permission. A binary whose start code was on that page crashed at its first instruction: on 64 KiB kernels that was hello and every other example and test in this tree, and on 16 KiB kernels small ones such as hello. Other binaries crashed only when they ran code from that page. |
| RT-pkgconfig | high | The installed `lightc.pc` gave link failures, or binaries that mixed in glibc and corrupted its thread data. `gcc $(pkg-config --cflags lightc) hello.c $(pkg-config --libs lightc)` now gives a static lightc binary, also on compilers that default to PIE and the stack protector. |
| RT-thread-local | high | `thread_local` variables compiled and linked, then shared memory with lightc's heap state. Links with `lightc.ld` or `lightc-tiny.ld`, which pkg-config uses, now fail. |
| RT-a64-sigaction | medium | On aarch64 `lc_kernel_sigaction` had no restorer field, so it was 24 bytes where the kernel reads and writes 32. Handlers installed through lightc ran with an arbitrary set of signals blocked, and reading an old action into this type overflowed it by 8 bytes. The type is now 32 bytes on both arches. |
| RT-gnu-stack | medium | Binaries had no `PT_GNU_STACK` header, which kernels before 5.8 treat as "everything readable is executable". A link with GNU ld's default script got an executable stack. Binaries built with lightc's linker scripts now have a non-executable stack. `liblightc.a` no longer asks for an executable one, so a link with the default script gets a non-executable stack too, unless another object asks for one. |
| HDR-result-null | low | `<lightc/result.h>` used `NULL` without including `<stddef.h>`, so it failed to compile alone. |

### Added

- `O_NOFOLLOW` and `O_DIRECT` with per-arch values, and `AT_SYMLINK_NOFOLLOW` (0x100), in `<lightc/syscall.h>`. Remove any copies you defined yourself.
- `SYS_fchmodat2` (452 on both arches) in `<lightc/arch/*/syscall_nr.h>`.
- `LC_ERR_NOSYS` (38) and `LC_ERR_OPNOTSUPP` (95) in `<lightc/result.h>`, with their names in `lc_error_name`.
- `ninja install` puts `lightc.ld` and `lightc-tiny.ld` in `<prefix>/lib/lightc/`. `lightc.pc` passes `-T` with the first and names the second in its `ldscript_tiny` variable; `docs/index.html` shows how to link with it. On aarch64 the installed Cflags add `-mno-outline-atomics`, which builds that skip pkg-config must pass themselves.
- `configure` takes the target arch from `ARCH`. `ARCH=aarch64 CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar ./configure debug` sets up a cross build; see Build and CI for the targets to build on aarch64.

### Build and CI

- A fresh clone builds: ninja no longer stops at `build.ninja:237` with a lexing error. On aarch64 a plain `ninja` still stops in `examples/bench_heap.c`, which uses the x86-only `rdtsc`. There, run `ninja build/liblightc.a` for the library, or `ninja test` to build and run the test suites. A cross build can build the suites without running them with `ninja tests build/test_plugin.so`.
- aarch64 builds pass `-mno-outline-atomics`, so they no longer need libgcc's atomic helpers.
- `ninja test` builds the plugin `test_library` loads, runs all 19 suites, including `test_lightio` and the new `test_syscall`, and shows each suite's output as it runs.
- The Build & Test workflow is back. It has debug, release and tiny jobs, a kernel ABI check for both arches, a pkg-config consumer check, and the test suites on aarch64 under qemu-user.
- Binaries are smaller: x86_64 tiny hello drops from 8200 B to 4448 B, because `.data` no longer starts on a fresh page in the file.

### Notes for lightbox

- Rebuild lightbox and its lightc as described above. lightbox's Makefile builds `lightc/build/liblightc.a` only when it is missing, and `make clean` leaves it, so delete it after moving the submodule to 0.1.1. With `LIGHTC=` pointing at another tree, run `make clean` after switching trees, because lightbox does not recompile when only that tree changes.
- On x86_64, lightbox at `9dfe420` or older calls `lc_kernel_mount` without checks during `setup`. Rebuilt against 0.1.1, it really mounts cgroup2 on `cgroup_root`; under 0.1.0 that call ran sendfile. Use lightbox `5179e04` or later, which checks the cgroup root first.
- lightbox from its `ls` rewrite on passes `AT_SYMLINK_NOFOLLOW` to its own statx. lightc defines that constant only from 0.1.1, so that lightbox no longer builds against 0.1.0 headers.
- On aarch64, lightbox's volume-source check and its container listing now open with a real `O_DIRECTORY`. 0.1.0 passed `O_DIRECT` there, so a regular file given as a volume source could pass the directory check; with 0.1.1 it fails it.
- The lightbox binary now has a `GNU_STACK` RW header, so its stack is not executable.
- lightbox's private `LB_SYS_mount` can go once it is pinned to 0.1.1, since `lc_kernel_mount` uses 165 on x86_64.
- lightbox needs no source change for the fchmodat, link-check or exit_group changes. It passes flags 0 to fchmodat, uses no `thread_local`, constructors or destructors, and starts no threads.

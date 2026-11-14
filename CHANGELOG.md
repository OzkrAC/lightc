# Changelog

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

# lightc

A freestanding C runtime for Linux. No libc, no linker scripts from the system, no external dependencies. Everything from `_start` to `exit` is implemented from scratch using raw Linux syscalls.

Targets x86_64 and aarch64. Built with `./configure tiny`, the hello-world example is a static binary of 4640 bytes in CI's `x86_64 gcc 14 tiny` job: a measured size, not a limit (see below for where it comes from).

## Building

Requires GCC and ninja.

```sh
./configure debug    # -O0, debug symbols, safety checks on
./configure release  # -O2 -DNDEBUG, stripped
./configure tiny     # -Os, minimal binary size
./configure ubsan    # -O1, undefined behavior checks that trap, safety checks on

ninja                # build the library and the examples, not the test suites
ninja tests          # build the test suites without running them
ninja test           # build and run the test suites (277 tests in 20 suites)
ninja regress        # run the regress programs of fixed defects (tests/regress/manifest)
ninja smoke          # run the examples that check their own results
ninja bench          # run benchmarks
ninja api-check      # check that api/ is the snapshot of the public API
```

The size of the tiny hello above and the test count in this block are measured, and CI reports both again on every run as notices, which the run's summary page and the annotations of each job's check run show. Every `x86_64 gcc N MODE` job gives the totals of `ninja test`, from the summary of `tests/run.sh`, and the `x86_64 gcc 13 tiny`, `x86_64 gcc 14 tiny` and `aarch64 cross builds (release + tiny)` jobs give the size in bytes of `build/hello`. The size quoted is that of the `x86_64 gcc 14 tiny` job, built by Ubuntu 24.04's GCC 14.2 and binutils 2.42. That GCC turns on `-fcf-protection` by default, which adds `endbr64` instructions to the code, so a GCC 14 without that default, such as upstream GCC, builds a smaller hello; another compiler, binutils or change to the sources also gives another size. The count is that of the x86_64 jobs, the same in every mode and with either GCC; on aarch64, `test_harness` leaves out its one x86_64-only test.

`ninja smoke` runs the examples that check their own results, and `lightio_test`, through `tests/run.sh`, as `ninja test` runs the suites. Each prints a line per check that ends in `PASS`, `FAIL` or `SKIP:` and a reason, then the summary of `tests/test.h` (see `examples/checks.h`), and exits 1 when a check fails. Without io_uring, `async.example` and `lightio_test` skip every check, which fails the run unless `LIGHTC_TEST_ALLOW_SKIP` names them, as in `LIGHTC_TEST_ALLOW_SKIP="async.example lightio_test"`. Release and tiny builds keep no heap statistics (`LC_STATS`), so there `heap.example` skips its `heap_pages_reclaimed` check. The other examples (`hello`, `syscalls`, `print`, `heapviz`, the servers `httpd`, `lightio_echo` and `lightio_http`, and the benchmarks) print or serve without checking anything, so smoke does not run them.

`ninja regress` runs its programs through `tests/run.sh` too. `HEAP-tid-reuse-tls` waits for the kernel's thread ids to wrap round, which takes seconds at a `kernel.pid_max` of 32768 and minutes at the 4194304 that systemd sets by default, so above 32768 it skips, which fails the run. Lower it with `sudo sysctl -w kernel.pid_max=32768`, as CI does, or set `LIGHTC_TEST_ALLOW_SKIP=HEAP-tid-reuse-tls`. `LIB-relr` loads a plugin whose relative relocations are packed in `DT_RELR` (`ld -z pack-relative-relocs`). GNU ld 2.42, Ubuntu 24.04's, ignores that flag on aarch64, so there configure links that plugin with lld, and without lld `LIB-relr` fails.

`configure` builds for the target of `CC` (default `gcc`), x86_64 or aarch64, and takes `ar`, `strip` and `objcopy` with `CC`'s prefix when those are installed. It reads `CC`, `AR`, `STRIP`, `OBJCOPY`, `CFLAGS`, `LDFLAGS` and `SYSROOT` from the environment; the comment at the top of `configure` says what each does. `ARCH` is still read, but it must equal the compiler's arch. A cross build for aarch64, where `ninja all tests` builds the default target and the test suites without running anything:

```sh
CC=aarch64-linux-gnu-gcc ./configure release
ninja all tests
```

The ubsan mode compiles every check of `-fsanitize=undefined` to a trap instruction (`-fsanitize-trap=all`), so it needs no sanitizer runtime: a program that hits undefined behavior stops at once with SIGILL on x86_64 or SIGTRAP on aarch64. Its binaries are not stripped, and like every lightc binary they are static and not PIE, so the address the kernel logs for the trap maps straight to the check's source line: `sudo dmesg | sh ci/crash-report.sh` prints that line for each program in `build/` that a fault or trap killed (a timed-out suite, which timeout(1) stops with a signal, leaves no report). ubsan links with `lightc.ld`, as debug and release do, and `ninja install` writes the same `lightc.pc` in every mode.

The syscall numbers and errno values come from `syscalls.def`. `sh tools/gensyscalls.sh` generates `include/lightc/arch/*/syscall_nr.h`, the `LC_ERR_*` errno lines of `include/lightc/result.h` and the lists `tests/abi_check.c` checks from it. Run it after editing `syscalls.def` and commit the files it changes with the edit: CI runs it too and fails when it changes a file.

`tests/abi_check.c` is only compiled, once per arch against that arch's Linux UAPI headers, and each `_Static_assert` in it compares lightc with the kernel: every syscall number and errno value of `syscalls.def`, every macro of the public headers and of `src/kernel.h` (the kernel structs and constants that only the library's sources use) that a UAPI header defines, and, for every struct lightc shares with the kernel, its size and the offset and size of each field. A macro that no UAPI header can check is listed there with the reason, and so is each of lightc's own. `sh ci/abilint.sh` fails when a header under `include/` or `src/` defines a macro that none of those lists names, and when a call `lc_syscallN(SYS_name, ...)` has another N than the argument count `syscalls.def` gives `name`, or names a syscall `syscalls.def` does not list; an `lc_syscallN` it cannot read as such a call fails too. CI runs both, and `sh ci/abilint-test.sh` checks that the lint catches such mistakes.

`api/` is a snapshot of the public API, one item per line: `symbols.txt` lists the global symbols `liblightc.a` defines, `decls.txt` each declaration of the public headers but not their macros, and `layout-x86_64.txt` and `layout-aarch64.txt` the size and alignment of each public struct and the offset, size and type of each field. `sh tools/api-snapshot.sh` writes it, with GCC for both arches (`gcc` and `aarch64-linux-gnu-gcc`, or `CC_X86_64` and `CC_AARCH64`), and the comment at its top says how. `ninja api-check` writes the snapshot of the tree to `build/api` and fails with a diff when it differs from `api/`, and CI runs it: after an intended change to the API, run `sh tools/api-snapshot.sh` and commit `api/` with the change. `api/index.md` is an index of every public function, one table row each: its header, signature and purpose, how it reports an error, its layer, the system calls it makes and whether it is safe after a raw clone. `api/syscalls.tsv` gives the same system calls, one line per function and call. The tool writes both: the purpose from the first sentence of the comment above each declaration, so every public function needs one, and the system calls and clone safety from the library's sources, by following each function's calls, so they cannot go stale. Look there before writing a wrapper of your own. `api/v0.1.1/` is the snapshot of the 0.1.1 release, and each release adds its own, written with `sh tools/api-snapshot.sh -o api/vX.Y.Z`. `ninja api-additive` fails when the API removes or changes a line of the newest of them, so only additions pass, and a renamed parameter or an added attribute changes a line too; it is to gate 0.2.x and 0.3.x once 0.2.0, which changes the API on purpose, is out, and until then CI does not run it.

`sh ci/headers.sh` compiles each public header alone, as C23 with `-Wpedantic -Werror` and as C17 with `-Werror`, so a header that uses something it does not include fails. The headers use C23's `[[attribute]]` syntax, so a program compiled as C17 needs a compiler that takes it there as an extension, GCC 14 or newer or clang; GCC 13 rejects it before C23, and the script then compiles as C23 only. CI runs it with GCC 13, GCC 14, the aarch64 cross compiler (GCC 13) and clang.

`sh ci/elfcheck.sh build` reads the program headers of every ELF executable and shared object under `build/`, found by their ELF header, not their names. An executable fails on a writable and executable header, a missing or executable `PT_GNU_STACK` (kernels before 5.8 make every readable mapping executable when it is missing), a `PT_TLS` header, or an R E and an RW `PT_LOAD` that touch the same page: 64 KiB on aarch64, where kernels use 4, 16 or 64 KiB pages, and 4 KiB on x86_64. A shared object fails only on the first two, so it may have `PT_TLS`. Each failure names the file and the rule, and the script also fails on a file it cannot read and when it finds no binary. CI runs it on the binaries of every job that builds lightc and on the pkg-config consumer's hello, and `sh ci/elfcheck-test.sh gcc aarch64-linux-gnu-gcc` checks that it fails binaries that break each rule, for that rule, and passes good ones.

## Modules

### Core (`include/lightc/`)

| Header | Description |
|---|---|
| `syscall.h` | Raw Linux syscall wrappers (inline assembly) |
| `types.h` | `uint8_t`, `size_t` and the like, from the compiler's own freestanding `<stdint.h>` and `<stddef.h>` (not libc's); `lc_sysret`, the version (`LC_VERSION_*`, `lc_version()`), and `result.h` |
| `string.h` | String ops (`length`, `find`, `compare`) and byte ops (`copy`, `fill`, `compare`) via `__builtin_*` |
| `memory.h` | Arena allocator (bump allocation, reset, inline fast path) and raw page allocation |
| `heap.h` | General-purpose heap allocator with bucket allocator, large block cache, per-thread pages, optional statistics |
| `format.h` | Type-safe format builder (no format strings, no varargs) |
| `print.h` | Direct-to-fd printing (char, string, integer, hex) |
| `io.h` | Buffered reader/writer, file utilities, directory listing |
| `thread.h` | Threads via `clone` syscall, spinlocks |
| `coroutine.h` | Stackful coroutines with cooperative scheduling (~5ns context switch) |
| `async.h` | Async I/O via `io_uring` |
| `socket.h` | TCP/UDP sockets, IPv4 and IPv6 |
| `time.h` | Monotonic clock, wall clock, date/time conversion, formatting |
| `log.h` | Leveled logging with plain text and JSON output |
| `signal.h` | Signal handling (portable across x86_64/aarch64) |
| `lifecycle.h` | `atexit` handler chain with SIGTERM integration |
| `library.h` | Runtime shared library loading (ELF parser; self-contained libraries without TLS) |

### Data structures (`include/lightdata/`)

| Header | Description |
|---|---|
| `array.h` | Dynamic array (push, pop, insert, remove, sort) |
| `hashmap.h` | Open-addressing hash map (string keys) |
| `ringbuf.h` | Fixed-capacity ring buffer |
| `list.h` | Intrusive doubly-linked list |

### Async I/O framework (`include/lightio/`)

| Header | Description |
|---|---|
| `lightio.h` | Event loop over `io_uring` that runs one coroutine per TCP connection, with read, write and sleep calls that suspend only that coroutine, and a threaded mode with a ring per worker |

### Heap allocator

The heap uses a segment/page architecture with thread-local ownership:

- **Small allocations** (<=4096 bytes): bitmap-based bucket allocator across 29 size classes with ~1.25x geometric spacing. Each thread owns pages exclusively -- allocation and same-thread free require no locks. Cross-thread free uses a lock-free atomic push to a per-page remote free list.
- **Large allocations** (>4096 bytes): direct `mmap`/`munmap`, with a 16-entry cache of freed mappings to avoid repeated syscalls. A request takes the smallest cached mapping that fits (best fit). When the cache is full, the smaller of the freed mapping and the smallest cached one is unmapped, so the cache keeps the largest.
- **Debug mode** (`LC_HEAP_DEBUG`): double-free detection via magic validation, use-after-free detection via 0xCC poison fill.
- **Statistics** (`LC_STATS`): optional per-allocation tracking with atomic counters (allocation counts, active bytes, peaks, large cache hit rate).

### Coroutines

Stackful coroutines with per-coroutine `mmap`'d stacks and a guard page at the bottom. Context switching saves/restores only callee-saved registers via hand-written assembly: 7 on x86_64 (rbx, rbp, r12-r15, rsp) and 21 on aarch64 (x19-x28, x29, x30, sp, d8-d15). A yield between two coroutines takes ~5ns.

## Build modes and compile-time flags

| Flag | Default (debug) | Default (release) | Effect |
|---|---|---|---|
| `LC_HEAP_DEBUG` | 1 | 0 | Double-free detection, use-after-free poisoning |
| `LC_STATS` | 1 | 0 | Heap/arena allocation statistics tracking |
| `NDEBUG` | not set | set | Controls defaults of the above |

Override any flag with `-DLC_STATS=1` or `-DLC_STATS=0` at compile time.



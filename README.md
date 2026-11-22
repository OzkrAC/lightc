# lightc

A freestanding C runtime for Linux. No libc, no linker scripts from the system, no external dependencies. Everything from `_start` to `exit` is implemented from scratch using raw Linux syscalls.

Targets x86_64 and aarch64. A tiny hello-world binary is 415 bytes.

## Building

Requires GCC and ninja.

```sh
./configure debug    # -O0, debug symbols, safety checks on
./configure release  # -O2 -DNDEBUG, stripped
./configure tiny     # -Os, minimal binary size
./configure ubsan    # -O1, undefined behavior checks that trap, safety checks on

ninja                # build everything
ninja test           # run all tests (171 tests across 10 suites)
ninja regress        # run the regress programs of fixed defects (tests/regress/manifest)
ninja smoke          # run the examples that check their own results
ninja bench          # run benchmarks
```

`ninja smoke` runs the examples that check their own results, and `lightio_test`, through `tests/run.sh`, as `ninja test` runs the suites. Each prints a line per check that ends in `PASS`, `FAIL` or `SKIP:` and a reason, then the summary of `tests/test.h` (see `examples/checks.h`), and exits 1 when a check fails. Without io_uring, `async.example` and `lightio_test` skip every check, which fails the run unless `LIGHTC_TEST_ALLOW_SKIP` names them, as in `LIGHTC_TEST_ALLOW_SKIP="async.example lightio_test"`. Release and tiny builds keep no heap statistics (`LC_STATS`), so there `heap.example` skips its `heap_pages_reclaimed` check. The other examples (`hello`, `syscalls`, `print`, `heapviz`, the servers `httpd`, `lightio_echo` and `lightio_http`, and the benchmarks) print or serve without checking anything, so smoke does not run them.

`configure` builds for the target of `CC` (default `gcc`), x86_64 or aarch64, and takes `ar`, `strip` and `objcopy` with `CC`'s prefix when those are installed. It reads `CC`, `AR`, `STRIP`, `OBJCOPY`, `CFLAGS`, `LDFLAGS` and `SYSROOT` from the environment; the comment at the top of `configure` says what each does. `ARCH` is still read, but it must equal the compiler's arch. A cross build for aarch64, where `ninja all tests` builds everything, the test suites included, without running anything:

```sh
CC=aarch64-linux-gnu-gcc ./configure release
ninja all tests
```

The ubsan mode compiles every check of `-fsanitize=undefined` to a trap instruction (`-fsanitize-trap=all`), so it needs no sanitizer runtime: a program that hits undefined behavior stops at once with SIGILL on x86_64 or SIGTRAP on aarch64. Its binaries are not stripped, and like every lightc binary they are static and not PIE, so the address the kernel logs for the trap maps straight to the check's source line: `sudo dmesg | sh ci/crash-report.sh` prints that line for each program in `build/` that a fault or trap killed (a timed-out suite, which timeout(1) stops with a signal, leaves no report). ubsan links with `lightc.ld`, as debug and release do, and `ninja install` writes the same `lightc.pc` in every mode.

The syscall numbers and errno values come from `syscalls.def`. `sh tools/gensyscalls.sh` generates `include/lightc/arch/*/syscall_nr.h`, the `LC_ERR_*` errno lines of `include/lightc/result.h` and the lists `tests/abi_check.c` checks from it. Run it after editing `syscalls.def` and commit the files it changes with the edit: CI runs it too and fails when it changes a file.

`tests/abi_check.c` is only compiled, once per arch against that arch's Linux UAPI headers, and each `_Static_assert` in it compares lightc with the kernel: every syscall number and errno value of `syscalls.def`, every macro of the public headers and of `src/kernel.h` (the kernel structs and constants that only the library's sources use) that a UAPI header defines, and, for every struct lightc shares with the kernel, its size and the offset and size of each field. A macro that no UAPI header can check is listed there with the reason, and so is each of lightc's own. `sh ci/abilint.sh` fails when a header under `include/` or `src/` defines a macro that none of those lists names, and when a call `lc_syscallN(SYS_name, ...)` has another N than the argument count `syscalls.def` gives `name`, or names a syscall `syscalls.def` does not list; an `lc_syscallN` it cannot read as such a call fails too. CI runs both, and `sh ci/abilint-test.sh` checks that the lint catches such mistakes.

`sh ci/headers.sh` compiles each public header alone, as C23 with `-Wpedantic -Werror` and as C17 with `-Werror`, so a header that uses something it does not include fails. The headers use C23's `[[attribute]]` syntax, so a program compiled as C17 needs a compiler that takes it there as an extension, GCC 14 or newer or clang; GCC 13 rejects it before C23, and the script then compiles as C23 only. CI runs it with GCC 13, GCC 14, the aarch64 cross compiler (GCC 13) and clang.

`sh ci/elfcheck.sh build` reads the program headers of every ELF executable and shared object under `build/`, found by their ELF header, not their names. An executable fails on a writable and executable header, a missing or executable `PT_GNU_STACK` (kernels before 5.8 make every readable mapping executable when it is missing), a `PT_TLS` header, or an R E and an RW `PT_LOAD` that touch the same page: 64 KiB on aarch64, where kernels use 4, 16 or 64 KiB pages, and 4 KiB on x86_64. A shared object fails only on the first two, so it may have `PT_TLS`. Each failure names the file and the rule, and the script also fails on a file it cannot read and when it finds no binary. CI runs it on the binaries of every job that builds lightc and on the pkg-config consumer's hello, and `sh ci/elfcheck-test.sh gcc aarch64-linux-gnu-gcc` checks that it fails binaries that break each rule, for that rule, and passes good ones.

## Modules

### Core (`include/lightc/`)

| Header | Description |
|---|---|
| `syscall.h` | Raw Linux syscall wrappers (inline assembly) |
| `types.h` | `uint8_t`, `size_t`, `bool`, etc. with no `<stdint.h>` |
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
| `library.h` | Runtime shared library loading (ELF parser) |

### Data structures (`include/lightdata/`)

| Header | Description |
|---|---|
| `array.h` | Dynamic array (push, pop, insert, remove, sort) |
| `hashmap.h` | Open-addressing hash map (string keys) |
| `ringbuf.h` | Fixed-capacity ring buffer |
| `list.h` | Intrusive doubly-linked list |

### Heap allocator

The heap uses a segment/page architecture with thread-local ownership:

- **Small allocations** (<=4096 bytes): bitmap-based bucket allocator across 29 size classes with ~1.25x geometric spacing. Each thread owns pages exclusively -- allocation and same-thread free require no locks. Cross-thread free uses a lock-free atomic push to a per-page remote free list.
- **Large allocations** (>4096 bytes): direct `mmap`/`munmap` with a 16-entry LRU cache to avoid repeated syscalls.
- **Debug mode** (`LC_HEAP_DEBUG`): double-free detection via magic validation, use-after-free detection via 0xCC poison fill.
- **Statistics** (`LC_STATS`): optional per-allocation tracking with atomic counters (allocation counts, active bytes, peaks, large cache hit rate).

### Coroutines

Stackful coroutines with per-coroutine `mmap`'d stacks and a guard page at the bottom. Context switching saves/restores only callee-saved registers (7 on x86_64, 13 on aarch64) via hand-written assembly. A yield between two coroutines takes ~5ns.

## Build modes and compile-time flags

| Flag | Default (debug) | Default (release) | Effect |
|---|---|---|---|
| `LC_HEAP_DEBUG` | 1 | 0 | Double-free detection, use-after-free poisoning |
| `LC_STATS` | 1 | 0 | Heap/arena allocation statistics tracking |
| `NDEBUG` | not set | set | Controls defaults of the above |

Override any flag with `-DLC_STATS=1` or `-DLC_STATS=0` at compile time.



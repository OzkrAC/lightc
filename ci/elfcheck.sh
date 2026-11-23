#!/bin/sh
# lightc — program header check of built binaries
#
# Reads the program headers (readelf -lW) of every ELF executable and
# shared object it is given or finds, and fails on the ones the kernel
# would load with a writable and executable mapping, an executable stack,
# or a page that two segments need with different protections.
#
# An executable fails on
#
#   rwx           a program header other than PT_GNU_STACK that is
#                 writable and executable;
#   no-gnu-stack  no PT_GNU_STACK header: kernels before 5.8 then set
#                 READ_IMPLIES_EXEC, which makes every readable mapping
#                 executable, the heap and the stack among them;
#   exec-stack    a PT_GNU_STACK header with the E flag;
#   tls           a PT_TLS header: lightc gives a program no static TLS,
#                 so its thread_local data would have no storage;
#   page          an R E PT_LOAD and an RW PT_LOAD that touch the same
#                 page. The kernel maps each PT_LOAD over whole pages with
#                 MAP_FIXED, so the later one takes over the page they share
#                 and the program faults on the code or the data in it. The
#                 page is 64 KiB on aarch64: kernels there use 4, 16 or 64
#                 KiB pages, and 64 KiB is ld's max page size there. It is
#                 4 KiB on x86_64, its only page size: the same failure, and
#                 the same check at no extra cost, so a linker script that
#                 put .data on the last page of .rodata fails here.
#
# A shared object fails only on rwx, no-gnu-stack and exec-stack. The
# kernel reads only the program's PT_GNU_STACK, but glibc's ld.so takes a
# shared object with no PT_GNU_STACK, or an executable one, to need an
# executable stack: it makes the stacks executable for one loaded at
# startup, and from glibc 2.41 dlopen refuses one. lc_library_open reads
# no PT_GNU_STACK, so this rule keeps lightc's shared objects right for
# other loaders too. A shared object may have a PT_TLS header: the test
# plugins build/plugins/tls_*.so carry one, to check that lc_library_open
# refuses them. It is not held to the page rule either: a plugin such as
# build/test_plugin.so is linked with the compiler's default linker script,
# for the page size of the ld that linked it, and lc_library_open refuses
# one with two segments of different protections on one page of the
# kernel's.
#
# A PIE is an executable, though its ELF type is ET_DYN as a shared
# object's is. It is told apart by a PT_INTERP header, or by DF_1_PIE in
# DT_FLAGS_1 for a static PIE, and gets every executable rule: the kernel
# runs it as a program, and its PT_TLS would get no storage either. lightc
# builds no PIE (its links pass -static -no-pie, and ci/consumer.sh checks
# that hello is ET_EXEC), so a PIE here was built some other way and is
# checked as what it is.
#
# A file is checked when it starts with the ELF magic, whatever its name.
# ELF objects (ET_REL) and core files are skipped, and so are archives and
# all other files. A file it cannot read fails, one found in a directory
# too, since it cannot tell what the file is. Each broken rule is a line "FAIL: FILE: RULE: what is
# wrong", and each file that passes is a line "ok: FILE (...)". It exits 1
# after checking every file when any of them failed, and also when it found
# no executable or shared object, so a wrong path cannot pass.
#
# Usage, from any directory: sh ci/elfcheck.sh PATH...
# Each PATH is a file or a directory, which is searched recursively.
# READELF defaults to readelf. GNU readelf reads the ELF files of every
# arch, so an x86_64 readelf checks aarch64 binaries too.

set -eu

READELF=${READELF:-readelf}

# readelf's English output, whatever the locale, and byte semantics for
# sort and awk.
LC_ALL=C
export LC_ALL

[ $# -gt 0 ] || { echo "usage: sh ci/elfcheck.sh PATH..."; exit 1; }
command -v "$READELF" >/dev/null || { echo "FAIL: $READELF not found"; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# Prints "TYPE MACHINE" for an ELF file, from its first 20 bytes: e_type
# (rel, exec, dyn, core or the number) and e_machine (x86_64, aarch64 or
# the number). Prints nothing for any other file.
elf_kind() {
    # -tu1 prints each byte in decimal.
    set -- $(od -An -tu1 -N20 -- "$1" 2>/dev/null || true)
    [ $# -eq 20 ] && [ "$1 $2 $3 $4" = "127 69 76 70" ] || return 0
    # Byte 6, EI_DATA, is 2 for big-endian.
    if [ "$6" = 2 ]; then
        type=$((${17} * 256 + ${18})) machine=$((${19} * 256 + ${20}))
    else
        type=$((${18} * 256 + ${17})) machine=$((${20} * 256 + ${19}))
    fi
    case $type in
        1) type=rel ;;
        2) type=exec ;;
        3) type=dyn ;;
        4) type=core ;;
    esac
    case $machine in
        62)  machine=x86_64 ;;
        183) machine=aarch64 ;;
    esac
    echo "$type $machine"
}

# The program headers as lines "TYPE VADDR MEMSZ FLAGS", where FLAGS is
# readelf's R, W and E, such as "R E" or "RW", or empty. readelf -lW puts
# one header on a line: Type, Offset, VirtAddr, PhysAddr, FileSiz, MemSiz,
# then the flags in up to two fields, then Align. Offset is the first field
# that starts with 0x, so this reads the fields from it and from the end,
# and the column titles and the "[Requesting program interpreter: ...]"
# line, which have no such field, are skipped.
PHDRS='
/^Program Headers:/ { in_headers = 1; next }
in_headers && NF == 0 { exit }
in_headers {
    i = 1
    while (i <= NF && $i !~ /^0x[0-9a-fA-F]+$/)
        i++
    if (i == 1 || NF < i + 5)
        next
    type = $1
    for (j = 2; j < i; j++)
        type = type "_" $j
    flags = ""
    for (j = i + 5; j < NF; j++)
        flags = flags (flags == "" ? "" : " ") $j
    print type, $(i + 1), $(i + 4), flags
}
'

failed_files=0
bad=0

# "1 file", "2 files"
count() {
    if [ "$1" -eq 1 ]; then echo "$1 $2"; else echo "$1 $2s"; fi
}

fail() {
    echo "FAIL: $file: $*"
    bad=1
}

# An address as readelf's 0x0000000000401000 or a number, printed as 0x401000.
hex() {
    printf '0x%x' $(($1))
}

# The page rule: every R E PT_LOAD against every RW one. Each list holds
# "first:last:vaddr" for a segment: the first and last page it touches,
# and its address.
check_pages() {
    for x in $text_loads; do
        for w in $data_loads; do
            x_first=${x%%:*} x_rest=${x#*:}
            w_first=${w%%:*} w_rest=${w#*:}
            x_last=${x_rest%%:*} w_last=${w_rest%%:*}
            if [ "$x_first" -le "$w_last" ] && [ "$w_first" -le "$x_last" ]; then
                shared=$x_first
                [ "$w_first" -le "$shared" ] || shared=$w_first
                fail "page: the R E LOAD at $(hex "${x_rest#*:}") and the RW LOAD at" \
                    "$(hex "${w_rest#*:}") share the $page_kib KiB page at $(hex $((shared * page)))"
            fi
        done
    done
}

: > "$work/files"
for path in "$@"; do
    if [ -d "$path" ]; then
        # find names what it could not search, as a directory it may not
        # read, and goes on with the rest.
        if ! find -H "$path" -type f >> "$work/files"; then
            echo "FAIL: $path: find could not search all of it"
            failed_files=$((failed_files + 1))
        fi
    elif [ -f "$path" ]; then
        printf '%s\n' "$path" >> "$work/files"
    else
        echo "FAIL: $path: no such file or directory"
        failed_files=$((failed_files + 1))
    fi
done
sort -u "$work/files" > "$work/sorted"

executables=0 shared_objects=0 objects=0 others=0
while IFS= read -r file; do
    if [ ! -r "$file" ]; then
        echo "FAIL: $file: unreadable: cannot read it, so cannot tell whether it is a binary"
        failed_files=$((failed_files + 1))
        continue
    fi
    kind=$(elf_kind "$file")
    case $kind in
        '')
            others=$((others + 1))
            continue
            ;;
        rel\ *|core\ *)
            objects=$((objects + 1))
            continue
            ;;
        exec\ *|dyn\ *) ;;
        *)
            echo "FAIL: $file: type: ELF type ${kind%% *} is neither ET_EXEC nor ET_DYN"
            failed_files=$((failed_files + 1))
            continue
            ;;
    esac
    type=${kind%% *} machine=${kind#* }

    if ! "$READELF" -lW -- "$file" > "$work/readelf" 2> "$work/readelf.err"; then
        echo "FAIL: $file: readelf: $READELF -lW failed:"
        cat "$work/readelf.err"
        failed_files=$((failed_files + 1))
        continue
    fi
    awk "$PHDRS" "$work/readelf" > "$work/phdrs"

    what=executable
    if [ "$type" = dyn ]; then
        what="shared object"
        if grep -q '^INTERP ' "$work/phdrs"; then
            what=PIE
        elif "$READELF" -dW -- "$file" 2>/dev/null |
                awk '$2 == "(FLAGS_1)" { for (i = 3; i <= NF; i++) if ($i == "PIE") found = 1 }
                     END { exit !found }'; then
            what="static PIE"
        fi
    fi

    bad=0
    stack=missing
    text_loads= data_loads=
    case $machine in
        x86_64)  page=4096 page_kib=4 ;;
        aarch64) page=65536 page_kib=64 ;;
        *)       page= ;;
    esac
    while read -r ptype vaddr memsz flags; do
        case $ptype in
            GNU_STACK)
                stack=$flags
                continue
                ;;
        esac
        case $flags in
            *W*E*)
                fail "rwx: the $ptype header at $(hex "$vaddr") is writable" \
                    "and executable ($flags)"
                ;;
        esac
        [ "$what" != "shared object" ] || continue
        case $ptype in
            TLS)
                fail "tls: a PT_TLS header in an executable; lightc gives a program no static TLS"
                ;;
            LOAD)
                case $flags in
                    *W*E*) continue ;;
                    *E*)   list=text ;;
                    *W*)   list=data ;;
                    *)     continue ;;
                esac
                [ -n "$page" ] || continue
                # Pages the segment's memory touches, from vaddr to
                # vaddr + memsz - 1. The shell does the arithmetic: awk
                # may not read hex. A user address has at most 15 hex
                # digits, so it fits in the shell's signed 64 bits.
                case ${vaddr#0x} in
                    [89a-fA-F]???????????????)
                        fail "page: the LOAD at $vaddr is above every user address space"
                        continue
                        ;;
                esac
                first=$((vaddr / page))
                last=$(((vaddr + (memsz > 0 ? memsz - 1 : 0)) / page))
                if [ "$list" = text ]; then
                    text_loads="$text_loads $first:$last:$vaddr"
                else
                    data_loads="$data_loads $first:$last:$vaddr"
                fi
                ;;
        esac
    done < "$work/phdrs"

    case $stack in
        missing)
            if [ "$what" = "shared object" ]; then
                fail "no-gnu-stack: no PT_GNU_STACK header, so glibc's ld.so" \
                    "takes it to need an executable stack"
            else
                fail "no-gnu-stack: no PT_GNU_STACK header, so kernels before 5.8" \
                    "make every readable mapping executable (READ_IMPLIES_EXEC)"
            fi
            ;;
        *E*)
            fail "exec-stack: PT_GNU_STACK asks for an executable stack ($stack), want RW"
            ;;
    esac
    if [ "$what" != "shared object" ]; then
        if [ -n "$page" ]; then
            check_pages
        else
            fail "machine: no page size known for ELF machine $machine;" \
                "this checks x86_64 and aarch64"
        fi
        executables=$((executables + 1))
    else
        shared_objects=$((shared_objects + 1))
    fi

    if [ "$bad" -eq 0 ]; then
        echo "ok: $file ($what, $machine)"
    else
        failed_files=$((failed_files + 1))
    fi
done < "$work/sorted"

echo "elfcheck: checked $(count "$executables" executable) and $(count "$shared_objects" "shared object");" \
    "skipped $(count "$objects" "ELF object") and $(count "$others" "other file")"
if [ $((executables + shared_objects)) -eq 0 ]; then
    echo "FAIL: no ELF executable or shared object in $*"
    exit 1
fi
if [ "$failed_files" -gt 0 ]; then
    echo "FAIL: $(count "$failed_files" file) did not pass"
    exit 1
fi

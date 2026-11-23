#!/bin/sh
# lightc — corrupt test plugins for the loader's regress programs
#
# Writes OUT, a copy of the shared object IN with one of its tables
# corrupted, as MODE says:
#
#   hash-cycle     Every bucket of the DT_HASH table names symbol 1, and the
#                  chain from symbol 1 runs through every symbol and back to
#                  1, so a lookup of a name the object does not define never
#                  reaches the 0 that ends a valid chain. IN must have
#                  DT_HASH with at least 3 symbols, and no DT_GNU_HASH,
#                  which a loader would search instead.
#   relaent1       DT_RELAENT is 1 and DT_RELASZ is 768614336404564651,
#                  which times 24, the size of an Elf64_Rela, is 2^64 + 8:
#                  a count taken as DT_RELASZ / DT_RELAENT then wraps a
#                  bounds check of count * 24 to 8 bytes. IN must have
#                  DT_RELASZ and DT_RELAENT.
#   unknown-reloc  The first entry of DT_RELA gets relocation type 32767,
#                  which no psABI of x86_64 or aarch64 defines.
#   wild-reloc     The target of the first entry of DT_RELA moves 1 GiB
#                  further, past the end of any plugin of the tests.
#   none-reloc     The first entry of DT_RELA becomes all zeros: a NONE
#                  relocation of target 0, which GNU ld leaves in a table
#                  it sized larger than it filled. Where IN is linked at a
#                  nonzero address, that target is outside it.
#   abs-symbol     The symbol of the first R_X86_64_64 or R_AARCH64_ABS64
#                  entry of DT_RELA becomes absolute: its st_shndx SHN_ABS
#                  and its value 0x12345678. GNU ld resolves a reference to
#                  an absolute symbol itself, so no object it links has
#                  such a relocation. IN must have one, and DT_SYMTAB.
#
# The last four need DT_RELA. The build edges configure writes run this
# to make those plugins, so nothing binary is checked in. It reads IN's
# little-endian ELF64 headers with od and patches OUT with dd, so it needs
# only a POSIX sh, od and dd, and builds the aarch64 plugins on any host.
#
# Usage: sh tests/plugins/corrupt.sh MODE IN OUT

set -eu

[ $# -eq 3 ] || { echo "usage: sh tests/plugins/corrupt.sh MODE IN OUT" >&2; exit 2; }
mode=$1 in=$2 out=$3
case $mode in
    hash-cycle|relaent1|unknown-reloc|wild-reloc|none-reloc|abs-symbol) ;;
    *) echo "corrupt.sh: unknown mode '$mode'" >&2; exit 2 ;;
esac

fail() {
    echo "corrupt.sh: $in: $*" >&2
    exit 1
}

# The ELF values this reads: <elf.h>'s names and values.
PT_LOAD=1 PT_DYNAMIC=2
DT_NULL=0 DT_HASH=4 DT_SYMTAB=6 DT_RELA=7 DT_RELASZ=8 DT_RELAENT=9
DT_GNU_HASH=$((0x6ffffef5))
EM_X86_64=62 EM_AARCH64=183 R_X86_64_64=1 R_AARCH64_ABS64=257
SHN_ABS=$((0xfff1))

# The unsigned little-endian number of $2 bytes at offset $1 of IN. Every
# value read here is far below 2^63, so the shell's signed arithmetic holds
# it.
read_le() {
    set -- "$2" $(od -An -tu1 -v -j "$1" -N "$2" -- "$in")
    [ $# -eq $(($1 + 1)) ] || fail "ends inside a header"
    shift
    value=0 bits=0
    for byte in "$@"; do
        value=$((value + (byte << bits)))
        bits=$((bits + 8))
    done
    echo "$value"
}

# Appends the $2 bytes of the little-endian number $1 to bytes, as the
# octal escapes printf writes them from.
add_le() {
    byte_index=0
    while [ "$byte_index" -lt "$2" ]; do
        bytes="$bytes\\$(printf '%03o' $((($1 >> (8 * byte_index)) & 255)))"
        byte_index=$((byte_index + 1))
    done
}

# Writes bytes over the copy at offset $1.
write_bytes() {
    printf "$bytes" | dd of="$tmp" bs=1 seek="$1" conv=notrunc 2>/dev/null
    bytes=
}

[ "$(read_le 0 4)" -eq $((0x464c457f)) ] || fail "not an ELF file"
[ "$(read_le 4 1)" -eq 2 ] || fail "not ELF64"
[ "$(read_le 5 1)" -eq 1 ] || fail "not little-endian"

# The program headers: PT_DYNAMIC, and each PT_LOAD as offset:vaddr:filesz
# for file_offset.
phoff=$(read_le 32 8)
phentsize=$(read_le 54 2)
phnum=$(read_le 56 2)
dynamic= dynamic_size= loads=
i=0
while [ "$i" -lt "$phnum" ]; do
    ph=$((phoff + i * phentsize))
    type=$(read_le "$ph" 4)
    p_offset=$(read_le $((ph + 8)) 8)
    p_vaddr=$(read_le $((ph + 16)) 8)
    p_filesz=$(read_le $((ph + 32)) 8)
    case $type in
        "$PT_LOAD")    loads="$loads $p_offset:$p_vaddr:$p_filesz" ;;
        "$PT_DYNAMIC") dynamic=$p_offset dynamic_size=$p_filesz ;;
    esac
    i=$((i + 1))
done
[ -n "$dynamic" ] || fail "no PT_DYNAMIC"

# The file offset of the virtual address $1.
file_offset() {
    for load in $loads; do
        offset=${load%%:*} rest=${load#*:}
        vaddr=${rest%%:*} filesz=${rest#*:}
        if [ "$1" -ge "$vaddr" ] && [ "$1" -lt $((vaddr + filesz)) ]; then
            echo $(($1 - vaddr + offset))
            return
        fi
    done
    fail "address $1 is in no PT_LOAD"
}

# The dynamic entries this needs: for each, its value and the file offset
# of that value.
hash= gnu_hash= symtab= rela= relasz= relasz_at= relaent_at=
entry=$dynamic
while [ "$entry" -lt $((dynamic + dynamic_size)) ]; do
    tag=$(read_le "$entry" 8)
    value=$(read_le $((entry + 8)) 8)
    case $tag in
        "$DT_NULL")     break ;;
        "$DT_HASH")     hash=$value ;;
        "$DT_GNU_HASH") gnu_hash=$value ;;
        "$DT_SYMTAB")   symtab=$value ;;
        "$DT_RELA")     rela=$value ;;
        "$DT_RELASZ")   relasz=$value relasz_at=$((entry + 8)) ;;
        "$DT_RELAENT")  relaent_at=$((entry + 8)) ;;
    esac
    entry=$((entry + 16))
done

tmp=$out.tmp
trap 'rm -f -- "$tmp"' EXIT
cat -- "$in" > "$tmp"
bytes=

case $mode in
    hash-cycle)
        [ -n "$hash" ] || fail "no DT_HASH"
        [ -z "$gnu_hash" ] || fail "has DT_GNU_HASH, which a loader searches instead of DT_HASH"
        table=$(file_offset "$hash")
        nbucket=$(read_le "$table" 4)
        nchain=$(read_le $((table + 4)) 4)
        [ "$nchain" -ge 3 ] || fail "DT_HASH has fewer than 3 symbols"
        # buckets[], then chains[]: chains[0] stays 0, chains[i] = i + 1,
        # and the last goes back to 1.
        i=0
        while [ "$i" -lt "$nbucket" ]; do
            add_le 1 4
            i=$((i + 1))
        done
        add_le 0 4
        i=1
        while [ "$i" -lt $((nchain - 1)) ]; do
            add_le $((i + 1)) 4
            i=$((i + 1))
        done
        add_le 1 4
        write_bytes $((table + 8))
        ;;
    relaent1)
        [ -n "$relasz_at" ] && [ -n "$relaent_at" ] || fail "no DT_RELASZ and DT_RELAENT"
        add_le 1 8
        write_bytes "$relaent_at"
        add_le 768614336404564651 8
        write_bytes "$relasz_at"
        ;;
    unknown-reloc)
        [ -n "$rela" ] || fail "no DT_RELA"
        table=$(file_offset "$rela")
        # r_info follows r_offset, and its low 32 bits are the type.
        add_le 32767 4
        write_bytes $((table + 8))
        ;;
    wild-reloc)
        [ -n "$rela" ] || fail "no DT_RELA"
        table=$(file_offset "$rela")
        # r_offset is the entry's first field.
        add_le $(($(read_le "$table" 8) + (1 << 30))) 8
        write_bytes "$table"
        ;;
    none-reloc)
        [ -n "$rela" ] || fail "no DT_RELA"
        table=$(file_offset "$rela")
        # r_offset, r_info and r_addend.
        add_le 0 8
        add_le 0 8
        add_le 0 8
        write_bytes "$table"
        ;;
    abs-symbol)
        [ -n "$rela" ] && [ -n "$relasz" ] || fail "no DT_RELA and DT_RELASZ"
        [ -n "$symtab" ] || fail "no DT_SYMTAB"
        case $(read_le 18 2) in
            "$EM_X86_64")  abs64=$R_X86_64_64 ;;
            "$EM_AARCH64") abs64=$R_AARCH64_ABS64 ;;
            *) fail "neither x86_64 nor aarch64" ;;
        esac
        table=$(file_offset "$rela")
        symbols=$(file_offset "$symtab")
        # Each Elf64_Rela is 24 bytes, r_info its second field: the type in
        # its low 32 bits, the symbol index in its high ones.
        sym=0 i=0
        while [ "$i" -lt $((relasz / 24)) ]; do
            info=$(read_le $((table + i * 24 + 8)) 8)
            if [ $((info & 0xffffffff)) -eq "$abs64" ] && [ $((info >> 32)) -ne 0 ]; then
                sym=$((info >> 32))
                break
            fi
            i=$((i + 1))
        done
        [ "$sym" -ne 0 ] || fail "no absolute relocation against a symbol in DT_RELA"
        # Each Elf64_Sym is 24 bytes: st_shndx at 6, st_value at 8.
        add_le "$SHN_ABS" 2
        write_bytes $((symbols + sym * 24 + 6))
        add_le $((0x12345678)) 8
        write_bytes $((symbols + sym * 24 + 8))
        ;;
esac

mv -- "$tmp" "$out"
trap - EXIT

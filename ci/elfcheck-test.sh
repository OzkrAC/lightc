#!/bin/sh
# lightc — self-test of ci/elfcheck.sh
#
# Links small programs that each break one rule of ci/elfcheck.sh, and
# some that break none, and checks that elfcheck fails each bad one with
# exactly the rule it breaks and passes the good ones. It needs no lightc
# build: each fixture is a small program with its own _start and
# code, read-only data, data and bss, linked with lightc.ld, lightc-tiny.ld
# or a copy of lightc.ld changed as below, or with the compiler's default
# linker script. With each compiler, the fixtures and what elfcheck must
# report are:
#
#   lightc.ld, lightc-tiny.ld  the scripts as they are: pass
#   old-align         lightc.ld with ". = ALIGN(4096);" before .data, the
#                     line of lightc 0.1.0, which put aarch64 hello's R E
#                     LOAD at 0x4000b0 and its RW LOAD at 0x401000. The RW
#                     LOAD shares a 64 KiB page with the R E one: page on
#                     aarch64. x86_64 has 4 KiB pages, so there it passes.
#   old-align-big     the same with 72 KiB of read-only data. On aarch64
#                     the RW LOAD starts on the 64 KiB page after the one
#                     the R E LOAD starts on, and the R E LOAD ends there,
#                     so only its size shows the page they share: page on
#                     aarch64, pass on x86_64
#   old-align-edge    old-align with .rodata padded to the next 64 KiB
#                     boundary, where the RW LOAD then starts: pass, since
#                     the two LOADs touch no page in common
#   no-align          lightc.ld without that line, so .data follows
#                     .rodata on its page: page, on both arches
#   no-gnu-stack      lightc.ld without its PT_GNU_STACK line, as in lightc
#                     0.1.0: no-gnu-stack
#   old-script        both, the program headers of lightc 0.1.0's
#                     lightc.ld: no-gnu-stack and page on aarch64,
#                     no-gnu-stack on x86_64
#   rwx               lightc.ld with FLAGS(7), RWX, on the text PT_LOAD: rwx
#   execstack         the default script and -z execstack: exec-stack
#   tls               a thread_local variable and the default script: tls
#   pie-tls, static-pie-tls
#                     the same linked as a PIE and as a static PIE: tls,
#                     since elfcheck checks a PIE as an executable
#   shared-tls        a shared object with a thread_local variable, linked
#                     as configure links build/test_plugin.so: pass, since
#                     a shared object may have PT_TLS
#   shared-execstack  the same with -z execstack: exec-stack
#   shared-rwx        a shared object without TLS, linked with a small
#                     script of its own (an R E and an RW PT_LOAD,
#                     PT_DYNAMIC and PT_GNU_STACK RW) whose text PT_LOAD is
#                     FLAGS(7), RWX: rwx
#   shared-no-gnu-stack
#                     the same object, linked with that script without its
#                     PT_GNU_STACK line, the text PT_LOAD R E: no-gnu-stack
#
# Each fixture name starts with its arch, as in aarch64-old-align. A shared
# object that passes must have PT_TLS, so the pass shows that elfcheck
# allows it. Then one run of elfcheck over the whole fixture directory,
# which also holds the objects and the linker scripts, must report every
# fixture as it did alone. Last, elfcheck must fail on a directory with
# only objects, an archive and a text file, on a path that does not exist,
# and on a file it cannot read, a bad fixture with mode 000, named beside a
# good one. The last is skipped for a user that reads such a file, as root
# does.
#
# Usage, from any directory: sh ci/elfcheck-test.sh [CC...]
# CC defaults to gcc; each one must target x86_64 or aarch64. Nothing is
# run, so a cross compiler works: CI runs this with gcc and
# aarch64-linux-gnu-gcc. AR defaults to ar, READELF to readelf, which
# elfcheck uses too. It exits 1 if anything did not behave as expected,
# after trying everything.

set -eu

# CDPATH= keeps cd from looking up "ci/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."
root=$(pwd)

[ $# -gt 0 ] || set -- gcc
AR=${AR:-ar}
READELF=${READELF:-readelf}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

dir=$work/fixtures
mkdir "$dir"
: > "$work/expected"

checks=0 wrong=0

fail() {
    echo "FAIL: $*"
    wrong=$((wrong + 1))
}

# Runs elfcheck from the fixture directory, so it names each fixture as
# given here. Its output goes to $work/out, its exit status to $status.
elfcheck() {
    status=0
    (cd "$dir" && sh "$root/ci/elfcheck.sh" "$@") > "$work/out" 2>&1 || status=$?
}

# link NAME ARGS...: runs $cc ARGS... -o $arch-NAME in the fixture
# directory, and prints the compiler's messages only when it fails.
link() {
    name=$arch-$1
    shift
    if ! $cc "$@" -o "$dir/$name" > "$work/link.log" 2>&1; then
        cat "$work/link.log"
        fail "$name did not build"
        return 1
    fi
}

# expect NAME RULES WHAT: elfcheck on the fixture $arch-NAME alone must
# report each of RULES once and no other rule, or with RULES "pass" no rule
# at all. WHAT says what the fixture is.
expect() {
    name=$arch-$1 want=$2
    checks=$((checks + 1))
    for rule in $want; do
        echo "$name $rule" >> "$work/expected"
    done
    elfcheck "$name"
    # The rules of the "FAIL: NAME: RULE: ..." lines, and "pass" for an
    # "ok: NAME (...)" line.
    got=$(awk -v name="$name" '
        index($0, "FAIL: " name ": ") == 1 {
            rule = substr($0, length("FAIL: " name ": ") + 1)
            sub(/:.*/, "", rule)
            print rule
        }
        index($0, "ok: " name " (") == 1 { print "pass" }
    ' "$work/out" | sort | tr '\n' ' ')
    if [ "$want" = pass ]; then
        want_status=0
    else
        want_status=1
    fi
    if [ "$got" = "$(printf '%s\n' $want | sort | tr '\n' ' ')" ] &&
            [ "$status" -eq "$want_status" ]; then
        if [ "$want" = pass ]; then
            echo "ok: $name ($3) passes:"
        else
            echo "ok: $name ($3) fails on exactly: $want"
        fi
        grep -e "^FAIL: $name: " -e "^ok: $name " "$work/out" | sed 's/^/    /'
    else
        fail "$name ($3): want ${want}, got exit $status and ${got:-no rule}:"
        sed 's/^/    /' "$work/out"
    fi
}

cat > "$work/hello.c" << 'EOF'
/* An elfcheck fixture with code, read-only data, data and bss, as hello
 * has. It is never run. */
const char message[] = "hello from an elfcheck fixture\n";
char buffer[32] = "data";
int count;

void _start(void) {
    for (;;) {
        buffer[count % 32] = message[count % 31];
        count++;
    }
}
EOF
cat > "$work/big.c" << 'EOF'
/* As hello.c, with more than 64 KiB of read-only data. */
const char table[72 * 1024] = { 1 };
char buffer[32] = "data";
int count;

void _start(void) {
    for (;;) {
        buffer[count % 32] = table[count % (72 * 1024)];
        count++;
    }
}
EOF
cat > "$work/tls.c" << 'EOF'
_Thread_local int counter = 5;

void _start(void) {
    for (;;)
        counter++;
}
EOF
cat > "$work/lib.c" << 'EOF'
_Thread_local int counter = 5;

int bump(void) {
    return ++counter;
}
EOF
cat > "$work/plain.c" << 'EOF'
int counter = 5;

int bump(void) {
    return ++counter;
}
EOF

# A linker script for a shared object, with the program headers lightc.ld
# gives an executable plus PT_DYNAMIC. ld adds no program header that
# PHDRS does not list, so the two derived from it below differ from it
# only in the header their sed changes.
cat > "$dir/shared.ld" << 'EOF'
PHDRS
{
    text    PT_LOAD FILEHDR PHDRS FLAGS(5);
    data    PT_LOAD FLAGS(6);
    dynamic PT_DYNAMIC FLAGS(6);
    stack   PT_GNU_STACK FLAGS(6);
}

SECTIONS
{
    . = SIZEOF_HEADERS;
    .hash     : { *(.hash) } :text
    .gnu.hash : { *(.gnu.hash) } :text
    .dynsym   : { *(.dynsym) } :text
    .dynstr   : { *(.dynstr) } :text
    .rela.dyn : { *(.rela.*) } :text
    .text     : { *(.text) *(.text.*) } :text
    . = ALIGN(CONSTANT(MAXPAGESIZE)) + (. & (CONSTANT(MAXPAGESIZE) - 1));
    .dynamic  : { *(.dynamic) } :data :dynamic
    .got      : { *(.got) *(.got.plt) } :data
    .data     : { *(.data) *(.data.*) *(.bss) *(.bss.*) } :data
    /DISCARD/ : { *(.note.*) *(.comment) *(.eh_frame) *(.eh_frame_hdr) *(.gnu.property) }
}
EOF

# The linker scripts, each changed in one place. The sed must change the
# line it is for: otherwise the fixture would be lightc.ld again.
cp lightc.ld lightc-tiny.ld "$dir/"
derive() {
    sed "$2" lightc.ld > "$dir/$1"
    if cmp -s lightc.ld "$dir/$1"; then
        fail "$1: sed '$2' did not change lightc.ld"
    fi
}
derive old-align.ld 's/^\( *\)\. = ALIGN(CONSTANT(MAXPAGESIZE)) .*/\1. = ALIGN(4096);/'
derive no-align.ld '/^ *\. = ALIGN(CONSTANT(MAXPAGESIZE)) /d'
derive no-gnu-stack.ld '/PT_GNU_STACK/d'
derive rwx.ld 's/^\( *text *PT_LOAD *\)FLAGS(5);/\1FLAGS(7);/'
sed '/PT_GNU_STACK/d' "$dir/old-align.ld" > "$dir/old-script.ld"
sed '/^ *\*(\.rodata\.\*)$/a\
        . = ALIGN(65536);' "$dir/old-align.ld" > "$dir/old-align-edge.ld"
if cmp -s "$dir/old-align.ld" "$dir/old-align-edge.ld"; then
    fail "old-align-edge.ld: sed did not pad .rodata in old-align.ld"
fi
sed 's/^\( *text *PT_LOAD *FILEHDR *PHDRS *\)FLAGS(5);/\1FLAGS(7);/' "$dir/shared.ld" > "$dir/shared-rwx.ld"
sed '/PT_GNU_STACK/d' "$dir/shared.ld" > "$dir/shared-no-gnu-stack.ld"
for script in shared-rwx.ld shared-no-gnu-stack.ld; do
    if cmp -s "$dir/shared.ld" "$dir/$script"; then
        fail "$script: sed did not change shared.ld"
    fi
done

for cc in "$@"; do
    target=$($cc -dumpmachine 2>/dev/null) || target=
    arch=${target%%-*}
    case $arch in
        x86_64|aarch64) ;;
        *)
            fail "$cc targets '${target:-nothing}', not x86_64 or aarch64"
            continue
            ;;
    esac
    echo "--- $cc ($target)"

    # $cc is split into words, as configure splits CC.
    cflags="-O0 -ffreestanding -fno-stack-protector"
    if ! $cc $cflags -fno-pie -c "$work/hello.c" -o "$dir/$arch-hello.o" ||
            ! $cc $cflags -fno-pie -c "$work/big.c" -o "$dir/$arch-big.o" ||
            ! $cc $cflags -fno-pie -c "$work/tls.c" -o "$dir/$arch-tls.o" ||
            ! $cc $cflags -fPIE -c "$work/tls.c" -o "$dir/$arch-tls-pie.o"; then
        fail "$cc did not compile the fixtures"
        continue
    fi
    exe="-nostdlib -static -no-pie"
    hello=$dir/$arch-hello.o

    for script in lightc.ld lightc-tiny.ld; do
        link "$script" $exe -T "$dir/$script" "$hello" &&
            expect "$script" pass "$script as it is"
    done
    if [ "$arch" = aarch64 ]; then
        old_align=page old_script="no-gnu-stack page" page=65536
    else
        old_align=pass old_script=no-gnu-stack page=4096
    fi
    link old-align $exe -T "$dir/old-align.ld" "$hello" &&
        expect old-align "$old_align" "lightc.ld with lightc 0.1.0's ALIGN(4096) before .data"
    if link old-align-big $exe -T "$dir/old-align.ld" "$dir/$arch-big.o"; then
        # The two LOADs must start on different pages, or this fixture
        # would show nothing old-align does not.
        starts=$("$READELF" -lW "$dir/$arch-old-align-big" | awk '$1 == "LOAD" { print $3 }' |
            while read -r vaddr; do echo $((vaddr / page)); done | sort -u | wc -l)
        if [ $((starts)) -eq 2 ]; then
            expect old-align-big "$old_align" "old-align with 72 KiB of read-only data"
        else
            fail "$arch-old-align-big: its LOADs do not start on two different pages:"
            "$READELF" -lW "$dir/$arch-old-align-big" | sed 's/^/    /'
        fi
    fi
    if link old-align-edge $exe -T "$dir/old-align-edge.ld" "$hello"; then
        # The R E LOAD, the first, must end right where the RW LOAD starts,
        # on a 64 KiB boundary, or this fixture would show nothing
        # old-align does not.
        if "$READELF" -lW "$dir/$arch-old-align-edge" | awk '$1 == "LOAD" { print $3, $6 }' | {
                read -r text_vaddr text_memsz && read -r data_vaddr data_memsz &&
                    [ $((text_vaddr + text_memsz)) -eq $((data_vaddr)) ] &&
                    [ $((data_vaddr % 65536)) -eq 0 ]
                }; then
            expect old-align-edge pass "old-align with the R E LOAD ending where the RW LOAD starts"
        else
            fail "$arch-old-align-edge: its R E LOAD does not end on the 64 KiB boundary its RW LOAD starts on:"
            "$READELF" -lW "$dir/$arch-old-align-edge" | sed 's/^/    /'
        fi
    fi
    link no-align $exe -T "$dir/no-align.ld" "$hello" &&
        expect no-align page "lightc.ld with no ALIGN before .data"
    link no-gnu-stack $exe -T "$dir/no-gnu-stack.ld" "$hello" &&
        expect no-gnu-stack no-gnu-stack "lightc.ld without PT_GNU_STACK"
    link old-script $exe -T "$dir/old-script.ld" "$hello" &&
        expect old-script "$old_script" "both, as lightc 0.1.0's lightc.ld"
    link rwx $exe -T "$dir/rwx.ld" "$hello" &&
        expect rwx rwx "lightc.ld with an RWX text PT_LOAD"
    link execstack $exe -Wl,-z,execstack "$hello" &&
        expect execstack exec-stack "the default script and -z execstack"
    link tls $exe "$dir/$arch-tls.o" &&
        expect tls tls "thread_local, the default script"
    link pie-tls -nostdlib -pie "$dir/$arch-tls-pie.o" &&
        expect pie-tls tls "thread_local in a PIE"
    link static-pie-tls -nostdlib -static-pie "$dir/$arch-tls-pie.o" &&
        expect static-pie-tls tls "thread_local in a static PIE"
    # As configure's cc_shared rule builds build/test_plugin.so.
    if link shared-tls -shared -fPIC -nostdlib "$work/lib.c"; then
        if "$READELF" -lW "$dir/$arch-shared-tls" | grep -q '^ *TLS '; then
            expect shared-tls pass "a shared object with PT_TLS"
        else
            fail "$arch-shared-tls has no PT_TLS header, so it shows nothing"
        fi
    fi
    link shared-execstack -shared -fPIC -nostdlib -Wl,-z,execstack "$work/lib.c" &&
        expect shared-execstack exec-stack "a shared object with -z execstack"
    link shared-rwx -shared -fPIC -nostdlib -T "$dir/shared-rwx.ld" "$work/plain.c" &&
        expect shared-rwx rwx "a shared object with an RWX text PT_LOAD"
    link shared-no-gnu-stack -shared -fPIC -nostdlib -T "$dir/shared-no-gnu-stack.ld" "$work/plain.c" &&
        expect shared-no-gnu-stack no-gnu-stack "a shared object without PT_GNU_STACK"
done

# One run over the whole directory: each fixture as alone, and the
# objects and linker scripts skipped.
echo "--- every fixture in one run"
checks=$((checks + 1))
elfcheck .
awk '
    /^FAIL: \.\// {
        name = substr($2, 3)
        sub(/:$/, "", name)
        rule = $3
        sub(/:$/, "", rule)
        print name, rule
    }
    /^ok: \.\// { print substr($2, 3), "pass" }
' "$work/out" | sort > "$work/got"
sort "$work/expected" > "$work/want"
if [ "$status" -eq 1 ] && cmp -s "$work/want" "$work/got"; then
    echo "ok: one run over the directory reports each fixture as it was reported alone:"
    grep '^elfcheck: ' "$work/out" | sed 's/^/    /'
else
    fail "one run over every fixture: exit $status, and its report differs (-: alone, +: in one run):"
    diff "$work/want" "$work/got" | sed 's/^/    /' || true
    sed 's/^/    /' "$work/out"
fi

# Nothing to check must fail, so a wrong path cannot pass.
echo "--- no binary"
checks=$((checks + 1))
mkdir "$dir/no-binary"
echo "not an ELF file" > "$dir/no-binary/notes.txt"
if ls "$dir"/*-hello.o > /dev/null 2>&1; then
    cp "$dir"/*-hello.o "$dir/no-binary/"
    $AR rc "$dir/no-binary/libfixture.a" "$dir"/*-hello.o
fi
elfcheck no-binary
if [ "$status" -eq 1 ] && grep -q '^FAIL: no ELF executable or shared object in no-binary$' "$work/out" &&
        ! grep -q '^ok: ' "$work/out"; then
    echo "ok: a directory with only objects, an archive and a text file fails:"
    sed 's/^/    /' "$work/out"
else
    fail "a directory with no binary: exit $status:"
    sed 's/^/    /' "$work/out"
fi

good=$(awk '$2 == "pass" { print $1; exit }' "$work/expected")
broken=$(awk '$2 != "pass" { print $1; exit }' "$work/expected")

checks=$((checks + 1))
if [ -n "$good" ]; then
    elfcheck "$good" missing
    if [ "$status" -eq 1 ] && grep -q '^FAIL: missing: no such file or directory$' "$work/out"; then
        echo "ok: a path that does not exist fails, beside a good one:"
        sed 's/^/    /' "$work/out"
    else
        fail "a path that does not exist: exit $status:"
        sed 's/^/    /' "$work/out"
    fi
else
    fail "no fixture was built, so the missing-path check has no good file"
fi

# A file elfcheck cannot read may be a binary that breaks a rule, so it
# must fail, here the copy of a bad fixture beside a good one.
if [ -n "$good" ] && [ -n "$broken" ]; then
    cp "$dir/$broken" "$dir/unreadable"
    chmod 000 "$dir/unreadable"
    if [ -r "$dir/unreadable" ]; then
        echo "skip: this user reads a file with mode 000, so the unreadable file check cannot run"
    else
        checks=$((checks + 1))
        elfcheck "$good" unreadable
        if [ "$status" -eq 1 ] && grep -q '^FAIL: unreadable: unreadable: ' "$work/out" &&
                grep -q "^ok: $good " "$work/out"; then
            echo "ok: a file elfcheck cannot read ($broken, mode 000) fails, beside a good one:"
            sed 's/^/    /' "$work/out"
        else
            fail "a file elfcheck cannot read: exit $status:"
            sed 's/^/    /' "$work/out"
        fi
    fi
else
    fail "no good and bad fixture were built, so the unreadable file check cannot run"
fi

if [ "$wrong" -gt 0 ]; then
    echo "FAIL: $wrong of the checks of ci/elfcheck.sh went wrong"
    exit 1
fi
echo "PASS: ci/elfcheck.sh reported all $checks checks as expected"

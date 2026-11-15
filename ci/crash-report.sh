#!/bin/sh
# lightc — kernel reports of crashed test programs
#
# Reads the kernel log on stdin and prints its reports of the programs of a
# build directory that a fault or trap killed: a suite, a regress program
# or an example that crashed or hit a ubsan trap. Then it prints the
# function and source line of each address in those reports, so a CI log
# shows where a program died without the binary at hand.
#
# The kernel logs a program that a CPU exception killed, with SIGSEGV,
# SIGBUS, SIGILL, SIGTRAP or SIGFPE and no handler for that signal, while
# debug.exception-trace is 1, its default:
#
#   x86_64   one line with the faulting address, as in
#            "traps: test_string[1234] trap invalid opcode ip:401a2b sp:...
#            error:0 in test_string[401000+1d000]" for a ubsan trap, or
#            "test_heap[1234]: segfault at 0 ip 0000000000402c1d ...",
#            which a "Code:" line of the bytes at ip follows;
#   aarch64  "test_heap[1234]: unhandled exception: ... in
#            test_heap[400000+30000]" and then the registers, among them
#            "pc : 0000000000401a2c" and "lr : 0000000000401a10".
#
# Some kernels put the address's offset in the file first, as in
# "test_string[1a2b,401000+1d000]". " in ..." is missing when the address
# is in no file, as after a jump to address 0.
#
# A signal that another process sent, or that the program raised itself,
# leaves no report. So a suite that ran out of time gets none, since
# timeout(1) stops it with SIGTERM, or SIGKILL when that does not end it,
# and the script says the log has no report.
#
# lightc's binaries are static and not PIE, so addr2line takes those
# addresses as they are. It names the source line only in a build with
# symbols, debug or ubsan; release and tiny binaries are stripped.
#
# Usage: sudo dmesg | sh ci/crash-report.sh [BUILDDIR]
# BUILDDIR defaults to build, relative to the source root; its regress/
# directory is searched too. ADDR2LINE defaults to addr2line. It exits 0
# whether or not there was a report.

set -eu

# CDPATH= keeps cd from looking up "ci/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."

dir=${1:-build}
ADDR2LINE=${ADDR2LINE:-addr2line}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# dmesg starts each line with "[seconds.micros] ", and some kernels add a
# "[T1234]" caller id. A report starts with the program's name and pid. The
# name is the kernel's copy, cut to 15 bytes (test_thread_sync is
# test_thread_syn), so the full name is taken from " in NAME[" when the
# address is in the program's own file. The awk script prints each report
# of a binary in $dir or $dir/regress, with the lines that follow it, and
# writes "binary address kind" for each address to $work/addrs.
sed 's/^\(\[[^]]*\] *\)*//' | LC_ALL=C awk -v dir="$dir" -v addrs="$work/addrs" '
# The executable in dir or dir/regress called name, or whose name starts
# with a 15-byte name. name has only [A-Za-z0-9_.+-], so the shell takes
# it as it is.
function binary(name,    cmd, f) {
    cmd = "for f in \"" dir "/" name "\" \"" dir "/regress/" name "\""
    if (length(name) == 15) cmd = cmd " \"" dir "/" name "\"* \"" dir "/regress/" name "\"*"
    cmd = cmd "; do if [ -f \"$f\" ] && [ -x \"$f\" ]; then echo \"$f\"; break; fi; done"
    f = ""
    cmd | getline f
    close(cmd)
    return f
}
match($0, /^(traps: )?[A-Za-z0-9_.+-]+\[[0-9]+\]:? /) && /segfault at|trap |general protection|unhandled / {
    name = substr($0, RSTART, RLENGTH)
    sub(/^traps: /, "", name)
    sub(/\[.*/, "", name)
    if (match($0, / in [A-Za-z0-9_.+-]+\[([0-9a-f]+,)?[0-9a-f]+\+[0-9a-f]+\]/)) {
        file = substr($0, RSTART + 4, RLENGTH - 4)
        sub(/\[.*/, "", file)
        if (index(file, name) == 1) name = file
    }
    bin = binary(name)
    if (bin == "") next
    print ""
    print
    reports++
    if (match($0, / ip[: ][0-9a-f]+ /)) {
        print bin, substr($0, RSTART + 4, RLENGTH - 5), "ip" > addrs
    }
    next
}
bin != "" && /^(Code: |CPU: |Hardware name: |pstate: |pc : |lr : |sp : |pmr_save: |x[0-9]+ *: )/ {
    print
    if ($1 == "pc" || $1 == "lr") print bin, $3, $1 > addrs
    next
}
{ bin = "" }
END {
    if (!reports) print "The kernel log has no report of a program in " dir "."
}
'

[ -s "$work/addrs" ] || exit 0

echo
echo "Where they stopped ($ADDR2LINE):"
while read -r bin addr kind; do
    addr=0x$addr
    # lr holds the return address. The call is the instruction before it,
    # 4 bytes back on aarch64, and its line is the one that matters.
    if [ "$kind" = lr ]; then
        addr=$(printf '0x%x' $((addr - 4)))
        kind="lr-4"
    fi
    printf '%s %s %s: ' "$bin" "$kind" "$addr"
    "$ADDR2LINE" -f -i -p -e "$bin" "$addr" 2>&1 || true
done < "$work/addrs"

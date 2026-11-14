#!/bin/sh
# lightc — test suite runner
#
# Runs each SUITE, the path of a suite binary such as build/test_heap, in
# turn, after a "--- SUITE ---" header, and passes its output through. A
# suite that fails does not stop the run, so one run shows every failure.
# ninja test runs this with the suites its test edge lists. Linux only: it
# reads /proc.
#
# Each suite runs under GNU timeout(1), in a process group of its own, with
# stdin on /dev/null. A suite still running after LIGHTC_TEST_TIMEOUT
# seconds (default 120; 0 for no limit) is sent SIGTERM, and SIGKILL 10 s
# later, together with the processes it started, and fails. Processes the
# suite leaves running in that group when it ends are killed. A process
# that left the group (with setsid, say) and still holds the suite's output
# open 5 s after the suite ended is killed too, and the suite fails. A slow
# reader of the runner's output, such as a pager or a slow terminal, is
# waited for.
#
# Without timeout(1), or with one that lacks -k, there is no time limit,
# each suite starts with SIGINT and SIGQUIT ignored, as a shell starts any
# background command, and it shares the runner's process group, so ^Z
# stops it too. Processes it leaves running are killed only when they
# still hold its output 5 s after it ended.
#
# A suite fails if it exits non-zero or prints no "RESULT: PASS" line. A
# suite that passes with no test passed skips every test, as test_async and
# test_lightio do on a kernel without io_uring. That fails the run too,
# unless LIGHTC_TEST_ALLOW_SKIP names the suite, as in
# LIGHTC_TEST_ALLOW_SKIP="test_async test_lightio".
#
# The summary at the end counts the suites and tests that passed, failed and
# skipped, and names each suite that failed or skipped every test.
#
# SIGHUP, SIGINT, SIGQUIT, SIGPIPE or SIGTERM stops the run: the suite is
# sent SIGTERM and killed 10 s later if it ignores that, output still on
# its way to a slow reader is dropped after 2 s, the temp dir is removed
# and the runner exits. A second signal kills the suite and what it started
# at once. With timeout(1), ^Z stops the runner but not the suite, which is
# in its own process group: it blocks once its output fills the FIFO, and
# its time limit keeps running. SIGPIPE is noticed the next time the
# runner or tee writes, so a quiet suite runs on until it prints or ends.
#
# Usage: tests/run.sh SUITE...
# Exit status: 0 if the run passed, 1 if it failed, 2 for a usage error or
# when the temp dir or a FIFO cannot be made, and 128 plus the signal
# number when a signal stops it.

set -fu

# job_pid is the background job running the suite: timeout(1), or the suite
# itself without it. timeout(1) makes the suite's process group, whose id is
# its own process id. Anything still in that group once the suite has ended
# would hold the FIFO open; kill it.
job_pid= tee_pid= tmp= timeout=
kill_leftovers() {
    if [ -n "$timeout" ]; then
        kill -s KILL -- -"$job_pid" 2>/dev/null
    fi
}

# The suite and tee run in the background, so that a signal is handled at
# once rather than when the suite ends: a signal with a trap ends the wait
# for them. The trap notes the signal and signals the suite; the loop then
# calls finish(). Nothing waits inside the trap, as bash can then wait
# forever. SIGPIPE comes when the runner's output is closed, as in
# "ninja test | head". The traps are set before anything else, so a signal
# from here to the last check ends the run as a stop; one that comes after
# that, once the summary is out, is ignored. bash drops a trapped SIGINT
# that arrives while it waits for a command substitution, so the runner
# has none. bash still loses a few: a SIGQUIT that comes while bash itself
# starts, before this script runs, and in testing one SIGHUP in about 2,000
# sent at the start of a run.
stopped= stopped_status= stops=0
stop() {
    stops=$((stops + 1))
    if [ -z "$stopped" ]; then
        stopped=$1 stopped_status=$2
    fi
    if [ "$stops" -eq 1 ]; then
        if [ -n "$job_pid" ]; then
            kill -s TERM "$job_pid" 2>/dev/null
        fi
    else
        if [ -n "$job_pid" ]; then
            kill_leftovers
            kill -s KILL "$job_pid" 2>/dev/null
        fi
        if [ -n "$tee_pid" ]; then
            kill -s KILL "$tee_pid" 2>/dev/null
        fi
    fi
}
trap 'stop HUP 129' HUP
trap 'stop INT 130' INT
trap 'stop QUIT 131' QUIT
trap 'stop PIPE 141' PIPE
trap 'stop TERM 143' TERM

if [ $# -eq 0 ]; then
    printf 'usage: %s SUITE...\n' "$0" >&2
    exit 2
fi

limit=${LIGHTC_TEST_TIMEOUT:-120}
case $limit in
    '' | *[!0-9]* | ?????????*)
        printf "%s: LIGHTC_TEST_TIMEOUT is '%s', not a whole number of seconds below 100000000\n" "$0" "$limit" >&2
        exit 2
        ;;
esac

# timeout(1) runs the suite even with no limit: a limit of 0 sets none, and
# timeout(1) still gives the suite its process group and default handling
# of SIGINT and SIGQUIT.
timeout=
if timeout -k 1 10 true >/dev/null 2>&1; then
    timeout="timeout -k 10 $limit"
elif [ "$limit" -ne 0 ]; then
    printf '%s: no timeout(1) that takes -k, so the suites run with no time limit\n' "$0" >&2
fi

# The runner polls in tenths of a second, or in seconds with a sleep(1)
# that takes only whole numbers.
tick=0.1 tick_tenths=1
if ! sleep 0.01 2>/dev/null; then
    tick=1 tick_tenths=10
fi

# Sets now to the seconds since boot, which no clock change moves, without
# a command substitution.
now=0
read_uptime() {
    read -r now _ < /proc/uptime || now=0
    now=${now%.*}
}

# Waits until child process $1 is gone. A signal ends a wait early, and
# bash can forget that it has already reaped a child, so the process table
# decides; a pid this shell cannot wait for (127) ends the loop.
reap() {
    while kill -0 "$1" 2>/dev/null; do
        wait "$1" 2>/dev/null
        if [ $? -eq 127 ]; then
            return
        fi
    done
}

# True while child process $1 runs. kill -0 also succeeds for a child that
# has exited but has not been waited for, so this reads its state.
running() {
    state=
    [ -r "/proc/$1/stat" ] || return 1
    read -r state 2>/dev/null < "/proc/$1/stat" || return 1
    state=${state##*) }
    [ "${state%% *}" != Z ]
}

# Kills every process but tee that still has this suite's FIFO open, and
# lists their pids in holders; true if there were any. The FIFO has been
# unlinked, so their fd links read "<tmp>/fifo (deleted)"; the last part of
# tmp is lightc-test.PID.N, so it holds no pattern characters.
kill_holders() {
    holders=
    find /proc -mindepth 3 -maxdepth 3 -path '/proc/[0-9]*/fd/*' \
        -lname "*/${tmp##*/}/fifo*" > "$tmp/holders" 2>/dev/null
    while IFS=/ read -r _ _ pid _; do
        case " $tee_pid $holders " in
            *" $pid "*) continue ;;
        esac
        holders="$holders $pid"
        kill -s KILL "$pid" 2>/dev/null
    done < "$tmp/holders"
    [ -n "$holders" ]
}

# Waits for tee to copy the rest of the suite's output, which it has done
# once no process holds the FIFO open for writing and it has written what
# it read. It can outlast the suite for two reasons: a process the suite
# started left its process group and holds the FIFO, or the reader of the
# runner's output is slow and tee waits to write. After $1 tenths of a
# second (2 s once a signal has come), and every 2 s after that, the runner
# looks for the first in /proc: such processes are killed, tee then copies
# what they wrote, and the suite fails. The second is waited out, unless a
# signal has come, when tee is stopped. tee_status gets tee's exit status,
# or "held" when processes held the FIFO.
finish_tee() {
    grace=$1 tenths=0 held=
    while running "$tee_pid"; do
        if [ -n "$stopped" ] && [ "$grace" -gt 20 ]; then
            grace=20
        fi
        if [ "$tenths" -ge "$grace" ]; then
            if kill_holders; then
                held="$held$holders"
            elif [ -n "$stopped" ]; then
                kill -s TERM "$tee_pid" 2>/dev/null
            fi
            grace=$((tenths + 20))
        fi
        sleep "$tick"
        tenths=$((tenths + tick_tenths))
    done
    wait "$tee_pid" 2>/dev/null
    tee_status=$?
    if [ -n "$held" ]; then
        tee_status=held
    fi
    tee_pid=
}

# Stops the run after a signal: stops the suite (timeout(1) passes SIGTERM
# on to every process the suite started), kills what it left, lets tee see
# end of file, removes the temp dir and exits.
finish() {
    trap '' PIPE
    if [ -n "$job_pid" ]; then
        kill -s TERM "$job_pid" 2>/dev/null
        # A job forked a moment ago is a copy of this shell until it starts
        # the suite. It catches a signal with the trap handler it inherited
        # and loses it when it resets its traps (dash), so SIGTERM goes again
        # after a tick, and on every tick while the job is, or just was, a
        # copy of this shell. Without timeout(1), nothing else kills a suite
        # that ignores SIGTERM, so it gets SIGKILL after 10 s.
        waited=0 was_shell=1
        while running "$job_pid"; do
            sleep "$tick"
            waited=$((waited + tick_tenths))
            running "$job_pid" || break
            is_shell=
            if [ "/proc/$job_pid/exe" -ef "/proc/$$/exe" ]; then
                is_shell=1
            fi
            if [ "$waited" -le 20 ] && [ -n "$is_shell$was_shell" ]; then
                kill -s TERM "$job_pid" 2>/dev/null
            elif [ -z "$timeout" ] && [ "$waited" -ge 100 ]; then
                kill -s KILL "$job_pid" 2>/dev/null
            fi
            was_shell=$is_shell
        done
        reap "$job_pid"
        kill_leftovers
    fi
    exec 4<&- 5>&-
    if [ -n "$tee_pid" ]; then
        finish_tee 20
    fi
    trap '' HUP INT QUIT TERM
    if [ -n "$tmp" ]; then
        rm -rf "$tmp"
    fi
    printf '%s: stopped by SIG%s\n' "$0" "$stopped" >&2
    exit "$stopped_status"
}
check() {
    if [ -n "$stopped" ]; then
        finish
    fi
}

# The temp dir holds each suite's FIFO and the copy of its output. The
# runner names it itself and makes it with mkdir, which fails on any name
# that exists, so it never uses a dir or a link someone else made, and it
# knows the name before the dir exists: a signal that kills mkdir still
# ends the run as a stop with nothing left behind. (mktemp would need a
# command substitution, in which bash can lose a ^C.) The EXIT trap ignores
# signals so that one cannot kill its rm.
trap 'trap "" HUP INT QUIT TERM; if [ -n "$tmp" ]; then rm -rf "$tmp"; fi' EXIT
for n in 0 1 2 3 4 5 6 7 8 9; do
    tmp=${TMPDIR:-/tmp}/lightc-test.$$.$n
    mkdir -m 700 "$tmp" 2>/dev/null
    made=$?
    # A mkdir killed by a signal may have made the dir; one that failed
    # did not, and the name is someone else's.
    if [ "$made" -eq 0 ] || { [ -n "$stopped" ] && [ "$made" -gt 128 ]; }; then
        break
    fi
    tmp=
    if [ -n "$stopped" ]; then
        break
    fi
done
check
if [ -z "$tmp" ]; then
    printf '%s: cannot make a temp dir in %s\n' "$0" "${TMPDIR:-/tmp}" >&2
    exit 2
fi

allowed_to_skip() {
    for allowed in ${LIGHTC_TEST_ALLOW_SKIP:-}; do
        if [ "${allowed##*/}" = "$1" ]; then
            return 0
        fi
    done
    return 1
}

suites=0 suites_passed=0 suites_failed=0 suites_skipped=0
tests_run=0 tests_passed=0 tests_failed=0 tests_skipped=0
failed_suites= skipped_suites= run_failed=0

for suite in "$@"; do
    check
    name=${suite##*/}
    # A path with no slash would be looked up in PATH.
    case $suite in
        */*) path=$suite ;;
        *) path=./$suite ;;
    esac
    printf '%s\n' "--- $suite ---"
    # The suite's output goes through a FIFO to tee, which shows it as it
    # comes and keeps a copy to read the suite's summary from. The runner
    # opens both ends itself and hands them to tee and the suite, so that
    # neither waits in open() for the other: with the FIFO open read-write
    # on fd 3, which on Linux never waits, opening the read end (fd 4) and
    # the write end (fd 5) returns at once. tee sees end of file when the
    # suite, what it started and the runner's fd 5, closed once the suite
    # is running, are all gone. Each suite gets a new FIFO, unlinked once
    # open, so a process an earlier suite left behind cannot write into it.
    if ! mkfifo "$tmp/fifo"; then
        check
        printf '%s: cannot make %s\n' "$0" "$tmp/fifo" >&2
        exit 2
    fi
    exec 3<> "$tmp/fifo" 4< "$tmp/fifo" 5> "$tmp/fifo" 3<&-
    rm -f "$tmp/fifo"
    tee "$tmp/output" <&4 4<&- 5>&- &
    tee_pid=$!
    exec 4<&-
    check
    # The elapsed time tells a timeout from a suite that exits 124 itself.
    read_uptime
    start=$now
    $timeout "$path" >&5 2>&1 5>&- &
    job_pid=$!
    exec 5>&-
    # A signal between the check below and the wait does not end the wait;
    # only the trap's SIGTERM to the job does, and the job loses that while
    # it is still a copy of this shell (see finish). So wait, briefly and
    # with builtins only, until the job runs timeout(1) or the suite.
    spins=0
    while [ "$spins" -lt 2000 ] && [ "/proc/$job_pid/exe" -ef "/proc/$$/exe" ]; do
        spins=$((spins + 1))
    done
    check
    # The redirection keeps the shell's own report of a suite killed by a
    # signal out of the output; the summary gives the reason.
    wait "$job_pid" 2>/dev/null
    status=$?
    check
    read_uptime
    end=$now
    kill_leftovers
    job_pid=
    finish_tee 50
    check
    if [ "$tee_status" = 141 ]; then
        # tee could not write to the runner's output, so it has been closed.
        stop PIPE 141
        check
    fi
    # A suite whose output does not end in a newline would have the next
    # header run on from its last line.
    if [ -s "$tmp/output" ]; then
        tail -c 1 "$tmp/output" > "$tmp/last"
        if ! read -r _ < "$tmp/last"; then
            echo
        fi
    fi

    # The suite's own summary is the last one in its output.
    awk '
        /^Tests run:/     { run = $3 }
        /^Tests passed:/  { passed = $3 }
        /^Tests failed:/  { failed = $3 }
        /^Tests skipped:/ { skipped = $3 }
        /^RESULT:/        { result = $2 }
        END { print run + 0, passed + 0, failed + 0, skipped + 0, (result == "" ? "none" : result) }
    ' "$tmp/output" > "$tmp/summary"
    check
    run=0 passed=0 failed=0 skipped=0 result=none
    read -r run passed failed skipped result < "$tmp/summary"
    suites=$((suites + 1))
    tests_run=$((tests_run + ${run:-0}))
    tests_passed=$((tests_passed + ${passed:-0}))
    tests_failed=$((tests_failed + ${failed:-0}))
    tests_skipped=$((tests_skipped + ${skipped:-0}))

    case $tee_status in
        0) problem= ;;
        held) problem="processes it started (pid$held) still held its output open 5 s after it ended, so they were killed" ;;
        *) problem="tee exited with status $tee_status, so its output may be incomplete" ;;
    esac

    if [ "$status" -ne 0 ] || [ "$result" != PASS ] || [ -n "$problem" ]; then
        suites_failed=$((suites_failed + 1))
        run_failed=1
        # timeout(1) exits 124 when the limit ran out, or 137 when the
        # suite then needed SIGKILL; a suite can exit 124 itself, but not
        # after the limit.
        timed_out=
        if [ -n "$timeout" ] && [ "$limit" -ne 0 ] &&
            [ $((end - start)) -ge "$limit" ]; then
            case $status in
                124) timed_out="timed out after $limit s" ;;
                137) timed_out="timed out after $limit s and ignored SIGTERM, so it was killed" ;;
            esac
        fi
        if [ -n "$timed_out" ]; then
            why=$timed_out
        elif [ "$status" -gt 128 ] && [ "$status" -le 192 ]; then
            why="killed by signal $((status - 128))"
        elif [ "$status" -ne 0 ]; then
            why="exit status $status"
        elif [ "$result" != PASS ]; then
            why="exit status 0 but no RESULT: PASS line"
        else
            why=
        fi
        if [ -n "$why" ] && [ "$result" != none ]; then
            why="$why, $failed of $run tests failed"
        fi
        if [ -n "$problem" ]; then
            why="${why:+$why; }$problem"
        fi
        failed_suites="$failed_suites  $name: $why
"
    elif [ "$passed" -eq 0 ]; then
        suites_skipped=$((suites_skipped + 1))
        if [ "$run" -eq 0 ]; then
            why="ran no tests"
        else
            why="skipped all $run tests"
        fi
        if allowed_to_skip "$name"; then
            why="$why, allowed by LIGHTC_TEST_ALLOW_SKIP"
        else
            run_failed=1
            why="$why; where that is expected, list it in LIGHTC_TEST_ALLOW_SKIP"
        fi
        skipped_suites="$skipped_suites  $name: $why
"
    else
        suites_passed=$((suites_passed + 1))
    fi
done

check
echo
echo "========================================"
echo "Suites: $suites run, $suites_passed passed, $suites_failed failed, $suites_skipped skipped every test"
echo "Tests:  $tests_run run, $tests_passed passed, $tests_failed failed, $tests_skipped skipped"
if [ -n "$failed_suites" ]; then
    echo "Failed:"
    printf '%s' "$failed_suites"
fi
if [ -n "$skipped_suites" ]; then
    echo "Skipped every test:"
    printf '%s' "$skipped_suites"
fi
echo "========================================"
if [ "$run_failed" -ne 0 ]; then
    echo "SUITES: FAIL"
else
    echo "SUITES: PASS"
fi
check
exit "$run_failed"

#!/bin/sh
# Collect, validate, and publish LCOV runtime coverage for RFC L2 tests.
set -eu

WORKDIR=${RFC_WORKDIR:-$(pwd)}
COVERAGE_DIR=/tmp/l2_coverage
TRACEFILE="$COVERAGE_DIR/coverage.info"
BASELINE="$COVERAGE_DIR/baseline.info"
RUNTIME="$COVERAGE_DIR/runtime.info"

mkdir -p "$COVERAGE_DIR"

cleanup_failed=0
cleanup_message() { echo "Coverage collection incomplete: $1" >&2; cleanup_failed=1; }

lcov_capture() {
    if [ "${RFC_L2_COVERAGE:-0}" != "1" ]; then
        return 0
    fi

    rm -f "$RUNTIME" "$COVERAGE_DIR/merged.info" "$COVERAGE_DIR/scoped.info" "$TRACEFILE"

    gcda_count=$(find "$WORKDIR" -name '*.gcda' -type f | wc -l)
    if [ "$gcda_count" -eq 0 ]; then
        cleanup_message "no gcda counters were produced by RFC L2 execution"
        return 1
    fi

    if ! lcov --capture --ignore-errors gcov,mismatch,source,negative,unused --directory "$WORKDIR" --base-directory "$WORKDIR" --output-file "$RUNTIME"; then
        cleanup_message "lcov runtime capture failed"
        return 1
    fi

    if [ ! -s "$BASELINE" ] || [ ! -s "$RUNTIME" ]; then
        cleanup_message "baseline or runtime LCOV trace is missing"
        return 1
    fi

    if ! lcov --add-tracefile "$BASELINE" --add-tracefile "$RUNTIME" --ignore-errors mismatch,negative,unused --output-file "$COVERAGE_DIR/merged.info"; then
        cleanup_message "lcov baseline/runtime merge failed"
        return 1
    fi

    if ! lcov --remove "$COVERAGE_DIR/merged.info" '/usr/*' '*/gtest/*' '*/mocks/*' '*/rfcapi/rfcapi_test.cpp' --ignore-errors unused --output-file "$COVERAGE_DIR/scoped.info"; then
        cleanup_message "lcov component filtering failed"
        return 1
    fi

    if ! lcov --extract "$COVERAGE_DIR/scoped.info" '*/rfcMgr/*' '*/rfcapi/*' '*/tr181api/*' '*/utils/*' --ignore-errors unused --output-file "$TRACEFILE"; then
        cleanup_message "lcov component extraction failed"
        return 1
    fi

    if [ ! -s "$TRACEFILE" ]; then
        cleanup_message "no component source records in LCOV trace"
        return 1
    fi

    if ! grep -Eq '^DA:[0-9]+,[1-9][0-9]*([,]|$)' "$TRACEFILE"; then
        cleanup_message "LCOV trace has no positive line execution evidence"
        return 1
    fi

    if ! genhtml "$TRACEFILE" --ignore-errors source --output-directory "$COVERAGE_DIR/html" --title 'RFC L2 Runtime Coverage'; then
        cleanup_message "genhtml failed"
        return 1
    fi
}

case "${1:-prepare}" in
    prepare)
        if [ "${RFC_L2_COVERAGE:-0}" = "1" ]; then
            find "$WORKDIR" -name '*.gcda' -delete
            rm -f "$BASELINE" "$RUNTIME" "$TRACEFILE"
            if ! lcov --zerocounters --directory "$WORKDIR" --ignore-errors unused,gcov,mismatch,source,negative; then
                :
            fi
            if ! lcov --capture --initial --ignore-errors gcov,mismatch,source,negative,unused --directory "$WORKDIR" --base-directory "$WORKDIR" --output-file "$BASELINE"; then
                cleanup_message "failed to create initial LCOV baseline"
                exit 1
            fi
            if [ ! -s "$BASELINE" ]; then
                cleanup_message "initial LCOV baseline is empty"
                exit 1
            fi
            gcno_count=$(find "$WORKDIR" -name '*.gcno' -type f | wc -l)
            if [ "$gcno_count" -eq 0 ]; then
                cleanup_message "coverage build produced no gcno build notes"
                exit 1
            fi
        fi
        ;;
    collect)
        lcov_capture
        exit "$cleanup_failed"
        ;;
    *) echo "usage: $0 prepare|collect" >&2; exit 2;;
esac


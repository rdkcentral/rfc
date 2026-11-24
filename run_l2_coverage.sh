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
    if [ "${RFC_L2_COVERAGE:-0}" = "1" ]; then
        lcov --capture --directory "$WORKDIR" --base-directory "$WORKDIR" --output-file "$RUNTIME"
        if [ ! -s "$BASELINE" ] || [ ! -s "$RUNTIME" ]; then
            cleanup_message "baseline or runtime LCOV trace is missing"
            return 1
        fi
        lcov --add-tracefile "$BASELINE" --add-tracefile "$RUNTIME" --output-file "$COVERAGE_DIR/merged.info"
        lcov --remove "$COVERAGE_DIR/merged.info" '/usr/*' '*/gtest/*' '*/mocks/*' --output-file "$COVERAGE_DIR/scoped.info"
        lcov --extract "$COVERAGE_DIR/scoped.info" '*/rfcMgr/*' '*/rfcapi/*' '*/tr181api/*' '*/utils/*' --output-file "$TRACEFILE"
        if [ ! -s "$TRACEFILE" ]; then
            cleanup_message "no component source records in LCOV trace"
            return 1
        fi
        if ! grep -q '^DA:[0-9][^,]*,[1-9][0-9]*' "$TRACEFILE"; then
            cleanup_message "LCOV trace has no positive line execution evidence"
            return 1
        fi
        genhtml "$TRACEFILE" --output-directory "$COVERAGE_DIR/html" --title 'RFC L2 Runtime Coverage'
    fi
}

case "${1:-prepare}" in
    prepare)
        if [ "${RFC_L2_COVERAGE:-0}" = "1" ]; then
            find "$WORKDIR" -name '*.gcda' -delete
            lcov --capture --directory "$WORKDIR" --base-directory "$WORKDIR" --output-file "$BASELINE"
        fi
        ;;
    collect)
        lcov_capture
        exit "$cleanup_failed"
        ;;
    *) echo "usage: $0 prepare|collect" >&2; exit 2;;
esac


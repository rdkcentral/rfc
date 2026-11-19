#!/bin/sh

set -u

REPO_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
COVERAGE_DIR=/tmp/rfc_l2_coverage
RESULT_DIR=/tmp/rfc_test_report
BASELINE_TRACE="$COVERAGE_DIR/baseline.info"
RUNTIME_TRACE="$COVERAGE_DIR/runtime.info"
MERGED_TRACE="$COVERAGE_DIR/coverage.info"

log_error() {
    printf 'ERROR: %s\n' "$*" >&2
}

reset_counters() {
    for source_root in rfcMgr rfcapi tr181api utils; do
        find "$REPO_DIR/$source_root" -type f -name '*.gcda' -exec rm -f {} \; || return 1
    done
}

extract_production_trace() {
    input_trace=$1
    output_trace=$2
    extracted_trace="$output_trace.extracted"
    lcov --extract "$input_trace" \
        "$REPO_DIR/rfcMgr/*" "$REPO_DIR/rfcapi/*" \
        "$REPO_DIR/tr181api/*" "$REPO_DIR/utils/*" \
        --output-file "$extracted_trace" || return 1
    lcov --remove "$extracted_trace" '*/gtest/*' '*/mocks/*' '*/test/*' \
        --output-file "$output_trace" || return 1
    rm -f "$extracted_trace"
}

capture_baseline() {
    if ! command -v lcov >/dev/null 2>&1; then
        log_error 'lcov is not installed'
        return 1
    fi
    rm -rf "$COVERAGE_DIR" "$RESULT_DIR"
    mkdir -p "$COVERAGE_DIR" "$RESULT_DIR" || return 1
    reset_counters || return 1
    lcov --capture --initial --directory "$REPO_DIR" --base-directory "$REPO_DIR" --output-file "$COVERAGE_DIR/baseline.raw.info" || return 1
    extract_production_trace "$COVERAGE_DIR/baseline.raw.info" "$BASELINE_TRACE" || return 1
    rm -f "$COVERAGE_DIR/baseline.raw.info"
    test -s "$BASELINE_TRACE"
}

wait_for_rfc_mgr() {
    wait_seconds=$1
    elapsed=0
    while pgrep -x rfcMgr >/dev/null 2>&1; do
        if [ "$elapsed" -ge "$wait_seconds" ]; then
            return 1
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 0
}

stop_rfc_mgrs() {
    if ! command -v pgrep >/dev/null 2>&1; then
        log_error 'pgrep is required to verify rfcMgr shutdown'
        return 1
    fi
    if wait_for_rfc_mgr 30; then
        return 0
    fi

    log_error 'rfcMgr did not exit naturally; requesting SIGTERM'
    for process_id in $(pgrep -x rfcMgr); do
        kill -TERM "$process_id" 2>/dev/null || true
    done
    if wait_for_rfc_mgr 30; then
        return 0
    fi

    log_error 'rfcMgr required SIGKILL; coverage is incomplete'
    for process_id in $(pgrep -x rfcMgr); do
        kill -KILL "$process_id" 2>/dev/null || true
    done
    return 1
}

validate_trace() {
    trace_file=$1
    awk '
        /^SF:/ {
            source = substr($0, 4)
            source_count++
            if (source ~ /\/rfcMgr\//) rfc_mgr = 1
            else if (source ~ /\/rfcapi\//) rfcapi = 1
            else if (source ~ /\/tr181api\//) tr181api = 1
            else if (source ~ /\/utils\//) utils = 1
            else foreign = 1
        }
        /^DA:/ {
            split(substr($0, 4), fields, ",")
            line_count++
            if (fields[2] + 0 > 0) hit_count++
        }
        /^FNDA:/ {
            split(substr($0, 6), fields, ",")
            if (source ~ /\/rfcMgr\/rfc_main\.cpp$/ && fields[2] == "main" && fields[1] + 0 > 0) main_hit = 1
        }
        END {
            printf "Sources: %d; lines: %d; executed lines: %d\n", source_count, line_count, hit_count
            if (source_count == 0 || line_count == 0 || hit_count == 0 || !main_hit || foreign || !rfc_mgr || !rfcapi || !tr181api || !utils) exit 1
        }
    ' "$trace_file"
}

capture_smoke_evidence() {
    if [ ! -s "$BASELINE_TRACE" ]; then
        log_error 'zero-hit baseline is missing for smoke validation'
        printf 'incomplete\n' > "$COVERAGE_DIR/smoke.status"
        return 1
    fi
    if lcov --capture --directory "$REPO_DIR" --base-directory "$REPO_DIR" --output-file "$COVERAGE_DIR/smoke.raw.info" && \
        extract_production_trace "$COVERAGE_DIR/smoke.raw.info" "$COVERAGE_DIR/smoke.runtime.info" && \
        lcov --add-tracefile "$BASELINE_TRACE" --add-tracefile "$COVERAGE_DIR/smoke.runtime.info" --output-file "$COVERAGE_DIR/smoke.info" && \
        validate_trace "$COVERAGE_DIR/smoke.info" > "$COVERAGE_DIR/smoke-validation.txt"; then
        printf 'complete\n' > "$COVERAGE_DIR/smoke.status"
        return 0
    fi
    printf 'incomplete\n' > "$COVERAGE_DIR/smoke.status"
    log_error 'smoke trace did not prove a hit in rfc_main.cpp main'
    return 1
}

write_test_counts() {
    python3 - "$RESULT_DIR" > "$COVERAGE_DIR/test-counts.txt" <<'PY'
import glob
import json
import os
import sys

result_dir = sys.argv[1]
counts = {"passed": 0, "failed": 0, "skipped": 0}
files = glob.glob(os.path.join(result_dir, "rfc*.json"))
parsed = 0
for path in files:
    try:
        with open(path, encoding="utf-8") as report_file:
            report = json.load(report_file)
        summary = report.get("summary", report.get("report", {}).get("summary", {}))
        if not isinstance(summary, dict) or not any(key in summary for key in counts):
            continue
        for key in counts:
            counts[key] += int(summary.get(key, 0) or 0)
        parsed += 1
    except (OSError, ValueError, TypeError):
        continue

if parsed:
    print("Pytest JSON reports parsed: {}".format(parsed))
    print("Reported tests: passed={}, failed={}, skipped={}".format(
        counts["passed"], counts["failed"], counts["skipped"]))
else:
    print("Reported test counts: N/A (no parseable pytest JSON summaries)")
PY
}

generate_shared_report() {
    reporter="$REPO_DIR/docker-device-mgt-service-test/generate_l2_coverage_report.py"
    if [ ! -f "$reporter" ]; then
        log_error 'shared L2 coverage reporter is missing from the mounted workspace'
        return 1
    fi

    python3 "$reporter" \
        --component "RFC" \
        --tracefile "$MERGED_TRACE" \
        --src-dir "$REPO_DIR" \
        --features-dir "$REPO_DIR/test/functional-tests/features" \
        --tests-dir "$REPO_DIR/test/functional-tests/tests" \
        --source-include 'rfcMgr/*.cpp' \
        --source-include 'rfcapi/rfcapi.cpp' \
        --source-include 'tr181api/*.cpp' \
        --source-include 'utils/*.cpp' \
        --output "$COVERAGE_DIR/L2_Coverage.md" || return 1
    test -s "$COVERAGE_DIR/L2_Coverage.md"
}

collect_coverage() {
    collection_status=0
    reporting_status=Incomplete
    if [ "${L2_SMOKE_STOP_OUTCOME:-unknown}" != "success" ] || \
        [ "${L2_STOP_OUTCOME:-unknown}" != "success" ] || \
        [ "${L2_SMOKE_EVIDENCE_OUTCOME:-unknown}" != "success" ]; then
        log_error 'smoke or final daemon shutdown did not complete successfully'
        collection_status=1
    fi
    if [ "$(cat "$COVERAGE_DIR/smoke.status" 2>/dev/null || true)" != "complete" ]; then
        log_error 'positive smoke-test coverage evidence is missing'
        collection_status=1
    fi

    if [ ! -s "$BASELINE_TRACE" ]; then
        log_error 'zero-hit baseline is missing'
        collection_status=1
    fi

    if lcov --capture --directory "$REPO_DIR" --base-directory "$REPO_DIR" --output-file "$COVERAGE_DIR/runtime.raw.info"; then
        if ! extract_production_trace "$COVERAGE_DIR/runtime.raw.info" "$RUNTIME_TRACE"; then
            collection_status=1
        fi
    else
        log_error 'runtime counter capture failed'
        collection_status=1
    fi
    rm -f "$COVERAGE_DIR/runtime.raw.info"

    if [ -s "$BASELINE_TRACE" ] && [ -s "$RUNTIME_TRACE" ]; then
        if ! lcov --add-tracefile "$BASELINE_TRACE" --add-tracefile "$RUNTIME_TRACE" --output-file "$MERGED_TRACE"; then
            collection_status=1
        fi
    else
        log_error 'baseline or runtime trace is missing'
        collection_status=1
    fi

    if [ -s "$MERGED_TRACE" ]; then
        if ! validate_trace "$MERGED_TRACE" > "$COVERAGE_DIR/trace-validation.txt"; then
            log_error 'trace failed source identity or positive execution validation'
            collection_status=1
        fi
        if ! lcov --summary "$MERGED_TRACE" > "$COVERAGE_DIR/coverage-summary.txt" 2>&1; then
            log_error 'LCOV summary generation failed'
            collection_status=1
        fi
        if ! genhtml --branch-coverage --function-coverage "$MERGED_TRACE" --output-directory "$COVERAGE_DIR/html"; then
            log_error 'HTML report generation failed'
            collection_status=1
        fi
    else
        log_error 'merged coverage trace is missing'
        collection_status=1
    fi

    if [ -s "$MERGED_TRACE" ] && generate_shared_report; then
        reporting_status=Complete
    else
        log_error 'shared Markdown coverage report generation failed'
        collection_status=1
    fi

    write_test_counts || collection_status=1
    if [ "$collection_status" -eq 0 ]; then
        measurement_status=Complete
    else
        measurement_status=Incomplete
    fi

    {
        printf 'Measurement status: %s\n' "$measurement_status"
        printf 'Markdown report status: %s\n' "$reporting_status"
        printf 'Smoke test outcome: %s\n' "${L2_SMOKE_OUTCOME:-unknown}"
        printf 'Main L2 suite outcome: %s\n' "${L2_MAIN_OUTCOME:-unknown}"
        printf 'Reboot-trigger suite outcome: %s\n' "${L2_REBOOT_OUTCOME:-unknown}"
        if [ -s /tmp/rfc_l2_compiler_version ]; then
            compiler_version=$(cat /tmp/rfc_l2_compiler_version)
        else
            compiler_version=unknown
        fi
        printf 'Compiler: %s\n' "$compiler_version"
        printf 'Collector: %s\n' "$(lcov --version 2>/dev/null | sed -n '1p')"
        printf 'HTML generator: %s\n' "$(genhtml --version 2>/dev/null | sed -n '1p')"
        printf 'Feature mapping: N/A; aggregate runtime trace does not establish per-scenario source coverage.\n'
        printf '\nTest result counts:\n'
        if [ -s "$COVERAGE_DIR/test-counts.txt" ]; then
            cat "$COVERAGE_DIR/test-counts.txt"
        else
            printf 'Pytest result counts unavailable.\n'
        fi
        printf '\nTrace validation:\n'
        if [ -s "$COVERAGE_DIR/trace-validation.txt" ]; then
            cat "$COVERAGE_DIR/trace-validation.txt"
        else
            printf 'No valid in-scope trace was produced.\n'
        fi
    } > "$COVERAGE_DIR/L2_Coverage_Status.txt"

    return "$collection_status"
}

case "${1:-}" in
    prepare)
        capture_baseline
        ;;
    collect)
        collect_coverage
        ;;
    smoke)
        capture_smoke_evidence
        ;;
    stop)
        stop_rfc_mgrs
        ;;
    *)
        printf 'Usage: %s {prepare|collect}\n' "$0" >&2
        exit 2
        ;;
esac
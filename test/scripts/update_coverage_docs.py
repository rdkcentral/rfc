#!/usr/bin/env python3
"""
update_coverage_docs.py — RFC CI coverage doc updater.

Reads per-test pytest JSON reports plus L1/L2 aggregate env files,
then rewrites the CI-generated sections in:
  test/docs/L1_Analysis_Report.md
  test/docs/L2_Analysis_Report.md

Usage:
  python3 test/scripts/update_coverage_docs.py \
      --reports-dir ci-artifacts/l2-reports \
      --l1-env ci-artifacts/l1/l1_metrics.env \
      --l2-env ci-artifacts/l2/l2_metrics.env
"""

import argparse
import glob
import json
import os
import re
import sys


# ---------------------------------------------------------------------------
# Component map: JSON report filename stem -> (component, [source_fns], disabled)
# Stems match the filenames produced by run_l2.sh and run_l2_reboot_trigger.sh.
# ---------------------------------------------------------------------------
COMPONENT_MAP = {
    "rfc_single_instance_run":              ("A. Startup & Init",        ["main()", "CurrentRunningInst()"],                              False),
    "rfc_init_failure":                     ("A. Startup & Init",        ["GetServURL()"],                                               False),
    "rfc_override_rfc_prop":               ("A. Startup & Init",        ["GetServURL()"],                                               False),
    "rfc_device_offline":                   ("B. Device Connectivity",   ["isDnsResolve()"],                                             False),
    "rfc_xconf_communication_success":      ("C. XConf Communication",   ["ProcessRuntimeFeatureControlReq()", "CreateXconfHTTPUrl()"],  False),
    "rfc_xconf_request_params":             ("C. XConf Communication",   ["CreateXconfHTTPUrl()"],                                       False),
    "rfc_feature_enable":                   ("C. XConf Communication",   ["ProcessRuntimeFeatureControlReq()"],                         False),
    "rfc_setget_param":                     ("D. RFC Param Mgmt",        ["setRFCParameter()", "getRFCParameter()"],                     False),
    "rfc_tr181_setget_local_param":         ("D. RFC Param Mgmt",        ["setLocalParam()", "getLocalParam()"],                         False),
    "rfc_factory_reset":                    ("D. RFC Param Mgmt",        ["processXconfResponseConfigDataPart()"],                       False),
    "rfc_valid_accountid":                  ("E. AccountID Lifecycle",   ["GetValidAccountId()"],                                        False),
    "rfc_trigger_reboot_unknown_accountid": ("E. AccountID Lifecycle",   ["isConfigValueChange()"],                                      False),
    "rfc_unknown_accountid":                ("E. AccountID Lifecycle",   ["rfcCheckAccountId()"],                                        False),
    "rfc_xconf_reboot":                     ("F. Reboot & Maintenance",  ["SendEventToMaintenanceManager()"],                           False),
    "rfc_configsethash_time":               ("G. Config Tracking",       ["updateHashAndTimeInDB()"],                                    False),
    "rfc_xconf_rfc_data":                   ("H. Data Persistence",      ["processXconfResponseConfigDataPart()"],                       False),
    "rfc_dynamic_static_cert_selector":     ("I. mTLS / Certificate",    ["getMtlscert()"],                                              True),
    "rfc_static_cert_selector":             ("I. mTLS / Certificate",    ["getMtlscert()"],                                              True),
    "rfc_rfc_webpa":                        ("J. WebPA",                 ["IARM event handler"],                                         False),
}

COMPONENT_ORDER = [
    "A. Startup & Init",
    "B. Device Connectivity",
    "C. XConf Communication",
    "D. RFC Param Mgmt",
    "E. AccountID Lifecycle",
    "F. Reboot & Maintenance",
    "G. Config Tracking",
    "H. Data Persistence",
    "I. mTLS / Certificate",
    "J. WebPA",
]

# Outcome icons used in per-test detail rows
OUTCOME_ICON = {
    "passed":  "PASS",
    "failed":  "FAIL",
    "error":   "ERROR",
    "skipped": "SKIP",
    "not_run": "NOT RUN",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_env_file(path):
    """Parse a KEY=VALUE env file and return a dict."""
    env = {}
    if not path or not os.path.isfile(path):
        return env
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, _, val = line.partition("=")
                env[key.strip()] = val.strip()
    return env


def load_reports(reports_dir):
    """Parse per-test results from every *.json in reports_dir.

    Returns dict {stem: {"summary": {...}, "tests": [{"name":..., "outcome":...}]}}
    """
    results = {}
    if not reports_dir or not os.path.isdir(reports_dir):
        print(f"WARNING: reports directory not found: {reports_dir}", file=sys.stderr)
        return results

    for path in glob.glob(os.path.join(reports_dir, "*.json")):
        stem = os.path.splitext(os.path.basename(path))[0]
        try:
            with open(path, "r", encoding="utf-8") as fh:
                data = json.load(fh)
            tests = []
            for t in data.get("tests", []):
                nodeid = t.get("nodeid", "")
                func_name = nodeid.split("::")[-1] if "::" in nodeid else nodeid
                tests.append({"name": func_name, "outcome": t.get("outcome", "unknown")})
            results[stem] = {"summary": data.get("summary", {}), "tests": tests}
        except Exception as exc:
            print(f"WARNING: Could not parse {path}: {exc}", file=sys.stderr)
    return results


def compute_component_metrics(results):
    """Group test results by component.

    Returns dict {component_label: {"source_fns": set, "rows": [...]}}
    where each row is {"name": str, "outcome": str, "disabled": bool}.
    """
    comp_data = {c: {"source_fns": set(), "rows": []} for c in COMPONENT_ORDER}

    for stem, (comp, source_fns, disabled) in COMPONENT_MAP.items():
        for fn in source_fns:
            comp_data[comp]["source_fns"].add(fn)

        if stem in results:
            for t in results[stem]["tests"]:
                comp_data[comp]["rows"].append(
                    {"name": t["name"], "outcome": t["outcome"], "disabled": disabled}
                )
        elif not disabled:
            # Report expected but missing — test was not run
            comp_data[comp]["rows"].append(
                {"name": f"[{stem} — report missing]", "outcome": "not_run", "disabled": False}
            )

    return comp_data


def build_ci_section(comp_data, generated_date, l2_passed, l2_collected, l2_failed,
                     feature_scenarios, gap_to_100):
    """Return the full markdown string to insert between the CI markers."""
    lines = []

    # ---- header -----------------------------------------------------------
    lines.append(f"*Generated: {generated_date} | L2 passed: {l2_passed}/{l2_collected}, "
                 f"failed: {l2_failed} | feature scenarios: {feature_scenarios}*")
    lines.append("")

    # ---- per-component table ----------------------------------------------
    lines.append("| Component | Source Functions Exercised | Tests | Passed | Failed | Coverage |")
    lines.append("|---|---|---:|---:|---:|---:|")

    grand_active = 0
    grand_passed = 0
    grand_failed = 0

    for comp_label in COMPONENT_ORDER:
        data = comp_data[comp_label]
        rows = data["rows"]
        src_str = ", ".join(f"`{fn}`" for fn in sorted(data["source_fns"])) or "—"

        # Split active vs disabled
        active_rows = [r for r in rows if not r["disabled"]]
        disabled_rows = [r for r in rows if r["disabled"]]

        if not active_rows and disabled_rows:
            # Whole component is disabled
            lines.append(
                f"| {comp_label} | {src_str} | {len(disabled_rows)} "
                "| DISABLED | DISABLED | *pending open-source* |"
            )
            continue

        if not active_rows:
            lines.append(f"| {comp_label} | {src_str} | 0 | — | — | — |")
            continue

        n_total = len(active_rows)
        n_passed = sum(1 for r in active_rows if r["outcome"] == "passed")
        n_failed = sum(1 for r in active_rows if r["outcome"] in ("failed", "error"))
        coverage = f"{n_passed / n_total * 100:.0f}%" if n_total else "—"

        grand_active += n_total
        grand_passed += n_passed
        grand_failed += n_failed

        lines.append(
            f"| {comp_label} | {src_str} | {n_total} | {n_passed} | {n_failed} | {coverage} |"
        )

    overall = f"{grand_passed / grand_active * 100:.2f}%" if grand_active else "0%"
    lines.append(
        f"| **TOTAL (active)** | | **{grand_active}** "
        f"| **{grand_passed}** | **{grand_failed}** | **{overall}** |"
    )
    lines.append("")
    lines.append(f"**Gap to 100% functional coverage: {gap_to_100}%**")
    lines.append("")

    # ---- per-test detail table -------------------------------------------
    lines.append("<details>")
    lines.append("<summary>Per-test function results (click to expand)</summary>")
    lines.append("")
    lines.append("| Component | Test Function | Outcome |")
    lines.append("|---|---|:---:|")

    for comp_label in COMPONENT_ORDER:
        data = comp_data[comp_label]
        for row in data["rows"]:
            icon = OUTCOME_ICON.get(row["outcome"], row["outcome"].upper())
            dis = " *(disabled)*" if row["disabled"] else ""
            lines.append(f"| {comp_label} | `{row['name']}`{dis} | {icon} |")

    lines.append("")
    lines.append("</details>")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Document updaters
# ---------------------------------------------------------------------------

def update_l2_report(doc_path, comp_data, generated_date, l2_env):
    """Replace the CI-generated section in L2_Analysis_Report.md."""
    if not os.path.isfile(doc_path):
        print(f"ERROR: {doc_path} not found", file=sys.stderr)
        return False

    l2_passed   = l2_env.get("L2_PASSED", "?")
    l2_collected = l2_env.get("L2_COLLECTED", "?")
    l2_failed   = l2_env.get("L2_FAILED", "?")
    feature_scenarios = l2_env.get("FEATURE_SCENARIOS", "?")
    gap_to_100  = l2_env.get("GAP_TO_100_PCT", "?")

    ci_block = build_ci_section(
        comp_data, generated_date,
        l2_passed, l2_collected, l2_failed,
        feature_scenarios, gap_to_100,
    )

    with open(doc_path, "r", encoding="utf-8") as fh:
        content = fh.read()

    START_MARKER = "<!-- CI-GENERATED-START -->"
    END_MARKER   = "<!-- CI-GENERATED-END -->"

    if START_MARKER not in content or END_MARKER not in content:
        print(
            f"ERROR: CI markers not found in {doc_path}. "
            "Add <!-- CI-GENERATED-START --> and <!-- CI-GENERATED-END --> to the document.",
            file=sys.stderr,
        )
        return False

    new_content = re.sub(
        re.escape(START_MARKER) + r".*?" + re.escape(END_MARKER),
        START_MARKER + "\n" + ci_block + "\n" + END_MARKER,
        content,
        count=1,
        flags=re.DOTALL,
    )

    with open(doc_path, "w", encoding="utf-8") as fh:
        fh.write(new_content)
    print(f"Updated: {doc_path}")
    return True


def update_l1_report(doc_path, generated_date, l1_env):
    """Replace the header section of L1_Analysis_Report.md (before ## Directory Breakdown)."""
    if not os.path.isfile(doc_path):
        print(f"ERROR: {doc_path} not found", file=sys.stderr)
        return False

    line_pct   = l1_env.get("LINE_PCT", "?")
    line_hit   = l1_env.get("LINE_HIT", "?")
    line_total = l1_env.get("LINE_TOTAL", "?")
    func_pct   = l1_env.get("FUNC_PCT", "?")
    func_hit   = l1_env.get("FUNC_HIT", "?")
    func_total = l1_env.get("FUNC_TOTAL", "?")

    new_header = (
        "# L1 Code Coverage Report\n\n"
        "**Test File:** coverage.info  \n"
        f"**Date:** {generated_date}  \n\n"
        "## Summary\n"
        f"- **Line Coverage:** {line_hit} / {line_total} (**{line_pct}%**)\n"
        f"- **Function Coverage:** {func_hit} / {func_total} (**{func_pct}%**)\n\n"
    )

    with open(doc_path, "r", encoding="utf-8") as fh:
        content = fh.read()

    # Replace everything before ## Directory Breakdown
    new_content = re.sub(
        r"\A.*?(?=^## Directory Breakdown)",
        new_header,
        content,
        count=1,
        flags=re.DOTALL | re.MULTILINE,
    )
    if new_content == content:
        new_content = re.sub(
            r"\A.*?(?=^---)",
            new_header,
            content,
            count=1,
            flags=re.DOTALL | re.MULTILINE,
        )
    if new_content == content:
        new_content = new_header + content

    with open(doc_path, "w", encoding="utf-8") as fh:
        fh.write(new_content)
    print(f"Updated: {doc_path}")
    return True


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Update L1/L2 coverage markdown docs.")
    parser.add_argument("--reports-dir", default="ci-artifacts/l2-reports",
                        help="Directory containing pytest JSON report files")
    parser.add_argument("--l1-env", default="ci-artifacts/l1/l1_metrics.env",
                        help="Path to L1 metrics env file")
    parser.add_argument("--l2-env", default="ci-artifacts/l2/l2_metrics.env",
                        help="Path to L2 metrics env file")
    parser.add_argument("--l1-doc",  default="test/docs/L1_Analysis_Report.md")
    parser.add_argument("--l2-doc",  default="test/docs/L2_Analysis_Report.md")
    parser.add_argument("--date",    default=None,
                        help="Override generated date (YYYY-MM-DD). Defaults to today UTC.")
    args = parser.parse_args()

    from datetime import datetime, timezone
    generated_date = args.date or datetime.now(timezone.utc).strftime("%Y-%m-%d")

    l1_env = load_env_file(args.l1_env)
    l2_env = load_env_file(args.l2_env)

    reports  = load_reports(args.reports_dir)
    comp_data = compute_component_metrics(reports)

    ok_l1 = update_l1_report(args.l1_doc, generated_date, l1_env)
    ok_l2 = update_l2_report(args.l2_doc, comp_data, generated_date, l2_env)

    if not ok_l1 or not ok_l2:
        sys.exit(1)


if __name__ == "__main__":
    main()

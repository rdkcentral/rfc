---
name: l2-coverage-onboarding
description: Generate and validate a module-specific changeset that measures runtime code coverage from L2 integration tests on every pull request. Use to onboard any component to PR coverage automation, adapt coverage instrumentation and daemon teardown, integrate shared LCOV reporting, and publish trustworthy CI evidence.
---

# L2 Coverage Onboarding

## Purpose

Implement PR-triggered L2 coverage in the repository the user identifies. Produce
actual edits, not just a design or a report. Adapt to the module's language,
build system, test harness, processes, and CI provider; do not copy another
component's dependency setup or runtime assumptions.

This skill generates automation once. CI, not an AI agent, measures coverage on
subsequent PRs. Coverage means code exercised by L2 tests, not the number of
tests or features present in the repository.

Read [the reference analysis](references/remote-debugger.md) for the verified
Remote Debugger implementation and its limitations. The reference is context,
not a requirement that the target repository contain those files.

## Invocation And Portability

Example request:

> Use l2-coverage-onboarding in this component. Generate the changeset to collect
> L2 runtime coverage on every PR using the existing L2 harness. Publish the
> trace, HTML, and Markdown report as CI artifacts. Do not commit or push.

For another repository, install this entire skill directory, including
`references/`, under that repository's `.github/skills/`, or the coding client's
configured shared skills location. No other Remote Debugger skill is required.
Follow the target repository's instructions before editing its files.

## 1. Discover The Local Contract

Start with the existing PR workflow and the command that actually runs L2.
Read only the nearby build, launch, teardown, and reporting paths needed to
identify where instrumentation and collection belong. Preserve existing edits.

Establish these facts from source, commands, or user-provided configuration:

| Input | Evidence required |
|---|---|
| Component and source scope | Production source roots, binaries, libraries, generated code exclusions |
| PR CI | Provider, target branches, event filters, checkout strategy, permissions |
| Coverage build | Build directory, compiler/runtime versions, compile and link commands |
| L2 harness | Suite command, test discovery, result files, exit-code semantics |
| Runtime | Process tree, services, fixtures, stop/wait paths, filesystem ownership |
| Environment | Runner/container/device, dependencies, mounts, tool installation |
| Reporting | Existing collector, shared generator version/CLI, report locations |
| Feature mapping | Optional feature directory and actual test naming conventions |
| Policy | Artifact retention, allowed exclusions, optional coverage gates |

Infer routine values from the repository. Ask only for a blocking unknown such
as an unavailable test environment or a required private dependency. Never ask
for secrets in chat. Do not invent an L2 suite if none exists: report that
prerequisite and obtain agreement before expanding into test-suite creation.

State one local hypothesis and a cheap discriminating check before editing.
Example: the L2 daemon is instrumented but force-killed before counters are
written; run one existing L2 test with orderly teardown and inspect its trace.
Then make the smallest change and run that check immediately.

## 2. Choose The Coverage Adapter

Reuse the project's supported coverage tools. Do not force GCC flags on an
unrelated language or compiler.

| Runtime | Typical adapter; verify locally |
|---|---|
| GCC C/C++ | `--coverage` at compile and link time, matching `gcov`, LCOV and `genhtml` |
| Clang C/C++ | Existing gcov-compatible mode or LLVM profiles and export; verify versions |
| Python service | Existing coverage.py configuration, including subprocess collection |
| JS/TS service | Existing V8/Istanbul coverage with source maps and process collection |
| JVM/.NET/Go/other | Native project coverage tooling and supported report export |

For non-LCOV backends, use their verified exporter or native report adapter.
Do not pass incompatible output to an LCOV parser. If a desired report format
is unsupported, state the limitation and implement supported runtime reporting
instead of claiming equivalent feature/function analysis.

Keep production build defaults unchanged. Add or reuse a coverage-only build
mode and isolated output directory. Instrument every in-scope binary and
library exercised by L2, not just a unit-test executable. Preserve required
compiler/linker flags. Ensure the test harness launches the newly built binary,
not a preinstalled copy. Record compiler, collector, and generator versions.

## 3. Implement The Measurement Lifecycle

Generate the smallest module-owned script/build/harness changes that implement:

1. Prepare an isolated build and coverage directory for this CI run.
2. Clean stale counters using the backend's supported reset operation. Do not
   delete build notes or mix unit-test counters into L2 coverage.
3. Capture a zero-hit baseline for all in-scope instrumented sources before
   L2 execution, including sources that no test reaches. For non-LCOV tools,
   configure their equivalent all-source inclusion.
4. Run the existing full L2 suite. Preserve ordering, fixtures, and result
   output. Accumulate failures across sequential test commands so later
   successes cannot overwrite an earlier failure. Zero collected tests fail.
5. Stop all instrumented processes orderly, wait with a bounded timeout for
   exit and profile writes, then collect counters. Treat forced-kill fallback
   as incomplete coverage, not successful measurement.
6. Merge the baseline and runtime traces with the tool's supported operation.
   Normalize container/build source paths and filter to verified component
   roots. Exclude dependencies, mocks, and generated code only with rationale.
7. Check trace structure, expected source identity, a nonzero denominator, and
   positive execution evidence from the smoke test. Reject missing, stale,
   malformed, or foreign-only traces. A valid zero-hit full-suite trace must
   be reported as zero, never replaced with an invented percentage.
8. Produce native HTML, machine-readable coverage, and a concise Markdown
   report. Keep test outcomes and measurement status separate.
9. Return failure if tests failed, collection/reporting failed, or an explicitly
   configured gate failed. Preserve both test and collection diagnostics.

For long-running services, prefer existing normal shutdown and runtime coverage
hooks. Do not blindly copy the reference's call to `__gcov_dump()` from a POSIX
signal handler: coverage runtime operations are not guaranteed async-signal-safe.
If needed, have the signal request shutdown and perform the flush from a normal
execution context, with a bounded wait. Account for threads and child processes.
For remote devices, provide a verified profile export and source-path mapping
back to the exact instrumented build; do not assume runner-local counters.

Do not add coverage-only behavior to production unless isolated behind the
project's existing test/coverage build controls. Do not refactor business logic.

## 4. Reuse Reporting Without Inventing Evidence

For compatible RDK modules, prefer the existing shared tool:

- Repository: `rdkcentral/docker-device-mgt-service-test`.
- Entry point observed in the reference: `generate_l2_coverage_report.py`.
- Observed arguments: `--component`, `--report-details`, `--tracefile`,
  `--src-dir`, `--features-dir`, `--tests-dir`, `--output`.

Before generating an invocation, inspect the selected shared revision's source,
README, and `--help`. Verify required arguments, metadata schema, LCOV dialect,
source-path handling, and behavior when feature files are absent. Prefer an
approved immutable revision; never carry over a temporary feature branch or
assume an unverified current default branch. Do not vendor a duplicate generator
or fetch and execute an unpinned script during CI.

Generate module metadata only from target-repository evidence. Descriptive
labels and reviewed recommendations are metadata, not measured coverage.
Never copy the reference's source list, behavior claims, or `~100%` target.
If the shared tool is inaccessible or incompatible, use native coverage reports
and a clearly identified minimal summary, and disclose unavailable mapping.

Distinguish these report signals:

- Runtime line/function/branch coverage: measured by the selected backend;
  unavailable dimensions are `N/A`, never zero or assumed complete.
- Feature-to-test mapping: static traceability or a naming heuristic, not proof
  that a scenario ran, passed, or exercised a particular source function.
- Test execution: pass/fail/skip counts from the actual L2 result files.
- Behavior assessments: manually curated unless tied to explicit test evidence.

Do not claim per-test or per-scenario source coverage from a single aggregate
trace. That requires isolated counters or backend test-context support.
Report zero denominators as `N/A`. Default to absolute PR-build coverage, not
changed-line coverage. A requested diff/regression gate requires separate,
validated base-revision data and explicit policy; never invent a threshold.

## 5. Integrate PR CI Securely

Extend the existing L2 job when possible; avoid running the suite twice. Generate
equivalent behavior for the target CI provider. For GitHub Actions:

- Use `pull_request` for every intended target branch, including updates to
  an open PR. Do not inherit the reference's `develop` restriction unless
  that is the target repository's policy. Avoid path filters that silently
  leave required checks pending. Ensure workflow-only PRs also exercise CI.
- Test the PR merge result by default. If repository policy tests the head
  instead, document it. Associate reports with the tested SHA and workflow run.
- Default to `permissions: contents: read`, no persisted checkout credentials,
  and no branch writes, commits, pushes, or automatic modifications to PRs.
- Never execute untrusted PR code under `pull_request_target`, with write
  tokens, privileged host mounts, or on a sensitive persistent runner.
- Run fork PRs without secrets where possible. If external L2 infrastructure
  requires secrets, report a clearly named blocked/skipped check and the
  approved trusted rerun path. Never mark missing evidence as measured success.
- Use timeouts, unique per-run output/container names, and isolated counters.
  Pin actions, tooling, and container images according to repository policy.
- After a successful build, run collection even when L2 fails, provided partial
  data can be collected safely. Use explicit outcome checks or equivalent
  control flow; `continue-on-error` must never turn missing coverage green.
- Publish traces, HTML, Markdown, L2 results, and available diagnostics with
  failure-aware artifact steps. Required coverage artifacts must fail when
  absent; optional diagnostic uploads may be best effort. Partial reports
  must say incomplete. Always attempt bounded cleanup on failure/cancellation.
- Add a short report or link to the job summary. PR comments and checked-in
  reports are opt-in and require a separately reviewed permission model.

Do not add a new secrets requirement or pull/build an expensive image without
following the user's and repository's approval rules.

## 6. Validate The Generated Changeset

Validate the first substantive edit immediately with the narrowest applicable
check. Then run the focused integration checks available in the target setup:

| Check | Acceptance evidence |
|---|---|
| Syntax and configuration | Repository workflow linter, script syntax check, metadata parser, verified reporter CLI |
| Coverage build | Compile/link instrumentation present; production build remains unchanged |
| Smoke measurement | Existing L2 test passes against the new binary; trace includes an expected executed function/line and an unexecuted in-scope source |
| Full L2 run | Actual test counts/status and nonempty trace, HTML, Markdown artifacts |
| Test failure | Temporarily induce a test failure in an isolated run; collection still attempted and job fails |
| Missing trace | Remove/redirect the trace in an isolated run; measurement fails explicitly |
| Isolation | A second fresh run cannot inherit the first run's hits; no unit-test counters included |
| PR behavior | Event coverage, tested SHA, read-only permissions, fork behavior, and artifact paths verified |

Use reversible local probes or fixtures, not permanent failing tests. Reuse the
repository's tests and helpers. Static inspection is not proof of a successful
container run or a real PR event. If Docker, device access, shared tooling,
credentials, or network are unavailable, finish the safe edits, run available
checks, and name each remaining unverified criterion and the exact command or
CI run needed. Do not fabricate validation results or add zero-byte artifacts.

## Deliverable

Leave a minimal, reviewable working-tree changeset, normally consisting of:

- Coverage-only build configuration if not already present.
- L2 runner/teardown and collection changes if needed.
- Existing PR workflow updates, or one new job when no L2 CI exists.
- Verified report metadata only if the selected reporter needs it.
- Brief local-run/report-location documentation in an existing suitable file.

Do not create all these files by default; reuse existing ownership boundaries.
Do not commit, push, create a PR, or change branch protections without an
explicit request. Finish with changed files, measurement scope, executed checks,
artifact locations, prerequisites, and any unverified gates. Clearly distinguish
the onboarding changeset from the coverage artifacts produced by future PRs.

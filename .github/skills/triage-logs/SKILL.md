---
name: triage-logs
description: >
  Triage RFC (Remote Feature Control) module behavioral issues on RDK devices
  by correlating device log bundles with the RFC module source tree. Covers
  rfcMgr daemon startup failures, XConf communication issues, TR181 parameter
  processing errors, IARM bus problems, mTLS certificate failures, and reboot
  trigger failures. The user states the issue; this skill guides systematic
  root-cause analysis regardless of issue type.
---

# RFC Module Log Triage Skill

## Purpose

Systematically correlate device log bundles with the RFC module source code
to identify likely root causes, characterize impact, and propose unit and
functional test reproductions for any behavioral anomaly reported by the user.

---

## Usage

Invoke this skill when:
- A device log bundle is available under `logs/` or is attached separately
- The user reports an RFC module problem such as startup failure, XConf fetch
  errors, TR181 parameter set/get failures, mTLS certificate issues, IARM bus
  problems, or unexpected reboot trigger behavior
- You need to design a reproduction scenario or propose validation coverage

The user's stated issue drives the investigation. Do not assume a fixed failure
mode.

---

## Step 1: Orient to the Log Bundle

Typical files to inspect first:

```text
logs/<MAC>/<SESSION_TIMESTAMP>/logs/
    rfcscript.log                  <- Primary RFC manager log (LOG_RFCMGR)
    messages.txt.0                 <- System messages and crashes
    top_log.txt.0                  <- CPU and memory snapshots
    /opt/rfc.properties            <- RFC configuration file
    /opt/secure/RFC/tr181store.ini <- TR181 parameter store
    /opt/secure/RFC/rfcVariable.ini <- RFC variable store
```

Additional files may exist depending on platform integration. For mTLS
issues also check `/etc/ssl/certs/client.pem` and related certificate paths.

---

## Step 2: Map Startup and Major Components

Read the startup portion of `rfcscript.log` and identify:

| What to find | Log pattern |
|---|---|
| Manager start | `RFC_MANAGER_INIT`, `rfcMgr`, `RFCManager::init` |
| XConf fetch start | `XConf`, `HTTP_GET`, `initializeXconfHandler` |
| XConf response | `httpCode`, `200`, `404`, `304` |
| TR181 parameter set | `setRFCParameter`, `tr181store`, `RFC_SET` |
| TR181 parameter get | `getRFCParameter`, `READ_RFC` |
| IARM event | `IARM`, `IARMBus`, `iarmEvent` |
| Reboot trigger | `reboot`, `RFC_REBOOT`, `MAINTENANCE_MGR` |
| mTLS cert selection | `mTLS`, `certSelector`, `RdkCertSelector` |

Key RFC module components in this repo:
- Manager entry point: `rfcMgr/rfc_main.cpp`
- RFC manager logic (fetch, parse, apply): `rfcMgr/rfc_manager.cpp`
- Common utilities and macros: `rfcMgr/rfc_common.cpp`, `rfcMgr/rfc_common.h`
- XConf HTTP handler: `rfcMgr/xconf_handler.cpp`, `rfcMgr/rfc_xconf_handler.cpp`
- mTLS certificate utilities: `rfcMgr/mtlsUtils.cpp`
- RFC get/set API: `rfcapi/rfcapi.cpp`
- TR181 parameter API: `tr181api/tr181api.cpp`
- JSON parsing utilities: `utils/jsonhandler.cpp`
- TR181 utilities: `utils/tr181utils.cpp`

---

## Step 3: Identify the Anomaly Window

Based on the user's stated issue, search for the relevant evidence pattern.

### RFC Manager Not Starting or Crashing

```bash
grep -n "RFCMGR\|rfc_manager\|RFC_MANAGER_INIT\|ERROR\|FATAL\|Segmentation\|core" rfcscript.log
grep -n "rfcMgr\|crash\|oom\|killed" messages.txt.0 | tail -50
```

Check for:
- Missing or malformed `/opt/rfc.properties`
- Network route unavailability at startup
- IARM bus initialization failures
- Library dependency failures surfaced in system logs

### XConf Communication Failures

```bash
grep -n "XConf\|xconf\|HTTP\|httpCode\|curl\|Failed\|retry" rfcscript.log
grep -n "RETRY_COUNT\|executeRequest\|initializeXconfHandler" rfcscript.log
```

Look for:
- HTTP response codes other than 200 (404 = no data, 304 = no change)
- `executeRequest` returning non-zero
- mTLS handshake failures
- DNS resolution or network timeout errors

### TR181 Parameter Set/Get Failures

```bash
grep -n "setRFCParameter\|getRFCParameter\|tr181store\|rfcVariable\|READ_RFC\|WRITE_RFC" rfcscript.log
```

Look for:
- `WRITE_RFC_FAILURE` or `READ_RFC_FAILURE` return values
- Missing or corrupt `/opt/secure/RFC/tr181store.ini`
- Parameter name format violations
- Value buffer overflow (check against `RFC_VALUE_BUF_SIZE = 512`)

### mTLS Certificate Failures

```bash
grep -n "mTLS\|mtls\|cert\|RdkCertSelector\|client.pem\|ssl\|SSL" rfcscript.log
```

Verify:
- Certificate file existence at expected paths
- `IS_LIBRDKCERTSEL_ENABLED` build flag effect
- Dynamic vs static cert selector fall-through behavior
- Certificate expiry or format issues

### IARM Bus Communication Failures

```bash
grep -n "IARM\|iarm\|IARMBus\|SYSMGR\|PWRMGR" rfcscript.log
```

Verify:
- IARM daemon (`IARMDaemonMain`) availability
- `IS_IARMBUS_ENABLED` compile-time flag
- Event registration and handler invocation
- IARM event payload parsing

### Reboot Trigger Issues

```bash
grep -n "reboot\|Reboot\|RFC_REBOOT\|MAINTENANCE\|ENABLE_MAINTENANCE" rfcscript.log
grep -n "ENABLE_MAINTENANCE" /etc/device.properties 2>/dev/null
```

Check for:
- Maintenance manager interaction when `EN_MAINTENANCE_MANAGER` is enabled
- Reboot flag set by XConf response payload
- RFC parameter changes that require device reboot

### Configuration / Single-Instance Failures

```bash
grep -n "already running\|lock\|pid\|instance\|rfc.properties" rfcscript.log
```

Check for:
- Stale PID lock file preventing second instance
- `/opt/rfc.properties` missing or unreadable
- `ConfigSetTime` not updated, causing unnecessary re-runs

---

## Step 4: Correlate with Source Code

Map the observed evidence to source files:

| Issue Area | Source Files |
|---|---|
| Manager initialization and RFC fetch cycle | `rfcMgr/rfc_manager.cpp` |
| XConf HTTP request execution | `rfcMgr/xconf_handler.cpp` |
| RFC XConf response parsing and apply | `rfcMgr/rfc_xconf_handler.cpp` |
| mTLS certificate selection | `rfcMgr/mtlsUtils.cpp` |
| Common utilities and error codes | `rfcMgr/rfc_common.cpp`, `rfcMgr/rfc_common.h` |
| RFC get/set parameter API | `rfcapi/rfcapi.cpp` |
| TR181 parameter store read/write | `tr181api/tr181api.cpp` |
| JSON payload parsing | `utils/jsonhandler.cpp` |
| TR181 store helper utilities | `utils/tr181utils.cpp` |
| Main entry point and signal handling | `rfcMgr/rfc_main.cpp` |

Example correlation:

If logs show XConf fetch succeeds (HTTP 200) but no parameters are applied:
1. Check `rfcMgr/rfc_xconf_handler.cpp` for JSON parsing of the response body
2. Check `utils/jsonhandler.cpp` for malformed payload handling
3. Verify `rfcMgr/rfc_manager.cpp::applyRFCSettings()` is reached

---

## Step 5: Reproduce Locally

### Unit Test Reproduction

Use existing gtest binaries when possible:

```bash
sh run_ut.sh
```

For targeted issues, run individual test binaries:

```bash
# RFC API tests
./rfcMgr/gtest/rfcapi_gtest

# TR181 API tests
./rfcMgr/gtest/tr181api_gtest

# RFC manager tests
./rfcMgr/gtest/rfcMgr_gtest

# Utility tests
./rfcMgr/gtest/utils_gtest
```

For XConf parse failures, inspect `rfcMgr/gtest/gtest_main.cpp` and mock
responses in `rfcMgr/gtest/mocks/` for patterns applicable to the observed
payload.

### L2 Reproduction

Use the repo's functional test harness:

```bash
sh cov_build.sh
sh run_l2.sh
sh run_l2_reboot_trigger.sh
```

Relevant L2 areas include:
- XConf communication success and failure paths
- TR181 set/get parameter flows
- Factory reset and RFC cleanup
- Single-instance enforcement
- Feature enable/disable via XConf response
- Reboot trigger on unknown account ID

### IARM / TR181 Verification on Device

```bash
# Check RFC feature flag directly via rbuscli
rbuscli get Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime

# Check stored TR181 parameter
rbuscli get Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID

# Check rfc.properties is present
cat /opt/rfc.properties
```

---

## Step 6: Test Gap Analysis

Identify code paths that may lack regression coverage.

### Unit Test Coverage

Inspect `rfcMgr/gtest/gtest_main.cpp` and related gtest files for:
- Error-path coverage in `xconf_handler.cpp`
- TR181 write failure handling
- mTLS certificate selection fall-through
- IARM event handler edge cases
- Single-instance lock file behavior

### L2 Test Coverage

Inspect `test/functional-tests/features/` and `test/functional-tests/tests/` for:
- Missing scenarios matching the reported bug
- XConf server error responses (e.g., 500, timeout)
- Malformed JSON payloads from XConf
- mTLS handshake failures (currently partial coverage)
- IARM bus unavailability paths

---

## Step 7: Propose Fix and Test

### Fix Template

```cpp
// BEFORE: Continue after XConf fetch failure
int httpCode = 0;
int ret = handler.ExecuteRequest(&fileDwnl, &security, &httpCode);
// Silently continued

// AFTER: Surface and log the failure
int httpCode = 0;
int ret = handler.ExecuteRequest(&fileDwnl, &security, &httpCode);
if (ret != SUCCESS || httpCode != 200) {
    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,
            "%s: XConf fetch failed ret=%d httpCode=%d\n",
            __FUNCTION__, ret, httpCode);
    return FAILURE;
}
```

### Test Template

```cpp
TEST(RfcManagerTest, ReportsFailureWhenXconfReturnsHttpError)
{
    // Configure mock curl to return HTTP 500
    // Verify rfc_manager returns FAILURE and logs the error
    xconf::XconfHandler handler;
    // ... set up mock ...
    EXPECT_EQ(handler.initializeXconfHandler(), FAILURE);
}
```

---

## Output Format

Present findings in this structure:

```markdown
## Triage Summary

**Issue:** <user's stated problem>
**Evidence:** <key log excerpts with timestamps>
**Root Cause:** <likely cause based on code analysis>
**Impact:** <what fails, user-visible effect>

## Code Location

**File:** <source file path>
**Function:** <function name>
**Line:** <approximate line number>

## Reproduction

[bash or test scenario to reproduce]

## Proposed Fix

[code diff or description]

## Test Coverage

**Existing:** [what tests exist]
**Missing:** [tests needed to prevent regression]

## Next Steps

1. [immediate action]
2. [follow-up verification]
```

---

## Example Triage Flow

**User:** "RFC fetches from XConf but no TR181 parameters are updated on device"

**Step 1:** Located `rfcscript.log`, confirmed HTTP 200 from XConf but no
`setRFCParameter` calls in the log.

**Step 2:** Checked `rfcMgr/rfc_xconf_handler.cpp` and confirmed the JSON
parsing path for parameter application.

**Step 3:** Verified mock XConf response in `rfcMgr/gtest/mocks/` and confirmed
the parameter key format in the payload did not match the expected pattern.

**Root Cause:** XConf response payload used a different parameter key format
than what the JSON parser expected in `rfcMgr/rfc_xconf_handler.cpp`.

**Fix Direction:** Update the JSON key matching logic and add a unit test with
the observed payload format to prevent regression.


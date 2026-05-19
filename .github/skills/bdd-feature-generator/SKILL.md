---
name: bdd-feature-generator
description: Generate BDD (Behavior Driven Development) feature files from RFC Manager source code analysis. Use for creating Gherkin-format documentation of XConf communication, TR-181 parameter processing, mTLS certificate selection, AccountID handling, Maintenance Manager integration, and reboot trigger behavior. Produces gap analysis between feature files and L2 test implementations.
---

# BDD Feature Generator for RFC Manager

## Purpose

Automatically generate BDD feature files in Gherkin format by analyzing the RFC Manager (`rfcMgr`) source code. This skill creates comprehensive behavioral documentation that can serve as:
- **Functional documentation** of XConf server communication, JSON response processing, and TR-181 parameter application
- **Test specifications** for L2 functional tests (`test/functional-tests/`)
- **Requirements traceability** linking RFC Manager source code to observable behavior
- **Gap analysis baseline** for comparing L2 tests vs implemented functionality

## Usage

Invoke this skill when:
- Documenting existing RFC Manager behavior in BDD format
- Creating test specifications for new XConf features or parameters
- Generating feature files for untested functionality (retry logic, error handling, etc.)
- Performing gap analysis between L2 tests and source implementation
- Onboarding new team members with behavioral documentation of the daemon

## Project Context

RFC Manager (`rfcMgr`) is a Remote Feature Control daemon for RDK devices. It:
- Queries the **XConf server** via HTTPS for feature-control configurations
- Parses **JSON responses** to extract TR-181 parameters, configSetHash, and timing data
- Applies parameters via **rbus** (RDKB/RDKC) or **WDMP-C/rfcapi** (video devices)
- Supports **mTLS** with dynamic (P12) and static (PEM) certificate selection
- Communicates with the **Maintenance Manager** via IARM bus events
- Manages **AccountID** resolution from XConf response and AuthService
- Handles **reboot scheduling** via Maintenance Manager events and cron jobs
- Runs as a **forked daemon** with single-instance enforcement via lock file

## Prerequisites

Before running this skill:

1. **Review the build system** — `configure.ac` and `Makefile.am` (top-level + sub-components)
2. **Identify compile flags** — `RDKB_SUPPORT`, `RDKC`, `USE_IARMBUS`, `LIBRDKCERTSELECTOR`, `GTEST_ENABLE`
3. **Review existing feature files** — Match the format in `test/functional-tests/features/`
4. **Check existing coverage** — Read `test/docs/L2_Analysis_Report.md` for current gap data
5. **Understand test interfaces** — L2 tests use `tr181` CLI, mock XConf HTTPS server (`rfcData.js`), mock `parodus` binary, and log scraping

## Process

### Step 1: Analyze Build Configuration

The RFC Manager build is Autotools-based. Identify compiled components from `configure.ac` and the Makefile chain:

```bash
# Top-level: identifies subdirectory build order
cat Makefile.am | grep "SUBDIRS"
# RDKC: SUBDIRS = rfcapi rfcMgr
# RDKB: SUBDIRS = rfcMgr
# Video (default): SUBDIRS = rfcapi tr181api utils rfcMgr

# rfcMgr: the main daemon binary
cat rfcMgr/Makefile.am
# Sources: rfc_main.cpp rfc_manager.cpp rfc_common.cpp mtlsUtils.cpp
#          rfc_xconf_handler.cpp xconf_handler.cpp
```

**Compile flags from `configure.ac`:**

| Flag | `configure` Option | Purpose |
|---|---|---|
| `-DRDKB_SUPPORT` | `--enable-rdkb=yes` | Broadband platform (rbus, dmcli, sysevent) |
| `-DRDKC` | `ENABLE_RDKC` condition | Camera platform (rbus, IP polling) |
| `-DUSE_IARMBUS` | `--enable-iarmbus=yes` | IARM bus for Maintenance Manager events |
| `-DLIBRDKCERTSELECTOR` | `--enable-rdkcertselector=yes` | Dynamic mTLS certificate selection (P12) |
| `-DLIBRDKCONFIG_BUILD` | `--enable-mountutils=yes` | Mount utilities for cert path resolution |
| `-DRDKB_EXTENDER_SUPPORT` | `--enable-rdkbextender=yes` | Broadband extender variant |
| `-DUSE_TR69HOSTIF` | `--enable-tr69hostif=yes` | TR-069 host interface integration |
| `-DGTEST_ENABLE` | `--enable-gtestapp=yes` | Google Test (L1 unit tests) |
| `-DINCLUDE_BREAKPAD` | `--enable-breakpad=yes` | Crash reporting via Breakpad |

**Platform build variants:**

| Platform | Libraries Built | Key Flags |
|---|---|---|
| RDKC (Camera) | `rfcapi`, `rfcMgr` | `-DRDKC`, rbus |
| RDKB (Broadband) | `rfcMgr` | `-DRDKB_SUPPORT`, rbus |
| RDKV (Video/STB) | `rfcapi`, `tr181api`, `utils` (tr181, rfctool), `rfcMgr` | WDMP-C, IARM |

### Step 2: Analyze Source Code Structure

**Core source files (`rfcMgr/`):**

| Source File | Key Functionality |
|---|---|
| `rfc_main.cpp` | Daemon entry point: fork(), lock file, signal handler, directory creation (`/opt/secure/RFC/`, `/tmp/RFC/`), calls `CheckDeviceIsOnline()` then `RFCManagerProcessXconfRequest()` |
| `rfc_manager.cpp` | `RFCManager` class: internet connectivity check (platform-specific), IARM initialization, Maintenance Manager events, XConf request orchestration, cron job management, post-processing |
| `rfc_manager.h` | Class declaration, `DeviceStatus` enum (`ONLINE`/`OFFLINE`), macros for file paths, maintenance event codes |
| `rfc_common.cpp` | `read_RFCProperty()` (rbus on RDKB/RDKC, WDMP on video), `waitForRfcCompletion()`, `CheckSpecialCharacters()`, `StringCaseCompare()` |
| `rfc_common.h` | Shared declarations, `RFCProperty` structure |
| `rfc_xconf_handler.cpp` | `RuntimeFeatureControlProcessor` class: XConf URL construction, JSON response parsing, configSetHash/Time tracking, TR-181 parameter application, AccountID handling, stash store/retrieve |
| `rfc_xconf_handler.h` | XConf handler declarations |
| `xconf_handler.cpp` | `XConfHandler` class: curl-based HTTPS communication, retry logic, HTTP response code handling |
| `xconf_handler.h` | XConf HTTP handler declarations |
| `mtlsUtils.cpp` | mTLS certificate retrieval: dynamic P12 cert via `RdkCertSelector`, static PEM fallback |
| `mtlsUtils.h` | mTLS utility declarations |

**Supporting libraries:**

| Directory | Library | Purpose |
|---|---|---|
| `rfcapi/` | `librfcapi.la` | RFC API: `getRFCParameter()`, `setRFCParameter()`, `freeRFCParameter()` |
| `tr181api/` | `libtr181api.la` | TR-181 API: parameter get/set wrappers (video only) |
| `utils/` | `tr181` binary | CLI tool for TR-181 parameter SET/GET from shell scripts |
| `utils/` | `rfctool` binary | JSON handler utility |

**Key elements to extract per source file:**

| Element | Where to Find | Example |
|---|---|---|
| XConf URL construction | `rfc_xconf_handler.cpp` | URL query parameters (eStbMac, model, firmwareVersion, env, accountId) |
| JSON response fields | `rfc_xconf_handler.cpp` | `featureControl.features[].configData`, `configSetHash`, `configSetTime` |
| TR-181 parameter application | `rfc_xconf_handler.cpp` | Parameter name/value from `effectiveImmediate` entries |
| Internet check paths | `rfc_manager.cpp` | IP route (RDKV), dmcli eRouter (RDKB), getifaddrs (RDKC) |
| IARM event types | `rfc_manager.cpp` | `MAINT_RFC_COMPLETE`, `MAINT_RFC_ERROR`, `MAINT_REBOOT_REQUIRED` |
| File-backed state | `rfc_xconf_handler.cpp` | `/opt/secure/RFC/tr181store.ini`, `/opt/secure/RFC/bootstrap.ini`, `/opt/secure/RFC/.version` |
| Certificate paths | `mtlsUtils.cpp` | Dynamic: `RdkCertSelector_Get_mTlsCreds()`, Static: `/etc/ssl/certs/client.pem` |
| Compile guards | All files | `#ifdef RDKB_SUPPORT`, `#ifdef RDKC`, `#ifdef LIBRDKCERTSELECTOR`, `#ifdef EN_MAINTENANCE_MANAGER` |

### Step 3: Create Feature File Structure

Feature files are placed in `test/functional-tests/features/`.

**Naming convention for RFC Manager:**

Feature files follow the pattern `rfc_{behavior_area}.feature`:

| Source Functionality | Feature File | Description |
|---|---|---|
| XConf HTTP communication | `rfc_xconf_communication.feature` | XConf server request/response, HTTP status codes |
| XConf request parameters | `rfc_xconf_request_params.feature` | URL query parameters sent to XConf |
| XConf configSetHash/Time | `rfc_xconf_configsetHash_time.feature` | Configuration hash and timestamp tracking |
| XConf RFC data processing | `rfc_data.feature` | JSON response feature data extraction |
| TR-181 parameter SET/GET | `rfc_setget_param.feature` | Parameter application via tr181 CLI |
| TR-181 local SET/GET | `rfc_tr181_setget_local_param.feature` | Local store parameter roundtrip |
| WebPA communication | `rfc_webpa.feature` | WebPA SET/GET via mock parodus binary |
| Feature enable/disable | `rfc_feature_enable.feature` | Feature control (HTTP 200 vs 304 responses) |
| Device offline status | `rfc_device_offline_status.feature` | Internet connectivity check behavior |
| Initialization failure | `rfc_initialization_failure.feature` | Unresolved XConf host, missing DNS |
| Single instance run | `rfc_single_instance_run.feature` | Lock file enforcement |
| AccountID handling | `rfc_valid_accountid.feature` | Valid AccountID resolution and application |
| Unknown AccountID | `rfc_unknown_accountid.feature` | Unknown/invalid AccountID behavior |
| Factory reset | `rfc_factory_reset.feature` | Store clear and re-application |
| Reboot required | `rfc_reboot_required.feature` | Reboot event via Maintenance Manager |
| Reboot trigger | `rfc_trigger_reboot.feature` | Account transition reboot trigger |
| Dynamic cert selection | `rfc_dynamic_cert_selector.feature` | P12 mTLS dynamic certificate |
| Static cert fallback | `rfc_static_cert_selector.feature` | PEM static certificate fallback |
| RFC properties override | `rfc_override_rfc_prop.feature` | rfc.properties override behavior |

### Step 4: Generate Feature Files

Use this template for RFC Manager feature files:

```gherkin
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright [YEAR] RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################

# Source: rfcMgr/{SourceFile}.cpp
# Test:   test/functional-tests/tests/{TestFile}.py

@order-{N}
Feature: {Feature Title}

  Background:
    Given the rfcMgr binary is available at "/usr/bin/rfcMgr"
    And the mock XConf HTTPS server is running on port 50053
    And the RFC properties file exists at "/etc/rfc.properties"

  Scenario: {Descriptive scenario name}
    Given {precondition}
    When {action — typically running rfcMgr or setting a parameter}
    Then {expected outcome — log message, parameter value, or file content}
```

### Step 5: Map Source Handlers to Scenarios

For each functional area in the RFC Manager source code, generate scenarios that exercise the corresponding behavior. Use the test helper constants and interfaces available in the L2 test infrastructure.

**XConf Communication (from `rfc_xconf_handler.cpp`, `xconf_handler.cpp`):**

```gherkin
Scenario: Successful XConf request with HTTP 200
  Given RFC_XCONF_URL is set to "https://mockxconf:50053/featureControl/getSettings"
  When the rfcMgr binary is executed
  Then the log should contain "HTTP Response code: 200"
  And the tr181store.ini file should be updated with parameters from the XConf response

Scenario: XConf returns 304 Not Modified
  Given the configSetHash matches the previous request
  When the rfcMgr binary is executed
  Then the log should contain "HTTP Response code: 304"
  And the tr181store.ini should remain unchanged
```

**Parameter SET/GET (from `rfc_common.cpp`, `rfcapi/rfcapi.cpp`):**

```gherkin
Scenario: SET and GET TR-181 parameter via tr181 CLI
  Given the TR-181 store file exists at "/opt/secure/RFC/tr181store.ini"
  When I SET "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass" to "default" via tr181
  And I GET "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass" via tr181
  Then the response should contain "default"
```

**WebPA Communication (from `rfc_common.cpp` — WDMP path):**

```gherkin
Scenario: SET parameter via WebPA
  When I send a WebPA SET payload via the mock parodus binary:
    """
    {"command":"SET","parameters":[{"name":"{param}","dataType":{type},"value":"{value}"}]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"message":"Success"'
```

**AccountID Handling (from `rfc_xconf_handler.cpp`):**

```gherkin
Scenario: Valid AccountID extracted from XConf response
  Given the XConf response contains a valid AccountID
  When the rfcMgr binary processes the response
  Then the log should contain the AccountID value
  And the TR-181 parameter for AccountID should be set

Scenario: Unknown AccountID triggers error handling
  Given the XConf response contains AccountID "3064488088886635972"
  When the rfcMgr binary processes the response
  Then the log should indicate an unknown AccountID
```

**Certificate Selection (from `mtlsUtils.cpp`):**

```gherkin
Scenario: Dynamic P12 certificate selection
  Given LIBRDKCERTSELECTOR is enabled
  And a valid P12 certificate is available
  When the rfcMgr binary initiates XConf communication
  Then the log should contain the dynamic certificate path

Scenario: Static PEM certificate fallback
  Given no dynamic certificate selector is available
  And "/etc/ssl/certs/client.pem" exists
  When the rfcMgr binary initiates XConf communication
  Then the log should contain "/etc/ssl/certs/client.pem"
```

**Reboot/Maintenance Manager (from `rfc_manager.cpp`):**

```gherkin
Scenario: Reboot required event sent to Maintenance Manager
  Given ENABLE_MAINTENANCE is set to "true" in device.properties
  And the XConf response requires a reboot
  When the rfcMgr binary completes processing
  Then an IARM event MAINT_REBOOT_REQUIRED should be sent

Scenario: RFC complete event sent to Maintenance Manager
  Given ENABLE_MAINTENANCE is set to "true" in device.properties
  When the rfcMgr binary completes successfully
  Then an IARM event MAINT_RFC_COMPLETE should be sent
```

### Step 6: Use Scenario Outlines for Parameterized Tests

When multiple test variations share the same flow but differ in data, use Scenario Outline with Examples:

```gherkin
Scenario Outline: XConf communication with different HTTP response codes
  Given RFC_XCONF_URL is set to "<url>"
  When the rfcMgr binary is executed
  Then the log should contain "HTTP Response code: <code>"

  Examples:
    | url                                                          | code |
    | https://mockxconf:50053/featureControl/getSettings           | 200  |
    | https://mockxconf:50053/featureControl304/getSettings        | 304  |
    | https://mockxconf:50053/featureControl404/getSettings        | 404  |
```

### Step 7: Create Feature-to-Test Mapping

Maintain a mapping between feature files and their corresponding test implementations:

| Feature File | Test File | Test Runner |
|---|---|---|
| `rfc_single_instance_run.feature` | `test_rfc_single_instance_run.py` | `run_l2.sh` |
| `rfc_device_offline_status.feature` | `test_rfc_device_offline_status.py` | `run_l2.sh` |
| `rfc_initialization_failure.feature` | `test_rfc_initialization_failure.py` | `run_l2.sh` |
| `rfc_xconf_communication.feature` | `test_rfc_xconf_communication.py` | `run_l2.sh` |
| `rfc_setget_param.feature` | `test_rfc_setget_param.py` | `run_l2.sh` |
| `rfc_tr181_setget_local_param.feature` | `test_rfc_tr181_setget_local_param.py` | `run_l2.sh` |
| `rfc_data.feature` | `test_rfc_xconf_rfc_data.py` | `run_l2.sh` |
| `rfc_xconf_request_params.feature` | `test_rfc_xconf_request_params.py` | `run_l2.sh` |
| `rfc_valid_accountid.feature` | `test_rfc_valid_accountid.py` | `run_l2.sh` |
| `rfc_factory_reset.feature` | `test_rfc_factory_reset.py` | `run_l2.sh` |
| `rfc_trigger_reboot.feature` | `test_rfc_trigger_reboot.py` | `run_l2.sh` |
| `rfc_feature_enable.feature` | `test_rfc_feature_enable.py` | `run_l2.sh` |
| `rfc_xconf_configsetHash_time.feature` | `test_rfc_xconf_configsethash_time.py` | `run_l2.sh` |
| `rfc_reboot_required.feature` | `test_rfc_xconf_reboot.py` | `run_l2.sh` |
| `rfc_override_rfc_prop.feature` | `test_rfc_override_rfc_prop.py` | `run_l2.sh` |
| `rfc_unknown_accountid.feature` | `test_rfc_unknown_accountid.py` | `run_l2_reboot_trigger.sh` |
| `rfc_webpa.feature` | `test_rfc_webpa.py` | `run_l2_reboot_trigger.sh` |
| `rfc_dynamic_cert_selector.feature` | `test_rfc_dynamic_static_cert_selector.py` | commented out |
| `rfc_static_cert_selector.feature` | `test_rfc_static_cert_selector.py` | commented out |

## Scenario Patterns for RFC Manager

### XConf Communication Pattern

```gherkin
Scenario: {Description of XConf interaction}
  Given RFC_XCONF_URL is configured in "/etc/rfc.properties"
  And the mock XConf server returns HTTP {status_code}
  When the rfcMgr binary is executed
  Then the log file "/opt/logs/rfcscript.txt" should contain "{expected_log_entry}"
```

### TR-181 Parameter SET/GET Pattern (tr181 CLI)

```gherkin
Scenario: SET and GET {parameter}
  When I SET "{Device.Namespace.Parameter}" to "{value}" via tr181 CLI
  And I GET "{Device.Namespace.Parameter}" via tr181 CLI
  Then the response should contain "{value}"
```

### WebPA SET/GET Pattern (mock parodus)

```gherkin
Scenario: SET {parameter} via WebPA
  When I send a WebPA SET payload via mock parodus:
    """
    {"command":"SET","parameters":[{"name":"{param}","dataType":{type},"value":"{value}"}]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"message":"Success"'

Scenario: GET {parameter} via WebPA
  When I send a WebPA GET payload via mock parodus:
    """
    {"command":"GET","names":["{param}"]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"value":"{expected}"'
```

### Device Connectivity Check Pattern

```gherkin
Scenario: Device offline — no route available
  Given the route file "/tmp/route_available" does not exist
  And the DNS file "/etc/resolv.dnsmasq" does not exist
  When the rfcMgr binary is executed
  Then the log should contain "OFFLINE" or connectivity failure message
  And the rfcMgr should exit without making XConf request

Scenario: Device online — route and DNS available
  Given the route file "/tmp/route_available" exists
  And the DNS file "/etc/resolv.dnsmasq" contains valid nameserver entries
  When the rfcMgr binary is executed
  Then the log should indicate successful connectivity check
```

### Single Instance Lock Pattern

```gherkin
Scenario: Only one rfcMgr instance runs at a time
  Given the lock file "/tmp/.rfcServiceLock" does not exist
  When the rfcMgr binary is executed
  Then the lock file should be created
  And a second rfcMgr instance should fail to acquire the lock
```

### Factory Reset Pattern

```gherkin
Scenario: Factory reset clears RFC stores
  Given the TR-181 store contains existing parameters
  When a factory reset is triggered
  And the rfcMgr binary is re-executed
  Then the store files should be cleared
  And parameters should be re-applied from the XConf response
```

### AccountID Handling Pattern

```gherkin
Scenario: AccountID from XConf response
  Given the XConf response contains AccountID "{account_id}"
  When the rfcMgr binary processes the response
  Then the log should contain "AccountID: {account_id}"
  And the TR-181 AccountID parameter should be set to "{account_id}"
```

### Reboot Trigger Pattern

```gherkin
Scenario: Account transition triggers reboot
  Given the previous AccountID was "{old_id}"
  And the new XConf response contains AccountID "{new_id}"
  When the rfcMgr binary processes the response
  Then a reboot trigger event should be generated
```

### Certificate Selection Pattern

```gherkin
Scenario: Dynamic mTLS certificate (P12)
  Given LIBRDKCERTSELECTOR is compiled in
  When the rfcMgr initiates HTTPS to XConf
  Then the log should show dynamic certificate selection via RdkCertSelector

Scenario: Static mTLS certificate (PEM fallback)
  Given the static certificate exists at "/etc/ssl/certs/client.pem"
  When the rfcMgr initiates HTTPS to XConf
  Then the log should show static certificate path "/etc/ssl/certs/client.pem"
```

### Maintenance Manager Event Pattern

```gherkin
Scenario: RFC {status} event to Maintenance Manager
  Given ENABLE_MAINTENANCE is "true" in "/etc/device.properties"
  When the rfcMgr binary completes with {status}
  Then an IARM event with code {event_code} should be sent

  # Event codes:
  # MAINT_RFC_COMPLETE = 2
  # MAINT_RFC_ERROR = 3
  # MAINT_REBOOT_REQUIRED = 4
```

### Feature Enable/Disable Pattern

```gherkin
Scenario: Feature enable via HTTP 200 response
  Given the XConf server returns a 200 with feature configuration
  When the rfcMgr binary is executed
  Then the feature parameters should be applied to the TR-181 store

Scenario: No change via HTTP 304 response
  Given the configSetHash matches the cached value
  When the rfcMgr binary is executed
  Then no parameters should be changed
  And the log should contain "304"
```

### RFC Properties Override Pattern

```gherkin
Scenario: Override XConf URL via rfc.properties
  Given "/opt/rfc.properties" contains "RFC_CONFIG_SERVER_URL={override_url}"
  When the rfcMgr binary is executed
  Then the XConf request should use "{override_url}" instead of the default
```

### Negative Test Pattern

```gherkin
Scenario: Unresolvable XConf hostname
  Given RFC_XCONF_URL points to "https://unmockxconf:50053/featureControl/getSettings"
  When the rfcMgr binary is executed
  Then the log should contain a DNS resolution failure
  And the rfcMgr should exit with an error

Scenario: Missing rfc.properties file
  Given "/etc/rfc.properties" does not exist
  When the rfcMgr binary is executed
  Then the log should indicate a configuration error
```

## Quality Checklist

Before completing feature generation for RFC Manager:

- [ ] All `rfcMgr/` source files analyzed for behavioral scenarios
- [ ] Platform-specific paths documented (RDKV, RDKB, RDKC connectivity checks)
- [ ] XConf communication scenarios cover 200, 304, 404, and error responses
- [ ] TR-181 parameter SET/GET roundtrip scenarios included
- [ ] WebPA/Parodus scenarios use mock parodus binary pattern
- [ ] AccountID handling scenarios cover valid, unknown, and transition cases
- [ ] Certificate selection scenarios cover dynamic (P12) and static (PEM) paths
- [ ] Maintenance Manager IARM event scenarios included (complete, error, reboot)
- [ ] Factory reset scenario documents store clear and re-application
- [ ] Single instance lock file enforcement documented
- [ ] Initialization failure scenarios cover DNS, network, and config errors
- [ ] Feature files match the existing naming convention (`rfc_*.feature`)
- [ ] License headers included (Apache 2.0, RDK Management)
- [ ] Source file references included as comments
- [ ] `@order-N` tags used for pytest execution ordering
- [ ] Feature-to-test mapping table is current
- [ ] Gap analysis identifies untested functionality
- [ ] Scenarios are atomic (one behavior per scenario)
- [ ] Given/When/Then structure followed consistently

## Test Infrastructure Reference

### L2 Test Environment

Tests run in a Docker container (`rdkcentral/docker-device-mgt-service-test`) with:

| Component | Description |
|---|---|
| Mock XConf Server | Node.js HTTPS server (`rfcData.js`) on port 50053, serves JSON feature-control responses |
| Mock Parodus Binary | Binary at `/usr/local/bin/parodus`, simulates WebPA communication |
| `tr181` CLI | Utility for TR-181 parameter SET/GET from shell (`utils/tr181utils.cpp`) |
| `rbuscli` | rbus command-line tool for direct parameter access |
| rfcMgr Binary | Compiled daemon at `/usr/bin/rfcMgr` |

### Key File Paths Used in Tests

| Constant | Path | Purpose |
|---|---|---|
| `RFC_MGR_PATH` | `/usr/bin/rfcMgr` | Daemon binary |
| `RFC_LOCK_FILE` | `/tmp/.rfcServiceLock` | Single-instance lock file |
| `RFC_LOG_FILE` | `/opt/logs/rfcscript.txt` | Primary RFC log file |
| `LOG_FILE` | `/opt/logs/rfcscript.txt.1` | Rotated log file (read after execution) |
| `RFC_PROPS_FILE` | `/etc/rfc.properties` | RFC configuration properties |
| `TR181_INI_FILE` | `/opt/secure/RFC/tr181.list` | TR-181 parameter list |
| `TR181_STORE_FILE` | `/opt/secure/RFC/tr181store.ini` | TR-181 store (XConf-applied values) |
| `BOOTSTRAP_FILE` | `/opt/secure/RFC/bootstrap.ini` | Bootstrap parameter store |
| `RFC_DEFAULTS_FILE` | `/etc/rfcdefaults.ini` | Default RFC parameter values |
| `RFC_OLD_FW_FILE` | `/opt/secure/RFC/.version` | Cached firmware version |
| `RFC_ROUTE_FILE` | `/tmp/route_available` | Network route availability flag |
| `RFC_DNS_FILE` | `/etc/resolv.dnsmasq` | DNS resolver configuration |
| `PARODUS_LOG_FILE` | `/opt/logs/parodus.log` | Mock parodus output log |
| `DEVICE_PROPERTIES` | `/etc/device.properties` | Device configuration (ENABLE_MAINTENANCE, etc.) |
| `VERSION_FILE` | `/version.txt` | Device firmware version |

### Mock XConf Server URLs

| Constant | URL | Response |
|---|---|---|
| `RFC_XCONF_URL` | `https://mockxconf:50053/featureControl/getSettings` | HTTP 200 with feature JSON |
| `RFC_XCONF_304_URL` | `https://mockxconf:50053/featureControl304/getSettings` | HTTP 304 Not Modified |
| `RFC_XCONF_404_URL` | `https://mockxconf:50053/featureControl404/getSettings` | HTTP 404 Not Found |
| `RFC_XCONF_UNRESOLVED_URL` | `https://unmockxconf:50053/featureControl/getSettings` | DNS resolution failure |
| `RFC_XCONF_OVERRIDE_URL` | `https://mockxconf_opt_rfc_properties/featureControl/getSettings` | Override URL test |

### Mock XConf Response Artifacts

Located in `test/test-artifacts/mockxconf/`:

| File | Purpose |
|---|---|
| `rfcData.js` | Node.js HTTPS mock server (Express) handling multiple endpoints |
| `xconf-rfc-response.json` | Standard XConf feature-control JSON response |
| `xconf-rfc-response-unknown-accountid.json` | Response with unknown AccountID for negative test |

### Test Execution Sequence

**Main suite (`run_l2.sh`):**

1. Pre-test setup: copy properties, certs, store files; set ConfigSetTime via `rbuscli`
2. Execute test files sequentially via `pytest`:
   - `test_rfc_single_instance_run.py`
   - `test_rfc_device_offline_status.py`
   - `test_rfc_initialization_failure.py`
   - `test_rfc_xconf_communication.py`
   - `test_rfc_setget_param.py`
   - `test_rfc_tr181_setget_local_param.py`
   - `test_rfc_xconf_rfc_data.py`
   - `test_rfc_xconf_request_params.py`
   - `test_rfc_valid_accountid.py`
   - `test_rfc_factory_reset.py`
   - `test_rfc_trigger_reboot.py`
   - `test_rfc_feature_enable.py`
   - `test_rfc_xconf_configsethash_time.py`
3. Enable Maintenance Manager: append `ENABLE_MAINTENANCE=true` to `device.properties`
4. Continue with:
   - `test_rfc_xconf_reboot.py`
   - `test_rfc_override_rfc_prop.py`
5. Certificate tests (currently commented out):
   - `test_rfc_dynamic_static_cert_selector.py`
   - `test_rfc_static_cert_selector.py`

**Reboot trigger suite (`run_l2_reboot_trigger.sh`):**

1. Set `ENABLE_MAINTENANCE=false` in `device.properties`
2. Remove `/opt/rfc.properties`
3. Execute:
   - `test_rfc_unknown_accountid.py`
   - `test_rfc_webpa.py`

## Output Structure

```
test/functional-tests/
├── features/                                      # BDD feature files (Gherkin documentation format)
│   ├── rfc_data.feature                           # XConf RFC data processing
│   ├── rfc_device_offline_status.feature          # Device connectivity checks
│   ├── rfc_dynamic_cert_selector.feature          # Dynamic P12 mTLS cert
│   ├── rfc_factory_reset.feature                  # Factory reset behavior
│   ├── rfc_feature_enable.feature                 # Feature enable/disable (200/304)
│   ├── rfc_initialization_failure.feature         # Startup failure scenarios
│   ├── rfc_override_rfc_prop.feature              # RFC properties override
│   ├── rfc_reboot_required.feature                # Reboot via Maintenance Manager
│   ├── rfc_setget_param.feature                   # TR-181 SET/GET roundtrip
│   ├── rfc_single_instance_run.feature            # Lock file enforcement
│   ├── rfc_static_cert_selector.feature           # Static PEM cert fallback
│   ├── rfc_tr181_setget_local_param.feature       # Local store SET/GET
│   ├── rfc_trigger_reboot.feature                 # Account transition reboot
│   ├── rfc_unknown_accountid.feature              # Unknown AccountID handling
│   ├── rfc_valid_accountid.feature                # Valid AccountID resolution
│   ├── rfc_webpa.feature                          # WebPA via mock parodus
│   ├── rfc_xconf_communication.feature            # XConf HTTP communication
│   ├── rfc_xconf_configsetHash_time.feature       # ConfigSetHash/Time tracking
│   └── rfc_xconf_request_params.feature           # XConf request parameters
├── tests/                                         # Runnable pytest functions
│   ├── rfc_test_helper.py                         # Shared constants, helper functions
│   ├── test_rfc_single_instance_run.py
│   ├── test_rfc_device_offline_status.py
│   ├── test_rfc_initialization_failure.py
│   ├── test_rfc_xconf_communication.py
│   ├── test_rfc_setget_param.py
│   ├── test_rfc_tr181_setget_local_param.py
│   ├── test_rfc_xconf_rfc_data.py
│   ├── test_rfc_xconf_request_params.py
│   ├── test_rfc_valid_accountid.py
│   ├── test_rfc_factory_reset.py
│   ├── test_rfc_trigger_reboot.py
│   ├── test_rfc_feature_enable.py
│   ├── test_rfc_xconf_configsethash_time.py
│   ├── test_rfc_xconf_reboot.py
│   ├── test_rfc_override_rfc_prop.py
│   ├── test_rfc_unknown_accountid.py
│   ├── test_rfc_webpa.py
│   ├── test_rfc_dynamic_static_cert_selector.py
│   └── test_rfc_static_cert_selector.py
└── test-artifacts/
    └── mockxconf/
        ├── rfcData.js                             # Mock XConf HTTPS server
        ├── xconf-rfc-response.json                # Standard response
        └── xconf-rfc-response-unknown-accountid.json  # Unknown AccountID response
```

**Test runner:** `pytest` executed sequentially by `run_l2.sh` and `run_l2_reboot_trigger.sh`.
**Interfaces exercised:** `tr181` CLI (parameter SET/GET), mock `parodus` binary (WebPA), mock XConf HTTPS server (rfcData.js), log scraping (`/opt/logs/rfcscript.txt`).

## Example: Complete RFC Manager Feature File

```gherkin
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright 2025 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################

# Source: rfcMgr/rfc_xconf_handler.cpp, rfcMgr/xconf_handler.cpp
# Test:   test/functional-tests/tests/test_rfc_xconf_communication.py

@order-4
Feature: RFC Manager XConf Communication

  Background:
    Given the rfcMgr binary is available at "/usr/bin/rfcMgr"
    And the mock XConf HTTPS server is running on port 50053
    And the RFC properties file exists at "/etc/rfc.properties"
    And the route file "/tmp/route_available" exists
    And the DNS file "/etc/resolv.dnsmasq" contains valid entries

  Scenario: Successful XConf request with feature data (HTTP 200)
    Given RFC_CONFIG_SERVER_URL is set to "https://mockxconf:50053/featureControl/getSettings"
    When the rfcMgr binary is executed
    Then the log should contain "HTTP Response code: 200"
    And the file "/opt/secure/RFC/tr181store.ini" should contain applied parameters

  Scenario: No configuration change (HTTP 304)
    Given the cached configSetHash matches the server value
    And RFC_CONFIG_SERVER_URL is set to "https://mockxconf:50053/featureControl304/getSettings"
    When the rfcMgr binary is executed
    Then the log should contain "HTTP Response code: 304"
    And the TR-181 store should remain unchanged

  Scenario: XConf server returns error (HTTP 404)
    Given RFC_CONFIG_SERVER_URL is set to "https://mockxconf:50053/featureControl404/getSettings"
    When the rfcMgr binary is executed
    Then the log should contain "HTTP Response code: 404"
    And no parameters should be applied
```

## Integration with Gap Analysis

After generating feature files, use them for gap analysis against the L2 test suite:

### Step 1: Map Features to Existing Tests

```
test/functional-tests/features/rfc_xconf_communication.feature  ↔ tests/test_rfc_xconf_communication.py
test/functional-tests/features/rfc_setget_param.feature          ↔ tests/test_rfc_setget_param.py
test/functional-tests/features/rfc_webpa.feature                 ↔ tests/test_rfc_webpa.py
test/functional-tests/features/rfc_device_offline_status.feature ↔ tests/test_rfc_device_offline_status.py
...
```

### Step 2: Count Coverage

For each feature file:
1. Count total scenarios (= total testable behaviors)
2. Count scenarios that have a matching `test_*` function in `test/functional-tests/tests/`
3. Calculate coverage = matched / total

### Step 3: Identify Missing Tests

Features without test coverage fall into categories:

| Category | Example | Required Infrastructure |
|---|---|---|
| Untested error paths | XConf timeout, curl errors | Mock server error injection |
| Platform-specific checks | RDKB dmcli, RDKC getifaddrs | Platform-specific Docker build |
| Retry/backoff logic | XConf retry on failure | Configurable mock server delays |
| Cron job management | `manageCronJob()` | Cron mock or verification |
| IARM event edge cases | Multiple events, race conditions | IARM bus mock verification |
| mTLS certificate tests | Dynamic P12, static PEM | Certificate infrastructure |

### Step 4: Identify Undocumented Tests

Tests that exist in `test/functional-tests/tests/` but have no matching scenario in
`test/functional-tests/features/`. These should be documented retroactively.

### Step 5: Generate Gap Report

Include a summary table in `test/docs/L2_Analysis_Report.md`:

```markdown
| Functional Area | Feature Scenarios | L2 Tests | Coverage | Top Gaps |
|---|:---:|:---:|:---:|---|
| XConf Communication | 5 | 3 | 60% | Timeout, retry logic |
| Parameter SET/GET | 4 | 4 | 100% | — |
| WebPA | 3 | 2 | 67% | Error payloads |
| AccountID | 4 | 3 | 75% | AuthService fallback |
| Certificate Selection | 4 | 2 | 50% | P12 expiry, rotation |
```

### Existing Coverage Reference

The comprehensive coverage analysis is maintained in:
- `test/docs/L2_Analysis_Report.md` — Full coverage data, gap analysis, and feature file corrections

## Maintenance

When RFC Manager source code changes:

1. **New XConf field added** — Add scenario to the appropriate `.feature` file; update JSON response artifacts
2. **New platform support** — Create platform-specific scenarios; note build flags
3. **New mTLS behavior** — Update certificate selection feature files
4. **IARM event added** — Document the event code and trigger conditions
5. **Handler removed** — Remove corresponding scenario; note in gap analysis
6. **Build flag changed** — Update conditional compilation notes
7. **L2 test added** — Update gap analysis coverage numbers
8. **Test helper constants changed** — Update the Test Infrastructure Reference section

## Related Skills

- `technical-documentation-writer` — For detailed architecture and API docs (`docs/`)
- `memory-safety-analyzer` — For safety analysis of rfcMgr C++ code
- `thread-safety-analyzer` — For concurrency analysis of daemon fork/signal handling
- `quality-checker` — For running static analysis and build verification
- `triage-logs` — For correlating device logs with RFC Manager source code

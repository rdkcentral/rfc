---
name: bdd-feature-generator
description: Generate BDD (Behavior Driven Development) feature files from tr69hostif source code analysis. Use for creating Gherkin-format documentation of TR-181 parameter handlers, daemon lifecycle, WebPA/Parodus communication, HTTP server protocol, and RFC store behavior. Produces gap analysis between feature files and L2 test implementations.
---

# BDD Feature Generator for tr69hostif

## Purpose

Automatically generate BDD feature files in Gherkin format by analyzing the tr69hostif source code. This skill creates comprehensive behavioral documentation that can serve as:
- **Functional documentation** of TR-181 data model parameter handlers (GET/SET)
- **Test specifications** for L2 functional tests (`test/functional-tests/`)
- **Requirements traceability** linking handler source code to observable behavior
- **Gap analysis baseline** for comparing L2 tests vs implemented handlers

## Usage

Invoke this skill when:
- Documenting existing tr69hostif parameter handlers in BDD format
- Creating test specifications for new TR-181 parameters
- Generating feature files for untested profiles (WiFi, MoCA, Ethernet, etc.)
- Performing gap analysis between L2 tests and source implementation
- Onboarding new team members with behavioral documentation of the daemon

## Project Context

tr69hostif is a TR-069/TR-181 host interface daemon for RDK devices. It:
- Exposes ~708 TR-181 parameters via **rbus DML** and **HTTP/WDMP-C** interfaces
- Communicates with **Parodus/WebPA** for cloud management
- Calls **Thunder JSON-RPC plugins** for device state (NetworkManager, AuthService, etc.)
- Manages **RFC**, **Bootstrap**, and **Non-RFC** parameter stores backed by INI files
- Runs as a systemd service with multiple initialization threads

## Prerequisites

Before running this skill:

1. **Review the build system** — `src/Makefile.am` and `src/hostif/profiles/Makefile.am`
2. **Identify compiled profiles** — Only document profiles that are actually built
3. **Review existing feature files** — Match the format in `docs/features/` and `test/functional-tests/features/`
4. **Check existing coverage** — Read `test/docs/L2_Test_Coverage.md` for current gap data
5. **Understand test interfaces** — L2 tests use `rbuscli` (rbus DML), mock `parodus` binary (WebPA), and log scraping

## Process

### Step 1: Analyze Build Configuration

The tr69hostif build is Autotools-based. Identify compiled components from the Makefile chain:

```bash
# Top-level: identifies src/ as the main SUBDIR
cat Makefile.am | grep "SUBDIRS"
# → SUBDIRS = $(SUBDIRS_MOCA) $(SUBDIRS_WIFI) src

# Source level: identifies compiled subsystems
cat src/Makefile.am | grep "SUBDIRS"
# → SUBDIRS = hostif/handlers hostif/profiles
# → SUBDIRS += hostif/snmpAdapter       (if WITH_SNMP_ADAPTER)
# → SUBDIRS += hostif/parodusClient
# → SUBDIRS += hostif/httpserver         (if !WITH_NEW_HTTP_SERVER_DISABLE)

# Profile level: identifies compiled TR-181 profile directories
cat src/hostif/profiles/Makefile.am | grep "SUBDIRS"
# → SUBDIRS = STBService Device DeviceInfo Ethernet IP Time
# → SUBDIRS += DHCPv4          (if WITH_DHCP_PROFILE)
# → SUBDIRS += StorageService   (if WITH_STORAGESERVICE_PROFILE)
# → SUBDIRS += InterfaceStack   (if WITH_INTFSTACK_PROFILE)
# → SUBDIRS += wifi             (if WITH_WIFI_PROFILE)
```

**Always compiled profiles:**

| Profile Directory | TR-181 Namespace | Key Source Files |
|---|---|---|
| `profiles/STBService/` | `Device.Services.STBService.*` | `Components_AudioOutput.cpp`, `Components_HDMI.cpp`, `Components_XrdkEMMC.cpp`, etc. |
| `profiles/Device/` | `Device.*` (x_rdk) | `x_rdk_profile.cpp` |
| `profiles/DeviceInfo/` | `Device.DeviceInfo.*` | `Device_DeviceInfo.cpp`, `XrdkBlueTooth.cpp`, `XrdkCentralComRFC.cpp`, `XrdkCentralComBSStore.cpp` |
| `profiles/Ethernet/` | `Device.Ethernet.*` | `Device_Ethernet_Interface.cpp`, `Device_Ethernet_Interface_Stats.cpp` |
| `profiles/IP/` | `Device.IP.*` | `Device_IP.cpp`, `Device_IP_Interface.cpp`, `Device_IP_Interface_IPv4Address.cpp`, `Device_IP_Interface_IPv6Address.cpp`, `Device_IP_Interface_Stats.cpp` |
| `profiles/Time/` | `Device.Time.*` | `Device_Time.cpp` |

**Conditionally compiled profiles:**

| Profile Directory | Build Flag | TR-181 Namespace |
|---|---|---|
| `profiles/DHCPv4/` | `WITH_DHCP_PROFILE` | `Device.DHCPv4.*` |
| `profiles/StorageService/` | `WITH_STORAGESERVICE_PROFILE` | `Device.StorageService.*` |
| `profiles/InterfaceStack/` | `WITH_INTFSTACK_PROFILE` | `Device.InterfaceStack.*` |
| `profiles/wifi/` | `WITH_WIFI_PROFILE` | `Device.WiFi.*` |

**Always compiled non-profile components:**

| Component | Key Source Files | Purpose |
|---|---|---|
| `handlers/` | `hostIf_rbus_Dml_Provider.cpp`, `hostIf_msgHandler.cpp`, `hostIf_jsonReqHandlerThread.cpp`, etc. | Request routing, rbus DML registration |
| `parodusClient/` | Parodus/WebPA client | Cloud management interface |
| `httpserver/` (conditional) | `http_server.cpp`, `request_handler.cpp`, `XrdkCentralComRFCVar.cpp` | libsoup HTTP server for WDMP-C JSON |

**Exclude from feature generation:**
- `src/hostif/include/` — Headers only
- `src/hostif/handlers/src/gtest/`, `src/hostif/profiles/*/gtest/` — Unit tests (L1)
- `src/hostif/*/docs/` — Existing documentation
- `test/` — Test infrastructure
- `scripts/` — Build/validation utilities

### Step 2: Analyze Source Code Structure

For each compiled profile/component:

1. **Read the header file** (`.h`) — Identify all `get_*` and `set_*` handler declarations
2. **Read the implementation** (`.cpp`) — Extract TR-181 parameter names from string comparisons, Thunder plugin calls, file I/O paths
3. **Identify the request handler** — Map the profile to its `hostIf_*_ReqHandler.cpp` in `handlers/src/`
4. **Note conditional compilation** — `#ifdef USE_HWSELFTEST_PROFILE`, `#ifdef USE_WIFI_PROFILE`, etc.
5. **Note Thunder dependencies** — Any `JSONRPCLink` or `org.rdk.*` plugin invocations
6. **Note file-backed parameters** — INI files, RFC stores, `/opt/secure/RFC/` paths

**Key elements to extract for each profile:**

| Element | Where to Find | Example |
|---|---|---|
| GET handler functions | `.h` class declaration | `get_Device_DeviceInfo_ModelName()` |
| SET handler functions | `.h` class declaration | `set_Device_DeviceInfo_X_RDKCENTRAL_COM_FirmwareToDownload()` |
| TR-181 parameter paths | `.cpp` string comparisons | `"Device.DeviceInfo.ModelName"` |
| Thunder plugin calls | `.cpp` `Invoke()` calls | `org.rdk.NetworkManager.GetIPSettings` |
| File-backed state | `.cpp` file open/write | `/opt/secure/RFC/bootstrap.ini` |
| Error return codes | `.cpp` return statements | `NOK`, `OK` |
| Compile guards | `.h` / `.cpp` `#ifdef` | `USE_HWSELFTEST_PROFILE`, `BLE_TILE_PROFILE` |

### Step 3: Create Feature File Structure

Feature files are generated to `docs/features/`.

```bash
mkdir -p docs/features
```

**Naming convention for tr69hostif:**

| Source | Feature File | Description |
|---|---|---|
| `profiles/DeviceInfo/` | `deviceinfo_parameters.feature` | DeviceInfo standard + custom params |
| `profiles/DeviceInfo/XrdkBlueTooth.cpp` | `bluetooth_parameters.feature` | BLE/Tile parameters |
| `profiles/IP/` | `ip_interface.feature` | IP, IPv4Address, IPv6Address, Stats |
| `profiles/Ethernet/` | `ethernet_interface.feature` | Ethernet interface + stats |
| `profiles/wifi/` | `wifi_parameters.feature` | WiFi Radio, SSID, AccessPoint, EndPoint |
| `profiles/moca/` | `moca_interface.feature` | MoCA interface, stats, QoS, mesh |
| `profiles/Time/` | `time_parameters.feature` | Time, NTP, Chrony |
| `profiles/STBService/` | `stbservice_components.feature` | AudioOutput, HDMI, eMMC, SDCard, etc. |
| `profiles/StorageService/` | `storage_service.feature` | PhysicalMedium |
| `profiles/DHCPv4/` | `dhcpv4_client.feature` | DHCPv4 client params |
| `profiles/InterfaceStack/` | `interface_stack.feature` | HigherLayer/LowerLayer |
| `handlers/src/hostIf_rbus_Dml_Provider.cpp` | `rbus_dml_registration.feature` | rbus data element registration |
| `httpserver/` | `http_server.feature` | WDMP-C HTTP server protocol |
| `parodusClient/` | `webpa_parodus.feature` | WebPA/Parodus communication |
| Daemon lifecycle | `bootup_sequence.feature` | Init, threads, shutdown |
| RFC/Bootstrap stores | `rfc_store.feature` | RFC, Bootstrap, rfcVariable persistence |

### Step 4: Generate Feature Files

Use this template for tr69hostif feature files:

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

# Source: src/hostif/profiles/{ProfileDir}/{SourceFile}.cpp

Feature: {TR-181 Namespace} Parameter Handlers

  Background:
    Given the tr69hostif daemon is running and initialized
    And rbuscli is available on the system

  Scenario: GET {TR-181 Parameter Path}
    When I GET "{parameter.path}" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain a valid value

  Scenario: SET and GET {TR-181 Parameter Path}
    When I SET "{parameter.path}" to "{test_value}" as {type} via rbus
    And I GET "{parameter.path}" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain "{test_value}"
```

### Step 5: Map Handler Code to Scenarios

**For each TR-181 parameter handler, create scenarios covering:**

1. **GET happy path** — Normal successful GET returning expected value
2. **SET + GET roundtrip** — SET a value, GET it back, verify match (for writable params)
3. **Error conditions** — GET/SET on invalid instance, wrong data type
4. **File-backed persistence** — Verify SET writes to INI file (for RFC/Bootstrap params)
5. **Thunder-backed params** — Verify GET maps Thunder plugin response correctly

**Example mapping — rbus DML GET handler:**

```cpp
// Source: src/hostif/profiles/DeviceInfo/Device_DeviceInfo.cpp
int hostIf_DeviceInfo::get_Device_DeviceInfo_ModelName(HOSTIF_MsgData_t *stMsgData, bool *pChanged)
{
    // reads /etc/device.properties for MODEL_NUM
    ...
    strncpy(stMsgData->paramValue, modelName, TR69HOSTIFMGR_MAX_PARAM_LEN);
    stMsgData->paramtype = hostIf_StringType;
    return OK;
}
```

**Generated scenario:**

```gherkin
Scenario: GET Device.DeviceInfo.ModelName
  When I GET "Device.DeviceInfo.ModelName" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain a non-empty string
```

**Example mapping — rbus DML SET+GET handler:**

```cpp
// Source: src/hostif/profiles/DeviceInfo/Device_DeviceInfo.cpp
int hostIf_DeviceInfo::set_Device_DeviceInfo_X_RDKCENTRAL_COM_FirmwareToDownload(
    HOSTIF_MsgData_t *stMsgData)
{
    snprintf(m_FirmwareToDownload, ...);
    return OK;
}
```

**Generated scenarios:**

```gherkin
Scenario: SET and GET FirmwareToDownload
  When I SET "Device.DeviceInfo.X_RDKCENTRAL-COM_FirmwareToDownload" to "test_image.bin" as string via rbus
  And I GET "Device.DeviceInfo.X_RDKCENTRAL-COM_FirmwareToDownload" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain "test_image.bin"
```

**Example mapping — WebPA via mock parodus:**

```gherkin
Scenario: SET parameter via WebPA
  When I send a WebPA SET payload:
    """
    {"command":"SET","parameters":[{"name":"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfUrl","dataType":0,"value":"https://mock/getSettings"}]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"message":"Success"'
```

**Example mapping — Thunder-backed parameter:**

```gherkin
Scenario: GET STB IP via Thunder NetworkManager
  Given the mock Thunder server is running on port 9998
  And "org.rdk.NetworkManager.GetIPSettings" returns {"ipaddress":"192.168.1.100"}
  When I GET "Device.DeviceInfo.X_COMCAST-COM_STB_IP" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain "192.168.1.100"
```

**Example mapping — RFC file-backed parameter:**

```gherkin
Scenario: Bootstrap parameter persisted to file
  When I SET "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName" to "TestProduct" as string via rbus
  And I GET "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName" via rbus
  Then the rbus response should contain "TestProduct"
  And the file "/opt/secure/RFC/bootstrap.ini" should contain "TestProduct"
```

### Step 6: Document Parameter Tables

For profiles with many parameters, use `Scenario Outline` with `Examples` tables:

```gherkin
Scenario Outline: GET Device.IP interface parameters
  When I GET "<parameter>" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain "<expected_value>"

  Examples:
    | parameter                                        | expected_value       |
    | Device.IP.Interface.1.IPv6Prefix.1.Autonomous    | false                |
    | Device.IP.Interface.1.IPv6Address.1.Anycast       | false                |
    | Device.IP.Interface.1.IPv6Address.1.Enable        | true                 |
    | Device.IP.Interface.1.IPv4Address.1.Enable        | true                 |
    | Device.IP.Interface.1.IPv6Enable                  | true                 |
```

For documenting the full handler inventory of a profile, use a table scenario:

```gherkin
Scenario: Device.Ethernet.Interface parameter handler coverage
  Given the Ethernet profile is compiled
  Then the following parameters should have GET handlers
    | Parameter                                          | Handler Function                              | Type    |
    | Device.Ethernet.Interface.{i}.Enable               | get_Device_Ethernet_Interface_Enable           | boolean |
    | Device.Ethernet.Interface.{i}.Status               | get_Device_Ethernet_Interface_Status           | string  |
    | Device.Ethernet.Interface.{i}.MACAddress            | get_Device_Ethernet_Interface_MACAddress        | string  |
    | Device.Ethernet.Interface.{i}.MaxBitRate            | get_Device_Ethernet_Interface_MaxBitRate        | int     |
    | Device.Ethernet.Interface.{i}.DuplexMode            | get_Device_Ethernet_Interface_DuplexMode        | string  |
    | Device.Ethernet.Interface.{i}.Stats.BytesSent       | get_Device_Ethernet_Interface_Stats_BytesSent   | ulong   |
    | Device.Ethernet.Interface.{i}.Stats.BytesReceived    | get_Device_Ethernet_Interface_Stats_BytesReceived| ulong  |
```

### Step 7: Create README Index

Create `docs/features/README.md`:

```markdown
# tr69hostif Feature Documentation

This folder contains BDD feature files documenting the TR-181 parameter
handlers and daemon behavior implemented in `src/hostif/`.

## Feature Files Overview

| Feature File | Source Components | Description |
|---|---|---|
| `bootup_sequence.feature` | `src/hostif/src/hostIf_main.cpp`, handlers | Daemon startup, thread init, subsystem checks |
| `handler_communications.feature` | `handlers/`, `profiles/DeviceInfo/` | RFC, Bootstrap, Time, Chrony SET/GET via rbus |
| `device_ip_profiles.feature` | `profiles/DeviceInfo/`, `profiles/IP/` | DeviceInfo defaults, IP, Services, ReverseSSH |
| `webpa_parodus.feature` | `parodusClient/` | WebPA SET/GET via mock parodus |
| `wifi_parameters.feature` | `profiles/wifi/` | WiFi Radio, SSID, AP, EndPoint |
| `ethernet_interface.feature` | `profiles/Ethernet/` | Ethernet interface + stats |
| `http_server.feature` | `httpserver/` | WDMP-C protocol tests |

## Source Directory Mapping

Based on `src/Makefile.am` and `src/hostif/profiles/Makefile.am`:

### Always Compiled
- `profiles/STBService/` — STB service components (Audio, HDMI, eMMC, etc.)
- `profiles/Device/` — x_rdk profile
- `profiles/DeviceInfo/` — DeviceInfo, RFC, Bootstrap, Bluetooth
- `profiles/Ethernet/` — Ethernet interface and stats
- `profiles/IP/` — IP, IPv4Address, IPv6Address, Interface stats
- `profiles/Time/` — NTP, Chrony time management
- `handlers/` — Request handlers, rbus DML provider, message handler
- `parodusClient/` — WebPA/Parodus client

### Conditionally Compiled
- `profiles/DHCPv4/` — `WITH_DHCP_PROFILE`
- `profiles/StorageService/` — `WITH_STORAGESERVICE_PROFILE`
- `profiles/InterfaceStack/` — `WITH_INTFSTACK_PROFILE`
- `profiles/wifi/` — `WITH_WIFI_PROFILE`
- `snmpAdapter/` — `WITH_SNMP_ADAPTER`
- `httpserver/` — `!WITH_NEW_HTTP_SERVER_DISABLE`

### Not Documented (Not Compiled)
- `src/hostif/include/` — Headers only
- `src/hostif/*/docs/` — Existing documentation
- `src/hostif/*/gtest/` — Unit tests (L1)

## Test Interface Summary

| Interface | Tool | Log File | Used For |
|---|---|---|---|
| rbus DML | `rbuscli get/set` | `/opt/logs/tr69hostif.log.0` | Parameter GET/SET |
| WebPA/Parodus | `/usr/local/bin/parodus` mock binary | `/opt/logs/parodus.log` | WebPA JSON payloads |
| HTTP Server | `curl` to `http://127.0.0.1:11999` | `/opt/logs/tr69hostif.log.0` | WDMP-C JSON protocol |
| Thunder | Mock JSON-RPC on `:9998` | `/opt/logs/tr69hostif.log.0` | Plugin-backed params |

## Generation Date

Generated: {DATE}
```

## Scenario Patterns for tr69hostif

### Parameter GET Pattern (rbus DML)

```gherkin
Scenario: GET {TR-181 Parameter}
  Given the tr69hostif daemon is running and initialized
  When I GET "{Device.Namespace.Parameter}" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain a valid {type} value
```

### Parameter SET+GET Roundtrip Pattern (rbus DML)

```gherkin
Scenario: SET and GET {TR-181 Parameter}
  Given the tr69hostif daemon is running and initialized
  When I SET "{Device.Namespace.Parameter}" to "{value}" as {type} via rbus
  And I GET "{Device.Namespace.Parameter}" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain "{value}"
```

### WebPA SET/GET Pattern (mock parodus)

```gherkin
Scenario: SET {parameter} via WebPA
  When I send a WebPA SET payload:
    """
    {"command":"SET","parameters":[{"name":"{param}","dataType":{type},"value":"{value}"}]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"message":"Success"'

Scenario: GET {parameter} via WebPA
  When I send a WebPA GET payload:
    """
    {"command":"GET","names":["{param}"]}
    """
  Then the parodus mock should exit with code 0
  And the parodus log should contain '"statusCode":200'
  And the parodus log should contain '"value":"{expected}"'
```

### HTTP Server Protocol Pattern (WDMP-C)

```gherkin
Scenario: GET parameter via HTTP server
  When I send HTTP GET to "http://127.0.0.1:11999" with body:
    """
    {"names":["{Device.Namespace.Parameter}"]}
    """
  Then the HTTP response status should be 200
  And the response body should contain '"statusCode":200'

Scenario: SET parameter via HTTP server with CallerID
  When I send HTTP POST to "http://127.0.0.1:11999" with CallerID header and body:
    """
    {"parameters":[{"name":"{param}","value":"{value}","dataType":{type}}]}
    """
  Then the HTTP response status should be 200
  And the response body should contain '"statusCode":200'

Scenario: SET parameter via HTTP server without CallerID
  When I send HTTP POST to "http://127.0.0.1:11999" without CallerID header
  Then the HTTP response status should be 500
  And the response body should contain "POST Not Allowed without CallerID"
```

### Thunder Plugin Pattern

```gherkin
Scenario: GET {parameter} backed by Thunder {plugin}
  Given the mock Thunder server is running on port 9998
  And "{org.rdk.Plugin.Method}" returns {mock_json_response}
  When I GET "{Device.Namespace.Parameter}" via rbus
  Then the rbus response should not contain an error
  And the rbus response should contain the mapped value from the Thunder response
```

### Daemon Bootup Log Pattern

```gherkin
Scenario: {Subsystem} initialization
  Given the tr69hostif binary has been invoked
  And the process has been active for at least 10 seconds
  When the daemon completes initialization
  Then the log should contain "{success_message}"
  And the log should NOT contain "{error_message}"
```

### RFC/Bootstrap File Persistence Pattern

```gherkin
Scenario: {Parameter} persisted to {file}
  When I SET "{Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.Param}" to "{value}" as string via rbus
  And I GET "{Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.Param}" via rbus
  Then the rbus response should contain "{value}"
  And the file "{/opt/secure/RFC/bootstrap.ini}" should contain "{value}"
```

### Negative Test Pattern

```gherkin
Scenario: GET nonexistent parameter
  When I GET "Device.Nonexistent.Parameter" via rbus
  Then the rbus response should contain an error

Scenario: SET wrong data type
  When I SET "{string_param}" to "123" as int via rbus
  Then the rbus response should indicate a type mismatch error

Scenario: Malformed WebPA JSON payload
  When I send a WebPA payload with malformed JSON
  Then the parodus mock should indicate a parse error
```

## Quality Checklist

Before completing feature generation for tr69hostif:

- [ ] All compiled profile directories analyzed (`profiles/Makefile.am` SUBDIRS)
- [ ] Non-compiled/conditional profiles noted with build flags
- [ ] Each GET handler has at least one scenario
- [ ] Each SET handler has a SET+GET roundtrip scenario
- [ ] Thunder-backed parameters identified and documented with plugin/method
- [ ] File-backed parameters (RFC, Bootstrap, Chrony) include file persistence checks
- [ ] HTTP server protocol scenarios included (GET, POST, error cases)
- [ ] WebPA/Parodus scenarios use mock parodus binary pattern
- [ ] Negative/edge case scenarios documented (wrong type, nonexistent param, malformed JSON)
- [ ] Conditional compilation guards noted (`#ifdef USE_WIFI_PROFILE`, etc.)
- [ ] License headers included (Apache 2.0, RDK Management)
- [ ] Source file references included as comments
- [ ] `@order-N` tags used for pytest execution ordering
- [ ] README index created with profile mapping and test interface summary
- [ ] Gap analysis section compares features to `test/functional-tests/tests/` implementations
- [ ] Scenarios are atomic (one parameter or behavior per scenario)
- [ ] Given/When/Then structure followed consistently

## Output Structure

```
docs/
└── features/
    ├── README.md                         # Index, profile mapping, gap summary
    ├── bootup_sequence.feature           # Daemon lifecycle, thread init
    ├── handler_communications.feature    # RFC/Bootstrap/Time SET+GET via rbus
    ├── device_ip_profiles.feature        # DeviceInfo, IP, Services via rbus
    ├── webpa_parodus.feature             # WebPA SET/GET via mock parodus
    ├── http_server.feature               # WDMP-C HTTP protocol tests
    ├── wifi_parameters.feature           # WiFi Radio/SSID/AP/EndPoint
    ├── ethernet_interface.feature        # Ethernet interface + stats
    ├── moca_interface.feature            # MoCA interface/stats/QoS
    ├── stbservice_components.feature     # AudioOutput/HDMI/eMMC/SDCard
    ├── time_parameters.feature           # NTP/Chrony beyond basic
    ├── rfc_store.feature                 # RFC variable store, override precedence
    ├── bluetooth_parameters.feature      # BLE/Tile (conditional)
    ├── dhcpv4_client.feature             # DHCPv4 (conditional)
    ├── storage_service.feature           # StorageService (conditional)
    ├── interface_stack.feature           # InterfaceStack (conditional)
    └── negative_edge_cases.feature       # Wrong type, nonexistent param, etc.
```

## Example: Complete tr69hostif Feature File

```gherkin
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright 2026 RDK Management
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

# Source: src/hostif/profiles/Ethernet/Device_Ethernet_Interface.cpp
# Source: src/hostif/profiles/Ethernet/Device_Ethernet_Interface_Stats.cpp

Feature: Device.Ethernet.Interface Parameter Handlers

  Background:
    Given the tr69hostif daemon is running and initialized
    And rbuscli is available on the system
    And the Ethernet profile is compiled (always compiled)

  Scenario: GET Ethernet interface enable status
    When I GET "Device.Ethernet.Interface.1.Enable" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain a boolean value

  Scenario: GET Ethernet interface status
    When I GET "Device.Ethernet.Interface.1.Status" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain one of "Up", "Down", "Unknown", "Dormant"

  Scenario: GET Ethernet interface MAC address
    When I GET "Device.Ethernet.Interface.1.MACAddress" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain a valid MAC address format

  Scenario Outline: GET Ethernet interface statistics
    When I GET "<parameter>" via rbus
    Then the rbus response should not contain an error
    And the rbus response should contain a numeric value

    Examples:
      | parameter                                              |
      | Device.Ethernet.Interface.1.Stats.BytesSent            |
      | Device.Ethernet.Interface.1.Stats.BytesReceived        |
      | Device.Ethernet.Interface.1.Stats.PacketsSent          |
      | Device.Ethernet.Interface.1.Stats.PacketsReceived      |
      | Device.Ethernet.Interface.1.Stats.ErrorsSent           |
      | Device.Ethernet.Interface.1.Stats.ErrorsReceived       |
      | Device.Ethernet.Interface.1.Stats.DiscardPacketsSent   |
      | Device.Ethernet.Interface.1.Stats.DiscardPacketsReceived|

  Scenario: GET Ethernet via WebPA wildcard
    When I send a WebPA GET payload:
      """
      {"command":"GET","names":["Device.Ethernet.Interface.1."]}
      """
    Then the parodus mock should exit with code 0
    And the parodus log should contain '"statusCode":200'

  Scenario: Ethernet interface handler coverage
    Given the Ethernet profile source is analyzed
    Then the following GET handlers should exist
      | Parameter                                           | Source File                              |
      | Device.Ethernet.Interface.{i}.Enable                | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.Status                | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.Name                  | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.LastChange             | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.Upstream               | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.MACAddress              | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.MaxBitRate              | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.DuplexMode              | Device_Ethernet_Interface.cpp            |
      | Device.Ethernet.Interface.{i}.Stats.BytesSent         | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.BytesReceived     | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.PacketsSent       | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.PacketsReceived   | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.ErrorsSent        | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.ErrorsReceived    | Device_Ethernet_Interface_Stats.cpp      |
      | Device.Ethernet.Interface.{i}.Stats.DiscardPacketsSent | Device_Ethernet_Interface_Stats.cpp     |
      | Device.Ethernet.Interface.{i}.Stats.DiscardPacketsReceived | Device_Ethernet_Interface_Stats.cpp |
```

## Integration with Gap Analysis

After generating feature files, use them for gap analysis against the L2 test suite:

### Step 1: Map Features to Existing Tests

```
docs/features/bootup_sequence.feature     ↔ test/functional-tests/tests/test_bootup_sequence.py
docs/features/handler_communications.feature ↔ test/functional-tests/tests/test_handlers_communications.py
docs/features/device_ip_profiles.feature   ↔ test/functional-tests/tests/tr69hostif_deviceip.py
docs/features/webpa_parodus.feature        ↔ test/functional-tests/tests/tr69hostif_webpa.py
docs/features/ethernet_interface.feature   ↔ (NO TEST FILE — gap)
docs/features/wifi_parameters.feature      ↔ (NO TEST FILE — gap)
docs/features/http_server.feature          ↔ (NO TEST FILE — gap)
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
| Untested profiles | WiFi, MoCA, Ethernet, DHCPv4 | Profile compiled in Docker container |
| Thunder-backed params | `X_COMCAST-COM_STB_IP` | Mock Thunder JSON-RPC on `:9998` |
| HTTP server protocol | GET/POST/error via `:11999` | `curl` or Python `requests` |
| Negative/edge cases | Wrong type, nonexistent param | Existing `rbuscli` / `parodus` mock |
| RFC store precedence | `rfcVariable` overrides `rfcdefaults` | File manipulation + rbus GET |

### Step 4: Identify Undocumented Tests

Tests that exist in `test/functional-tests/tests/` but have no matching scenario in the
original `test/functional-tests/features/` files. These should be documented retroactively.

### Step 5: Generate Gap Report

Include a summary table in `docs/features/README.md`:

```markdown
| Profile | Feature Scenarios | L2 Tests | Coverage | Top Gaps |
|---|:---:|:---:|:---:|---|
| Bootup lifecycle | 18 | 18 | 100% | — |
| DeviceInfo params | 172 | ~20 | ~12% | Thunder, BT, ProcessStatus |
| WiFi | 153 | 0 | 0% | Entire profile |
| MoCA | 99 | 0 | 0% | Entire profile |
```

### Existing Coverage Reference

The comprehensive coverage analysis is maintained in:
- `test/docs/L2_Test_Coverage.md` — Full per-parameter handler counts and gap data
- `test/functional-tests/automatics/` — Automatics test gap analysis tooling

## Current L2 Test Layout

```
test/functional-tests/
├── features/                                   # Original BDD feature files (docs only, not wired to pytest-bdd)
│   ├── tr69hostif_bootup_sequence.feature
│   ├── tr69hostif_deviceip.feature
│   ├── tr69hostif_handlers_communications.feature
│   └── tr69hostif_webpa.feature
├── tests/                                      # Runnable pytest functions
│   ├── test_bootup_sequence.py                 # orders 1–18: log scraping
│   ├── test_handlers_communications.py         # orders 19–27: rbus SET+GET (RFC, Bootstrap, Chrony)
│   ├── tr69hostif_deviceip.py                  # orders 25–28: rbus GET (DeviceInfo, IP, Services, SSH)
│   ├── tr69hostif_webpa.py                     # orders 29–44: mock parodus WebPA SET/GET
│   ├── helper_functions.py                     # Shell/log/rbus helper functions
│   ├── basic_constants.py                      # Shared constants (LOG_FILE, RBUS_EXCEPTION_STRING, etc.)
│   └── profile_helper_functions.py             # ⚠ Broken stub (GREP_STRING undefined)
└── automatics/                                 # Automatics gap analysis tooling
    ├── generate_test_gap_report.py
    ├── format_xls.py
    ├── xls_to_markdown.py
    └── requirements.txt
```

**Test runner:** `pytest` with `@pytest.mark.run(order=N)`, executed sequentially.
**Interfaces exercised:** `rbuscli` (rbus DML), mock `parodus` binary (WebPA), log scraping (`/opt/logs/tr69hostif.log.0`).

## Maintenance

When tr69hostif source code changes:

1. **New parameter handler added** — Add scenario to the appropriate `.feature` file; add parameter to handler coverage table
2. **New profile compiled** — Create a new `.feature` file; add to README index
3. **Thunder plugin call added** — Document the plugin/method mapping; note mock requirement
4. **Handler removed** — Remove corresponding scenario; note in gap analysis
5. **Build flag changed** — Update conditional compilation notes in README
6. **L2 test added** — Update gap analysis coverage numbers
7. **Version tag** — Include generation date in README

## Related Skills

- `technical-documentation-writer` — For detailed architecture and API docs (`docs/architecture/`, `docs/api/`)
- `memory-safety-analyzer` — For safety analysis of handler code
- `thread-safety-analyzer` — For concurrency analysis of daemon threads
- `quality-checker` — For running static analysis and build verification
- `tr69hostif-issue-triage` — For correlating device logs with source code

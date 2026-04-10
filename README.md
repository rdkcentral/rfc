# RFC — Remote Feature Control

[![Build](https://img.shields.io/badge/build-autotools-blue)](#building)
[![License](https://img.shields.io/badge/license-Apache%202.0-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STB%20%7C%20RDKB%20%7C%20RDKC-orange)](#platform-support)

> C++ daemon and API library for fetching, applying, and managing Remote Feature Control (RFC) settings from an Xconf configuration server across RDK device platforms.

---

## Table of Contents

- [Overview](#overview)
- [Platform Support](#platform-support)
- [Repository Structure](#repository-structure)
- [Architecture Overview](#architecture-overview)
- [Building](#building)
- [Configuration](#configuration)
- [Usage](#usage)
- [Testing](#testing)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**rfcMgr** is a C++ daemon that replaces the legacy shell-based `RFCbase.sh` for Remote Feature Control. It:

1. **Detects** when the device is online
2. **Collects** device identity (MAC, firmware, model, partner ID, account ID)
3. **Queries** the Xconf server for feature-control configuration
4. **Applies** RFC parameters to the local device
5. **Evaluates** whether a reboot is needed for `effectiveImmediate` features
6. **Schedules** periodic re-checks via cron

The **rfcapi** library provides a public C API (`getRFCParameter`, `setRFCParameter`, `isRFCEnabled`) for other RDK components to read/write RFC parameters.

---

## Platform Support

| Platform | Define Flag | Description |
|----------|-------------|-------------|
| **STB** | *(default)* | Set-Top Box — uses hostif/WDMP APIs, TR-181 data model |
| **RDKB** | `-DRDKB_SUPPORT` | RDK Broadband — uses rbus for TR-181 access |
| **RDKC** | `-DRDKC` | RDK Camera (XHC1/XCAM2) — flat-file parameter storage, mfrApi for device identity |

All platform-specific code is behind `#ifdef` guards, ensuring a single codebase builds cleanly for all targets.

---

## Repository Structure

```
rfc-fork/
├── rfcMgr/                        # RFC Manager daemon (core component)
│   ├── rfc_main.cpp               # Entry point — daemonize, init directories
│   ├── rfc_manager.h/cpp          # Lifecycle orchestrator
│   ├── xconf_handler.h/cpp        # Base device-identity collector
│   ├── rfc_xconf_handler.h/cpp    # Core RFC state machine & Xconf logic
│   ├── rdkc_rfc_xconf_handler.h/cpp  # RDKC camera-specific overrides
│   ├── mtlsUtils.h/cpp            # mTLS certificate handling
│   ├── rfc_common.h/cpp           # Shared utilities
│   ├── rfc_mgr_iarm.h             # IARM bus integration
│   ├── rfc_mgr_json.h             # JSON field constants
│   ├── rfc_mgr_key.h              # Configuration key constants
│   └── gtest/                     # Unit & L2 integration tests
│       └── mocks/                 # Test doubles
├── rfcapi/                        # Public RFC parameter API library
│   ├── rfcapi.h                   # C API header
│   └── rfcapi.cpp                 # Implementation
├── tr181api/                      # TR-181 data model API
│   ├── tr181api.h
│   └── tr181api.cpp
├── utils/                         # Utility libraries
│   ├── jsonhandler.h/cpp          # JSON parsing helpers
│   ├── trsetutils.h/cpp           # TR-181 set utilities
│   └── tr181utils.cpp             # TR-181 access utilities
├── test/                          # Functional test suite
│   └── functional-tests/
├── documentation/                 # Architecture & design docs
│   ├── architecture.md            # Component architecture & class diagrams
│   ├── sequence-diagrams.md       # End-to-end sequence flows
│   └── data-processing-flow.md    # Data processing & parameter flow
├── .github/                       # CI/CD workflows
│   ├── workflows/
│   └── CODEOWNERS
├── configure.ac                   # Autotools build configuration
├── Makefile.am                    # Top-level Automake
├── rfc.properties                 # Runtime configuration
├── getRFC.sh                      # Legacy shell wrapper
├── isFeatureEnabled.sh            # Feature check script
├── CHANGELOG.md                   # Release history
├── CONTRIBUTING.md                # Contribution guidelines
├── LICENSE / COPYING / NOTICE     # Legal
└── run_ut.sh / run_l2.sh         # Test runners
```

---

## Architecture Overview

```mermaid
graph TB
    subgraph "rfcMgr Daemon"
        MAIN["rfc_main.cpp<br/>Daemonize & Init"]
        MGR["RFCManager<br/>Orchestrator"]
        XCONF["XconfHandler<br/>Device Identity"]
        RFC["RuntimeFeatureControl<br/>Processor"]
        CAM["RdkcRuntimeFeatureControl<br/>Processor"]
        MTLS["mtlsUtils<br/>Certificate Handling"]
    end

    subgraph "Libraries"
        API["rfcapi<br/>Public C API"]
        TR181["tr181api<br/>TR-181 Access"]
        UTILS["utils<br/>JSON & Set Helpers"]
    end

    subgraph "External"
        SERVER["Xconf Server"]
        FS["File System<br/>tr181store.ini"]
    end

    MAIN --> MGR
    MGR --> RFC
    RFC --> XCONF
    RFC -.->|RDKC| CAM
    RFC --> MTLS
    RFC --> SERVER
    RFC --> API
    API --> FS
    API --> TR181
    TR181 --> UTILS
```

**Class Hierarchy:**

```
XconfHandler (device identity collection)
  └── RuntimeFeatureControlProcessor (Xconf query, parse, apply)
        └── RdkcRuntimeFeatureControlProcessor (camera-specific overrides)
```

For detailed architecture diagrams, see [documentation/architecture.md](documentation/architecture.md).

---

## Building

### Prerequisites

- GNU Autotools (`autoconf`, `automake`, `libtool`)
- C++11 compiler
- `libcurl`, `libcjson`
- Platform-specific: `librbus` (RDKB), `libhostif` (STB), `libmfrapi` (RDKC)

### Build Commands

```bash
# Generate build system
autoreconf -i

# Configure for target platform
./configure                          # STB (default)
./configure --enable-rdkc            # RDK Camera
./configure --enable-rdkb            # RDK Broadband

# Build
make

# Install
make install    # Installs /usr/bin/rfcMgr and librfcapi

# Unit tests
./configure --enable-gtestapp
make check
```

### Build Flags

| Option | Description |
|--------|-------------|
| `--enable-rdkc` | Enable RDKC camera platform support |
| `--enable-rdkb` | Enable RDKB broadband platform support |
| `--enable-gtestapp` | Enable Google Test unit tests |
| `--enable-rdkcertselector` | Use `librdkcertselector` for mTLS |
| `--enable-mountutils` | Use `librdkconfig` for configuration |

---

## Configuration

### rfc.properties

Runtime configuration is read from `rfc.properties`:

| Property | Description |
|----------|-------------|
| `RFC_CONFIG_SERVER_URL` | Primary Xconf server endpoint |
| `RFC_CONFIG_SERVER_URL_EU` | EU region Xconf server endpoint |
| `RFC_RAM_PATH` | Temporary storage path (`/tmp/RFC`) |
| `TR181_STORE_FILENAME` | Persistent parameter storage file |
| `RFC_POSTPROCESS` | Post-processing script path |
| `RFC_SERVICE_LOCK` | Lock file to prevent concurrent runs |

### RDKC Device Files

| File | Purpose |
|------|---------|
| `/opt/usr_config/partnerid.txt` | Syndication partner ID |
| `/opt/usr_config/service_number.txt` | Account ID |
| `/opt/usr_config/accounthash.txt` | MD5 account hash |
| `/opt/secure/RFC/tr181store.ini` | Persisted RFC parameters |
| `/tmp/RFC/.hashValue` | Configuration hash (RAM) |
| `/tmp/RFC/.timeValue` | Last update timestamp (RAM) |

---

## Usage

```bash
# Run the RFC manager daemon
/usr/bin/rfcMgr

# Check if a feature is enabled
./isFeatureEnabled.sh <feature_name>

# Query RFC settings (legacy shell)
./getRFC.sh
```

### rfcapi Library

```c
#include "rfcapi.h"

// Read a parameter
RFC_ParamData_t param;
int ret = getRFCParameter("module", "Device.X_RDK.Feature.Enable", &param);
if (ret == 0) {
    printf("Value: %s, Type: %d\n", param.value, param.type);
}

// Check feature status
if (isRFCEnabled("AccountInfo")) {
    // Feature is active
}
```

---

## Testing

```bash
# Unit tests (L1)
./run_ut.sh

# L2 integration tests
./run_l2.sh

# L2 reboot trigger tests
./run_l2_reboot_trigger.sh
```

Test coverage is tracked via GitHub Actions workflows:
- `L1-Test.yaml` — Unit test execution
- `L2-tests.yml` — Integration test execution
- `code-coverage.yml` — Coverage reporting

---

## Documentation

| Document | Description |
|----------|-------------|
| [Architecture](documentation/architecture.md) | Component architecture, class hierarchy, build system |
| [Sequence Diagrams](documentation/sequence-diagrams.md) | End-to-end execution flows with Mermaid diagrams |
| [Data Processing Flow](documentation/data-processing-flow.md) | Parameter lifecycle, storage strategies, data transformations |
| [Changelog](CHANGELOG.md) | Release history and version changes |
| [Contributing](CONTRIBUTING.md) | How to contribute to this project |

---

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on the contribution process, coding standards, and how to submit pull requests.

Before RDK accepts your code, you must sign the [RDK Contributor License Agreement (CLA)](https://wiki.rdkcentral.com/display/DOC/Contributor+License+Agreement).

---

## License

This project is licensed under the Apache License, Version 2.0 — see the [LICENSE](LICENSE) file for details.

```
Copyright 2018-2026 RDK Management

Licensed under the Apache License, Version 2.0
```

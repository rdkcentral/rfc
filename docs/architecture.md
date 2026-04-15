# Architecture

> Component architecture, class hierarchy, and build system design for the RFC (Remote Feature Control) daemon.

---

## Table of Contents

- [1. High-Level Architecture](#1-high-level-architecture)
- [2. Component Diagram](#2-component-diagram)
- [3. Class Hierarchy](#3-class-hierarchy)
- [4. Build System Architecture](#4-build-system-architecture)
- [5. Platform Abstraction Strategy](#5-platform-abstraction-strategy)
- [6. File Responsibilities](#6-file-responsibilities)
- [7. Dependency Graph](#7-dependency-graph)
- [8. External Integrations](#8-external-integrations)

---

## 1. High-Level Architecture

The `rfcMgr` daemon is a multi-platform C++ application that replaces the legacy `RFCbase.sh` shell script. It uses a polymorphic class hierarchy with `#ifdef` conditional compilation to support STB, RDKB, and RDKC platforms from a single codebase.

```mermaid
graph TD
    subgraph "Entry & Lifecycle"
        A["rfc_main.cpp<br/>fork() &#8226; signal handlers &#8226; directory setup"]
    end

    subgraph "Orchestration"
        B["RFCManager<br/>Device online check &#8226; processor lifecycle<br/>post-processing &#8226; cron scheduling"]
    end

    subgraph "RFC Processing Engine"
        C["XconfHandler<br/>Device identity collection<br/>MAC &#8226; FW &#8226; model &#8226; partner ID"]
        D["RuntimeFeatureControlProcessor<br/>Xconf query &#8226; JSON parse &#8226; param apply<br/>hash/time management &#8226; reboot eval"]
        E["RdkcRuntimeFeatureControl<br/>Processor<br/>Camera URL &#8226; RAM hash/time &#8226; no-op metadata"]
    end

    subgraph "Security"
        F["mtlsUtils<br/>Dynamic XPKI &#8226; Static XPKI &#8226; cert selector"]
    end

    subgraph "Public API"
        G["rfcapi (librfcapi)<br/>getRFCParameter() &#8226; setRFCParameter()<br/>isRFCEnabled()"]
    end

    subgraph "Data Model Layer"
        H["tr181api / utils<br/>TR-181 access &#8226; JSON handler &#8226; set utilities"]
    end

    subgraph "External Systems"
        I["Xconf Server<br/>HTTP REST API"]
        J["File System<br/>tr181store.ini &#8226; hash/time files"]
        K["Platform APIs<br/>hostif &#8226; rbus &#8226; mfrApi"]
    end

    A --> B
    B --> D
    D --> C
    D -.->|"#ifdef RDKC"| E
    D --> F
    D --> I
    D --> G
    G --> J
    G --> H
    H --> K

    style E fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px
    style F fill:#fff9c4,stroke:#f9a825,stroke-width:2px
    style G fill:#bbdefb,stroke:#1565c0,stroke-width:2px
```

---

## 2. Component Diagram

```mermaid
graph LR
    subgraph "rfcMgr Binary"
        direction TB
        MAIN[rfc_main]
        MGR[rfc_manager]
        XCONF[xconf_handler]
        RFCX[rfc_xconf_handler]
        RDKC_X[rdkc_rfc_xconf_handler]
        COMMON[rfc_common]
        MTLS[mtlsUtils]
        IARM[rfc_mgr_iarm]
        JSON_H[rfc_mgr_json]
        KEY_H[rfc_mgr_key]

        MAIN --> MGR
        MGR --> RFCX
        RFCX --> XCONF
        RFCX -.-> RDKC_X
        RFCX --> COMMON
        RFCX --> MTLS
        MGR --> IARM
        RFCX --> JSON_H
        RFCX --> KEY_H
    end

    subgraph "librfcapi.so"
        RFCAPI[rfcapi]
    end

    subgraph "libtr181api.so"
        TR181[tr181api]
    end

    subgraph "libutils"
        JSONUTIL[jsonhandler]
        TRSET[trsetutils]
        TR181U[tr181utils]
    end

    RFCX --> RFCAPI
    RFCAPI --> TR181
    TR181 --> JSONUTIL
    TR181 --> TRSET

    style RDKC_X fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px
```

---

## 3. Class Hierarchy

```mermaid
classDiagram
    class XconfHandler {
        #string _estb_mac_address
        #string _firmware_version
        #EBuildType _ebuild_type
        #string _build_type_str
        #string _model_number
        #string _ecm_mac_address
        #string _manufacturer
        #string _partner_id
        +initializeXconfHandler() int
        +ExecuteRequest(FileDownload*, MtlsAuth_t*, int*) int
    }

    class RuntimeFeatureControlProcessor {
        #string _accountId
        #string _experience
        #string _xconf_server_url
        #bool _rfcRebootCronNeeded
        #set~string~ _effectiveImmediateParams
        +InitializeRuntimeFeatureControlProcessor() int
        +ProcessRuntimeFeatureControlReq() int
        +GetAccountID() void
        +getRfcRebootCronNeeded() bool
        #CreateXconfHTTPUrl()* stringstream
        #RetrieveHashAndTimeFromPreviousDataSet()* void
        #StoreXconfEndpointMetadata()* void
        #set_RFCProperty(char*, char*, char*) WDMP_STATUS
        #clearDB() void
        #updateHashInDB(string) void
        #updateTimeInDB(string) void
    }

    class RdkcRuntimeFeatureControlProcessor {
        -string _accountHash
        -GetAccountHash() void
        #CreateXconfHTTPUrl() stringstream
        #RetrieveHashAndTimeFromPreviousDataSet() void
        #StoreXconfEndpointMetadata() void
    }

    class RFCManager {
        -RuntimeFeatureControlProcessor* _rfcProcessor
        +CheckDeviceIsOnline() DeviceStatus
        +RFCManagerProcess() int
        +RFCManagerProcessXconfRequest() int
        +RFCManagerPostProcess() void
    }

    XconfHandler <|-- RuntimeFeatureControlProcessor : inherits
    RuntimeFeatureControlProcessor <|-- RdkcRuntimeFeatureControlProcessor : inherits
    RFCManager --> RuntimeFeatureControlProcessor : creates & uses

    note for RdkcRuntimeFeatureControlProcessor "Compiled only with -DRDKC flag.\nOverrides 3 virtual methods for\ncamera-specific behavior."
```

### Inheritance Design Rationale

The class hierarchy follows the **Template Method Pattern**:

- **`XconfHandler`** — Collects device identity. Platform differences are behind `#ifdef` blocks within methods.
- **`RuntimeFeatureControlProcessor`** — Implements the full RFC workflow. Declares `virtual` methods for steps that vary by platform.
- **`RdkcRuntimeFeatureControlProcessor`** — Overrides only 3 methods, keeping the camera delta minimal:
  - `CreateXconfHTTPUrl()` — Camera-specific URL parameters
  - `RetrieveHashAndTimeFromPreviousDataSet()` — RAM-file storage
  - `StoreXconfEndpointMetadata()` — No-op (camera must not persist Xconf metadata)

---

## 4. Build System Architecture

### Configure Options Flow

```mermaid
flowchart TD
    A["./configure"] --> B{"--enable-rdkc?"}
    B -- Yes --> C["AM_CONDITIONAL ENABLE_RDKC = true<br/>-DRDKC added to CPPFLAGS"]
    B -- No --> D{"--enable-rdkb?"}
    D -- Yes --> E["AM_CONDITIONAL ENABLE_RDKB = true<br/>-DRDKB_SUPPORT added"]
    D -- No --> F["Default STB build"]

    C --> G["SUBDIRS = rfcapi rfcMgr"]
    E --> H["SUBDIRS = rfcMgr"]
    F --> I["SUBDIRS = rfcapi tr181api utils rfcMgr"]

    G --> J["rfcMgr sources += rdkc_rfc_xconf_handler.cpp<br/>Links: -lrfcapi -ldwnlutil -lfwutils -lcurl"]
    H --> K["Links: -lrbus"]
    I --> L["Links: ../rfcapi/.libs/librfcapi.la"]

    style C fill:#c8e6c9,stroke:#2e7d32
    style G fill:#c8e6c9,stroke:#2e7d32
    style J fill:#c8e6c9,stroke:#2e7d32
```

### Build Outputs

| Platform | Subdirectories Built | Binary | Libraries |
|----------|---------------------|--------|-----------|
| **STB** | rfcapi, tr181api, utils, rfcMgr | `/usr/bin/rfcMgr` | librfcapi.so, libtr181api.so |
| **RDKB** | rfcMgr | `/usr/bin/rfcMgr` | *(links external rbus)* |
| **RDKC** | rfcapi, rfcMgr | `/usr/bin/rfcMgr` | librfcapi.so |

---

## 5. Platform Abstraction Strategy

The codebase uses two complementary strategies:

### Strategy 1: Preprocessor Guards (`#ifdef`)

Used within shared source files for small, inline platform differences:

```
┌──────────────────────────────────────────────┐
│  #ifdef RDKC                                 │
│      // Camera-specific implementation        │
│  #elif defined(RDKB_SUPPORT)                 │
│      // Broadband-specific implementation     │
│  #else                                       │
│      // STB default implementation            │
│  #endif                                      │
└──────────────────────────────────────────────┘
```

**Used in:** `rfc_manager.cpp`, `xconf_handler.cpp`, `rfc_xconf_handler.cpp`, `mtlsUtils.cpp`, `rfcapi.cpp`

### Strategy 2: Polymorphic Override

Used for larger behavioral differences via virtual method dispatch:

```mermaid
flowchart LR
    A["RFCManager"] --> B{"Platform?"}
    B -- RDKC --> C["new RdkcRuntimeFeature<br/>ControlProcessor()"]
    B -- Other --> D["new RuntimeFeature<br/>ControlProcessor()"]
    C --> E["Virtual dispatch for<br/>CreateXconfHTTPUrl()<br/>RetrieveHashAndTime...<br/>StoreXconfEndpoint..."]
    D --> E
```

---

## 6. File Responsibilities

### Core Daemon (`rfcMgr/`)

| File | Responsibility | Key Functions |
|------|---------------|---------------|
| `rfc_main.cpp` | Process entry point | `main()`, `cleanup_lock_file()`, `signal_handler()`, `createDirectoryIfNotExists()` |
| `rfc_manager.h/cpp` | Lifecycle orchestrator | `CheckDeviceIsOnline()`, `RFCManagerProcess()`, `RFCManagerPostProcess()`, `SendEventToMaintenanceManager()` |
| `xconf_handler.h/cpp` | Device identity | `initializeXconfHandler()`, `ExecuteRequest()` |
| `rfc_xconf_handler.h/cpp` | Core RFC logic | `InitializeRuntimeFeatureControlProcessor()`, `ProcessRuntimeFeatureControlReq()`, `set_RFCProperty()`, `clearDB()` |
| `rdkc_rfc_xconf_handler.h/cpp` | Camera overrides | `CreateXconfHTTPUrl()`, `RetrieveHashAndTimeFromPreviousDataSet()`, `StoreXconfEndpointMetadata()` |
| `mtlsUtils.h/cpp` | Certificate management | `getMtlscert()`, `isStateRedSupported()`, `isInStateRed()` |
| `rfc_common.h/cpp` | Shared utilities | `read_RFCProperty()`, `getSyseventValue()`, `waitForRfcCompletion()` |
| `rfc_mgr_iarm.h` | IARM bus integration | Event handler registration constants |
| `rfc_mgr_json.h` | JSON field names | Feature/parameter key string constants |
| `rfc_mgr_key.h` | Config keys | TR-181 parameter name constants |

### Public API (`rfcapi/`)

| File | Responsibility | Key Functions |
|------|---------------|---------------|
| `rfcapi.h/cpp` | RFC parameter access | `getRFCParameter()`, `setRFCParameter()`, `isRFCEnabled()`, `getRFCErrorString()` |

### Data Model (`tr181api/`, `utils/`)

| File | Responsibility |
|------|---------------|
| `tr181api.h/cpp` | TR-181 data model read/write |
| `jsonhandler.h/cpp` | JSON parse/build helpers |
| `trsetutils.h/cpp` | TR-181 set operation utilities |
| `tr181utils.cpp` | TR-181 access layer |

---

## 7. Dependency Graph

```mermaid
graph TD
    subgraph "System Libraries"
        CURL[libcurl]
        CJSON[libcjson]
        PTHREAD[libpthread]
    end

    subgraph "RDK Platform Libraries"
        HOSTIF[libhostif<br/>STB only]
        RBUS[librbus<br/>RDKB only]
        MFRAPI[mfrApi_test<br/>RDKC only]
        CERTSELECTOR[librdkcertselector<br/>optional]
        DWNLUTIL[libdwnlutil<br/>RDKC only]
        FWUTILS[libfwutils<br/>RDKC only]
        SECWRAP[libsecure_wrapper]
    end

    subgraph "RFC Components"
        RFCMGR[rfcMgr]
        RFCAPI[librfcapi]
        TR181API[libtr181api]
        UTILS_LIB[utils]
    end

    RFCMGR --> CURL
    RFCMGR --> CJSON
    RFCMGR --> PTHREAD
    RFCMGR --> SECWRAP
    RFCMGR --> RFCAPI
    RFCMGR -.->|STB| HOSTIF
    RFCMGR -.->|RDKB| RBUS
    RFCMGR -.->|RDKC| DWNLUTIL
    RFCMGR -.->|RDKC| FWUTILS
    RFCMGR -.->|optional| CERTSELECTOR
    RFCAPI --> TR181API
    TR181API --> UTILS_LIB
    UTILS_LIB --> CJSON

    style RFCMGR fill:#bbdefb,stroke:#1565c0,stroke-width:2px
    style RFCAPI fill:#bbdefb,stroke:#1565c0,stroke-width:2px
```

---

## 8. External Integrations

```mermaid
graph LR
    subgraph "rfcMgr"
        RFC[RFC Daemon]
    end

    RFC -->|"HTTP GET + mTLS"| XCONF["Xconf Server<br/>featureControl/getSettings"]
    RFC -->|"read/write"| INI["tr181store.ini<br/>RFC parameters"]
    RFC -->|"read"| PROPS["rfc.properties<br/>Server URLs & paths"]
    RFC -->|"read"| DEVICE["Device Identity Files<br/>partnerid.txt<br/>service_number.txt<br/>accounthash.txt"]
    RFC -->|"execute"| MFRAPI["mfrApi_test<br/>MAC address (RDKC)"]
    RFC -->|"register events"| IARM["IARM Bus<br/>Maintenance events"]
    RFC -->|"trigger"| CRON["RfcRebootCronschedule.sh<br/>Reboot scheduling"]
    RFC -->|"notify"| TELEMETRY["telemetry2_0_client<br/>Feature status (non-RDKC)"]
    RFC -->|"read/write"| RAMFILES["/tmp/RFC/<br/>.hashValue &#8226; .timeValue"]

    style RFC fill:#bbdefb,stroke:#1565c0,stroke-width:2px
    style XCONF fill:#fff9c4,stroke:#f9a825,stroke-width:2px
```

### Integration Points Summary

| Integration | Protocol/Method | Platform |
|------------|-----------------|----------|
| Xconf Server | HTTP GET + mTLS (libcurl) | All |
| tr181store.ini | File I/O (flat key=value) | RDKC |
| TR-181 Data Model | hostif / rbus | STB / RDKB |
| IARM Bus | Event registration | STB |
| mfrApi_test | popen() command | RDKC |
| Maintenance Manager | IARM event | STB |
| Reboot Cron | v_secure_system() shell exec | RDKC |
| Telemetry | v_secure_system() call | STB / RDKB |

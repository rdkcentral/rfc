# Data Processing Flow

> Parameter lifecycle, storage strategies, data transformations, and platform-specific data paths for the RFC system.

---

## Table of Contents

- [1. End-to-End Data Flow Overview](#1-end-to-end-data-flow-overview)
- [2. Xconf Request Data Pipeline](#2-xconf-request-data-pipeline)
- [3. Xconf Response Processing Pipeline](#3-xconf-response-processing-pipeline)
- [4. Parameter Storage Architecture](#4-parameter-storage-architecture)
- [5. Hash & Timestamp Management](#5-hash--timestamp-management)
- [6. RFC State Machine](#6-rfc-state-machine)
- [7. ClearDB Data Flow](#7-cleardb-data-flow)
- [8. Platform Data Source Matrix](#8-platform-data-source-matrix)
- [9. Configuration File Formats](#9-configuration-file-formats)
- [10. Error Handling Flow](#10-error-handling-flow)

---

## 1. End-to-End Data Flow Overview

This diagram shows how data flows through the RFC system from external sources to persistent storage.

```mermaid
flowchart TD
    subgraph "Input Sources"
        DEV["Device Identity<br/>MAC • FW • Model<br/>Partner • Account"]
        CFG["rfc.properties<br/>Server URLs • Paths"]
        HASH_IN["Previous Hash/Time<br/>.hashValue • .timeValue"]
    end

    subgraph "Processing Engine"
        URL["URL Construction<br/>Query string assembly"]
        MTLS["mTLS Certificate<br/>Resolution"]
        HTTP["HTTP GET Request<br/>libcurl + mTLS"]
        PARSE["JSON Response<br/>Parser"]
        EVAL["Change Detection<br/>Hash comparison"]
        APPLY["Parameter<br/>Application"]
        REBOOT["Reboot<br/>Evaluation"]
    end

    subgraph "Output Storage"
        PARAMS["RFC Parameters<br/>tr181store.ini (RDKC)<br/>TR-181 DB (others)"]
        HASH_OUT["Updated Hash/Time<br/>/tmp/RFC/.hashValue<br/>/tmp/RFC/.timeValue"]
        CRON["Cron Schedule<br/>Periodic re-run"]
        REBOOT_OUT["Reboot Trigger<br/>RfcRebootCronschedule.sh"]
    end

    subgraph "External"
        XCONF["Xconf Server"]
    end

    DEV --> URL
    CFG --> URL
    HASH_IN --> EVAL
    URL --> HTTP
    MTLS --> HTTP
    HTTP --> XCONF
    XCONF --> HTTP
    HTTP --> PARSE
    PARSE --> EVAL
    EVAL -->|"Config changed"| APPLY
    EVAL -->|"No change"| HASH_OUT
    APPLY --> PARAMS
    APPLY --> REBOOT
    REBOOT -->|"Reboot needed"| REBOOT_OUT
    PARSE --> HASH_OUT
    APPLY --> CRON

    style XCONF fill:#fff9c4,stroke:#f9a825,stroke-width:2px
    style PARAMS fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px
    style REBOOT_OUT fill:#ffcdd2,stroke:#c62828,stroke-width:2px
```

---

## 2. Xconf Request Data Pipeline

How device data is collected, transformed, and assembled into the Xconf query URL.

```mermaid
flowchart LR
    subgraph "Raw Data Sources (RDKC)"
        A1["mfrApi_test 3 9<br/>→ raw MAC output"]
        A2["GetFirmwareVersion()<br/>→ version string"]
        A3["GetBuildType()<br/>→ enum"]
        A4["GetModelNum()<br/>→ model string"]
        A5["/opt/usr_config/<br/>partnerid.txt"]
        A6["/opt/usr_config/<br/>service_number.txt"]
        A7["/opt/usr_config/<br/>accounthash.txt"]
        A8["rfc.properties<br/>RFC_CONFIG_SERVER_URL"]
    end

    subgraph "Transform"
        T1["Parse popen() output<br/>Extract MAC string"]
        T2["Map enum to string<br/>dev/vbn/prod"]
        T3["Read file, trim<br/>whitespace/newline"]
        T4["URL-encode values"]
    end

    subgraph "URL Assembly"
        URL["https://xconf.server/<br/>featureControl/getSettings?<br/>estbMacAddress=MAC<br/>&firmwareVersion=FW<br/>&env=BUILD_TYPE<br/>&model=MODEL<br/>&accountHash=HASH<br/>&partnerId=PID<br/>&accountId=AID<br/>&experience=EXP<br/>&version=2"]
    end

    A1 --> T1 --> URL
    A2 --> URL
    A3 --> T2 --> URL
    A4 --> URL
    A5 --> T3 --> URL
    A6 --> T3
    A7 --> T3
    A8 --> URL
    T4 --> URL

    style URL fill:#bbdefb,stroke:#1565c0,stroke-width:2px
```

### URL Parameter Comparison

```mermaid
flowchart TB
    subgraph "RDKC Camera URL"
        C1[estbMacAddress]
        C2[firmwareVersion]
        C3[env]
        C4[model]
        C5[accountHash]
        C6[partnerId]
        C7[accountId]
        C8[experience]
        C9["version=2"]
    end

    subgraph "STB/RDKB URL"
        S1[estbMacAddress]
        S2[firmwareVersion]
        S3[env]
        S4[model]
        S5[ecmMacAddress]
        S6[controllerId]
        S7[channelMapId]
        S8[vodId]
        S9[partnerId]
        S10[accountId]
        S11[experience]
        S12[osclass]
        S13[manufacturer]
        S14["version=2"]
    end

    style C5 fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px
    style S5 fill:#ffcdd2,stroke:#c62828
    style S6 fill:#ffcdd2,stroke:#c62828
    style S7 fill:#ffcdd2,stroke:#c62828
    style S8 fill:#ffcdd2,stroke:#c62828
    style S12 fill:#ffcdd2,stroke:#c62828
    style S13 fill:#ffcdd2,stroke:#c62828
```

> **Green** = RDKC-only field. **Red** = Fields NOT present in RDKC URL.

---

## 3. Xconf Response Processing Pipeline

```mermaid
flowchart TD
    A["HTTP Response Body<br/>(JSON)"] --> B["cJSON_Parse()"]
    B --> C{"Parse successful?"}
    C -- No --> ERR["Log error, return failure"]
    C -- Yes --> D["Extract featureControl object"]

    D --> E["Iterate features[] array"]

    E --> F["For each feature:"]
    F --> G["Read feature name"]
    F --> H["Read effectiveImmediate flag"]
    F --> I["Read configData[] array"]

    I --> J["For each config param:"]
    J --> K["Extract key (TR-181 name)"]
    J --> L["Extract value"]
    J --> M["Extract dataType"]

    subgraph "effectiveImmediate Tracking (RDKC)"
        H -->|true| N["Add all param keys<br/>to _effectiveImmediateParams set"]
    end

    subgraph "Parameter Application"
        K --> O["set_RFCProperty(name, key, value)"]
        L --> O
        M --> O
    end

    subgraph "Hash Computation"
        O --> P["Compute hash of<br/>all applied parameters"]
        P --> Q{"Hash different<br/>from stored?"}
        Q -- Yes --> R["Configuration changed<br/>Update hash & time"]
        Q -- No --> S["No changes detected<br/>Skip update"]
    end

    style A fill:#fff9c4,stroke:#f9a825
    style O fill:#c8e6c9,stroke:#2e7d32
    style R fill:#bbdefb,stroke:#1565c0
```

### JSON Response Structure

```
{
  "featureControl": {
    "features": [
      {
        "name": "AccountInfo",
        "effectiveImmediate": true,
        "enable": true,
        "configData": [
          {
            "key": "Device.X_RDK.AccountID",
            "value": "5789171196032993066",
            "dataType": 0
          },
          {
            "key": "Device.X_RDK.MD5AccountHash",
            "value": "1EMvt8zMMd8muKCBJRnp1z6nZTgsBJ1VhL",
            "dataType": 0
          }
        ]
      }
    ]
  }
}
```

---

## 4. Parameter Storage Architecture

### Write Path

```mermaid
flowchart TD
    A["set_RFCProperty(featureName, key, value)"]
    
    A --> B{"Platform?"}
    
    B -- "RDKC" --> C["Read /opt/secure/RFC/tr181store.ini"]
    C --> D["Parse into lines[]"]
    D --> E{"Key exists?"}
    E -- Yes --> F["Replace: line = key=value"]
    E -- No --> G["Append: lines.push_back(key=value)"]
    F --> H["Write all lines to tr181store.ini"]
    G --> H
    H --> I["Return WDMP_SUCCESS"]

    B -- "RDKB" --> J["rbus_open(handle)"]
    J --> K["rbus_set(handle, key, value)"]
    K --> L["rbus_close(handle)"]
    L --> I

    B -- "STB" --> M["setRFCParameter(key, value, type)<br/>via hostif/WDMP"]
    M --> I

    style C fill:#c8e6c9,stroke:#2e7d32
    style H fill:#c8e6c9,stroke:#2e7d32
```

### Read Path (rfcapi)

```mermaid
flowchart TD
    A["getRFCParameter(name, key, &param)"]
    
    A --> B{"Platform / Backend?"}
    
    B -- "Flat-file (RDKC)" --> C["Open tr181store.ini"]
    C --> D["Scan for key= prefix"]
    D --> E{"Found?"}
    E -- Yes --> F["Extract value after '='"]
    F --> G["param.value = value<br/>param.type = STRING"]
    E -- No --> H["Return WDMP_ERR_VALUE_IS_EMPTY"]

    B -- "WDMP (STB)" --> I["Build WDMP request"]
    I --> J["Send to hostif backend"]
    J --> K["Parse WDMP response"]
    K --> G

    B -- "TR-181 int (RDKB)" --> L["rbus_get(key)"]
    L --> M["Parse rbus response"]
    M --> G

    G --> N["Return 0 (success)"]

    style C fill:#c8e6c9,stroke:#2e7d32
```

### File Format: tr181store.ini

```
Device.X_RDK.Feature.AccountInfo.Enable=true
Device.X_RDK.AccountID=5789171196032993066
Device.X_RDK.MD5AccountHash=1EMvt8zMMd8muKCBJRnp1z6nZTgsBJ1VhL
Device.X_RDK.Feature.VideoAnalytics.Enable=true
Device.X_RDK.Feature.AmbientListening.Enable=false
```

---

## 5. Hash & Timestamp Management

```mermaid
flowchart TD
    subgraph "Read Phase"
        R1["GetStoredHashAndTime()"]
        R1 --> R2{"RDKC?"}
        R2 -- Yes --> R3["Always call<br/>RetrieveHashAndTime<br/>FromPreviousDataSet()"]
        R2 -- No --> R4["Check XconfSelector<br/>slot first"]
        R4 --> R5{"Slot matches?"}
        R5 -- Yes --> R6["Read from that slot"]
        R5 -- No --> R3

        R3 --> R7{"RDKC override"}
        R7 --> R8["Read /tmp/RFC/.hashValue<br/>Default: UPGRADE_HASH"]
        R7 --> R9["Read /tmp/RFC/.timeValue<br/>Default: 0"]
    end

    subgraph "Compare Phase"
        CP1["New hash from<br/>current config"] --> CP2{"Hash matches<br/>stored hash?"}
        CP2 -- Yes --> CP3["Config unchanged<br/>Skip parameter apply"]
        CP2 -- No --> CP4["Config changed<br/>Proceed with apply"]
        
        CP5["First request?<br/>(FW changed or<br/>no prior hash)"] --> CP6{"Is first?"}
        CP6 -- Yes --> CP4
        CP6 -- No --> CP2
    end

    subgraph "Write Phase"
        W1["updateHashInDB(hash)"]
        W1 --> W2{"RDKB or RDKC?"}
        W2 -- Yes --> W3["Write /tmp/RFC/.hashValue"]
        W2 -- No --> W4["set_RFCProperty(ConfigSetHash)"]

        W5["updateTimeInDB(time)"]
        W5 --> W6{"RDKB or RDKC?"}
        W6 -- Yes --> W7["Write /tmp/RFC/.timeValue"]
        W6 -- No --> W8["set_RFCProperty(ConfigSetTime)"]
    end

    style R8 fill:#c8e6c9,stroke:#2e7d32
    style R9 fill:#c8e6c9,stroke:#2e7d32
    style W3 fill:#c8e6c9,stroke:#2e7d32
    style W7 fill:#c8e6c9,stroke:#2e7d32
```

### Storage Location Matrix

| Data | STB | RDKB | RDKC |
|------|-----|------|------|
| **Config Hash** | TR-181 DB (`ConfigSetHash`) | `/tmp/RFC/.hashValue` (RAM) | `/tmp/RFC/.hashValue` (RAM) |
| **Config Time** | TR-181 DB (`ConfigSetTime`) | `/tmp/RFC/.timeValue` (RAM) | `/tmp/RFC/.timeValue` (RAM) |
| **Xconf URL** | TR-181 DB (`XconfUrl`) | TR-181 DB | **Not stored** (no-op) |
| **Xconf Selector** | TR-181 DB (`XconfSelector`) | TR-181 DB | **Not stored** (no-op) |

> **Note:** RDKC uses RAM files that do NOT survive reboot. This means every boot is treated as a fresh request, which is intentional for camera devices.

---

## 6. RFC State Machine

```mermaid
stateDiagram-v2
    [*] --> Init : ProcessRuntimeFeatureControlReq()

    Init --> CheckLocal : Load rfc.properties
    
    CheckLocal --> LocalOverride : Local RFC file exists
    CheckLocal --> Redo : No local override
    
    LocalOverride --> Finish : Apply local config
    
    Redo --> BuildURL : CreateXconfHTTPUrl()
    BuildURL --> AcquireCert : getMtlscert()
    AcquireCert --> HttpRequest : ExecuteRequest()
    
    HttpRequest --> ParseResponse : HTTP 200
    HttpRequest --> Retry : HTTP error
    
    Retry --> Redo : Retry with backoff
    Retry --> Finish : Max retries exceeded
    
    ParseResponse --> CompareHash : ProcessJsonResponse()
    
    CompareHash --> ApplyParams : Hash changed / first request
    CompareHash --> Finish : Hash unchanged
    
    ApplyParams --> ClearDB : clearDB()
    ClearDB --> SetParams : set_RFCProperty() loop
    SetParams --> EvalReboot : Check effectiveImmediate [RDKC]
    EvalReboot --> UpdateHash : updateHashInDB()
    UpdateHash --> UpdateTime : updateTimeInDB()
    UpdateTime --> StoreMetadata : StoreXconfEndpointMetadata()
    StoreMetadata --> NotifyTelemetry
    
    NotifyTelemetry --> Finish : [RDKC: skip telemetry]
    
    Finish --> [*]

    note right of Init
        RfcState enum:
        Init, Local, Redo,
        Redo_With_Valid_Data,
        Finish
    end note

    note right of ClearDB
        RDKC: std::remove(tr181store.ini)
        Others: TR-181 ClearDB parameter
    end note
```

---

## 7. ClearDB Data Flow

```mermaid
flowchart TD
    A["clearDB() called"] --> B{"Platform?"}
    
    B -- "RDKC" --> C["std::remove(<br/>/opt/secure/RFC/tr181store.ini)"]
    C --> D["rfcStashRetrieveParams()"]
    D --> E["Restore stashed AccountID<br/>and other identity params"]
    
    B -- "STB / RDKB" --> F["Create empty tr181store.ini"]
    F --> G["set_RFCProperty(ClearDB, true)"]
    G --> H["set_RFCProperty(BootstrapClearDB, true)"]
    H --> I["set_RFCProperty(ConfigChangeTime, timestamp)"]
    
    I --> J["clearDBEnd()"]
    J --> K{"RDKC or RDKB?"}
    K -- Yes --> L["SKIP clearDBEnd()"]
    K -- No --> M["set_RFCProperty(ClearDBEnd, true)"]
    M --> N["set_RFCProperty(BootstrapClearDBEnd, true)"]
    N --> O["set_RFCProperty(ReloadCache, true)"]

    style C fill:#c8e6c9,stroke:#2e7d32
    style L fill:#c8e6c9,stroke:#2e7d32
```

---

## 8. Platform Data Source Matrix

### Device Identity

| Data Field | STB Source | RDKB Source | RDKC Source |
|------------|-----------|-------------|-------------|
| MAC Address | `GetEstbMac()` → `/tmp/.estb_mac` | `getErouterMac()` | `popen("mfrApi_test 3 9")` |
| Firmware Version | `GetFirmwareVersion()` | `GetFirmwareVersion()` | `GetFirmwareVersion()` |
| Build Type | `GetBuildType()` | `GetBuildType()` | `GetBuildType()` |
| Model Number | `GetModelNum()` | `GetModelNum()` | `GetModelNum()` |
| Manufacturer | `GetMFRName()` | `GetMFRName()` | **Skipped** |
| Partner ID | `GetPartnerId()` → `/opt/partnerid` | `GetPartnerId()` | `fopen("/opt/usr_config/partnerid.txt")` |
| Account ID | `read_RFCProperty()` (TR-181) | `read_RFCProperty()` (rbus) | `ifstream("/opt/usr_config/service_number.txt")` |
| Account Hash | N/A | N/A | `ifstream("/opt/usr_config/accounthash.txt")` |
| ECM MAC | N/A | `geteCMMac()` | N/A |

### Parameter Storage

| Operation | STB | RDKB | RDKC |
|-----------|-----|------|------|
| **Write** | `setRFCParameter()` via hostif | `rbus_set()` | Write to `tr181store.ini` |
| **Read** | `getRFCParameter()` via WDMP | `rbus_get()` | Scan `tr181store.ini` |
| **Clear** | TR-181 ClearDB params | TR-181 ClearDB params | `std::remove(tr181store.ini)` |

### mTLS Certificate

| Strategy | STB | RDKB | RDKC |
|----------|-----|------|------|
| **Primary** | `librdkcertselector` | `librdkcertselector` | Dynamic XPKI (`/opt/certs/devicecert_1.pk12`) |
| **Fallback** | Cert selector retry | Cert selector retry | Static XPKI (`/etc/ssl/certs/staticXpkiCrt.pk12`) |
| **Last resort** | Fail | Fail | Proceed without mTLS |

### Reboot Mechanism

| Aspect | STB | RDKB | RDKC |
|--------|-----|------|------|
| **Trigger** | MaintenanceManager | MaintenanceManager | `RfcRebootCronschedule.sh` |
| **Conditions** | `effectiveImmediate` flag | `effectiveImmediate` flag | `effectiveImmediate` + provisioned + not identity param |
| **Notification** | IARM event | IARM event | Shell script (background) |

---

## 9. Configuration File Formats

### rfc.properties

```properties
# Server endpoints
RFC_CONFIG_SERVER_URL=https://xconf.example.com/featureControl/getSettings
RFC_CONFIG_SERVER_URL_EU=https://xconf-eu.example.com/featureControl/getSettings

# Path configuration
export RFC_RAM_PATH="/tmp/RFC"
TR181_STORE_FILENAME="/opt/secure/RFC/tr181store.ini"
RFC_VAR_FILENAME="/opt/secure/RFC/rfcVariable.ini"
BS_STORE_FILENAME="/opt/secure/RFC/bootstrap.ini"

# Process management
RFC_SERVICE_LOCK="/tmp/.rfcServiceLock"
RFC_WRITE_LOCK="/tmp/.rfcWriteLock"

# Tools & scripts
RFC_WHITELIST_TOOL="rfctool"
RFC_POSTPROCESS="/lib/rdk/RFCpostprocess.sh"
```

### tr181store.ini (RFC Parameters)

```ini
# Format: TR-181_parameter_name=value
Device.X_RDK.Feature.AccountInfo.Enable=true
Device.X_RDK.AccountID=5789171196032993066
Device.X_RDK.MD5AccountHash=1EMvt8zMMd8muKCBJRnp1z6nZTgsBJ1VhL
Device.X_RDK.Feature.VideoAnalytics.Enable=true
```

### .hashValue / .timeValue (RAM files)

```
# /tmp/RFC/.hashValue
a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5

# /tmp/RFC/.timeValue
1712937600
```

---

## 10. Error Handling Flow

```mermaid
flowchart TD
    A["rfcMgr starts"] --> B{"Lock file exists?"}
    B -- Yes --> C["Log: Already running<br/>Exit immediately"]
    B -- No --> D["Create lock file"]
    
    D --> E{"Network available?"}
    E -- No --> F["Poll every 10s<br/>(RDKC: getifaddrs loop)"]
    F --> E
    E -- Yes --> G["Proceed with RFC"]

    G --> H{"Xconf URL valid?"}
    H -- No --> I["Log error<br/>Return failure"]
    H -- Yes --> J["HTTP Request"]

    J --> K{"HTTP success?"}
    K -- No --> L{"Retry count < max?"}
    L -- Yes --> M["Backoff & retry"]
    M --> J
    L -- No --> N["Log: Max retries<br/>Record error state"]
    K -- Yes --> O["Parse JSON"]

    O --> P{"JSON valid?"}
    P -- No --> Q["Log parse error<br/>Return failure"]
    P -- Yes --> R["Apply parameters"]

    R --> S{"Write error?"}
    S -- Yes --> T["Log write error<br/>Continue with next param"]
    S -- No --> U["Parameter applied"]

    U --> V["Cleanup & exit"]
    N --> V
    I --> V

    V --> W["Remove lock file"]

    style C fill:#ffcdd2,stroke:#c62828
    style N fill:#ffcdd2,stroke:#c62828
    style Q fill:#ffcdd2,stroke:#c62828
    style T fill:#fff9c4,stroke:#f9a825
    style U fill:#c8e6c9,stroke:#2e7d32
```

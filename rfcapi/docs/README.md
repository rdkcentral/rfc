# rfcapi — RFC Parameter Get/Set API

## Overview

`librfcapi` is a C/C++ library that provides the canonical interface for reading and writing RFC (Remote Feature Control) parameters on RDK devices. It resolves parameter values from a layered file store: XConf-applied overrides first, then component defaults. All other RDK components use this library instead of accessing the INI files directly.

---

## Architecture

```mermaid
graph TB
    caller["RDK Component\n(caller)"]
    rfcapi["librfcapi\n(rfcapi.cpp)"]
    tr69["tr69hostif HTTP server\n(127.0.0.1:11999)"]
    rfcVar["/opt/secure/RFC/\nrfcVariable.ini"]
    tr181store["/opt/secure/RFC/\ntr181store.ini"]
    rfcdefaults["/tmp/rfcdefaults.ini\n(merged from /etc/rfcdefaults/*.ini)"]

    caller -->|"getRFCParameter()"| rfcapi
    caller -->|"setRFCParameter()"| rfcapi
    rfcapi -->|"RFC_xxxx keys"| rfcVar
    rfcapi -->|"Device.* TR181 keys"| tr181store
    rfcapi -->|"fallback"| rfcdefaults
    rfcapi -->|"live parameter\n(when server ready)"| tr69
```

### Lookup Priority

```mermaid
flowchart LR
    A[getRFCParameter called] --> B{Key starts with RFC_\nand no dot?}
    B -->|Yes| C[Read rfcVariable.ini]
    B -->|No| D[Read tr181store.ini]
    D --> E{Found?}
    E -->|Yes| F[Return value]
    E -->|No| G[Read rfcdefaults.ini\n merged from /etc/rfcdefaults/]
    G --> H{Found?}
    H -->|Yes| F
    H -->|No| I[Return WDMP_FAILURE]
    C --> J{Found?}
    J -->|Yes| F
    J -->|No| I
```

---

## Key Components

### `RFC_ParamData_t`

Holds a single RFC parameter name–value pair.

```c
typedef struct _RFC_Param_t {
    char name[MAX_PARAM_LEN];   /* Parameter name  (max 2048 chars) */
    char value[MAX_PARAM_LEN];  /* Parameter value (max 2048 chars) */
    DATA_TYPE type;             /* Value type enum */
} RFC_ParamData_t;

#define MAX_PARAM_LEN  (2 * 1024)
```

### `DATA_TYPE`

```c
typedef enum {
    SUCCESS  = 0,   /* Used as return value AND type for RDKB */
    FAILURE  = 1,
    NONE     = 2,   /* Type unknown / not set */
    EMPTY    = 3    /* Key found but value is empty */
} DATA_TYPE;
```

---

## API Reference

### `getRFCParameter()`

Reads a single RFC parameter value from the local file store.

**Signature (non-RDKB):**
```c
WDMP_STATUS getRFCParameter(const char *pcCallerID,
                             const char *pcParameterName,
                             RFC_ParamData_t *pstParamData);
```

**Signature (RDKB):**
```c
int getRFCParameter(const char *pcParameterName,
                    RFC_ParamData_t *pstParamData);
```

**Parameters:**
- `pcCallerID` — Component name used for logging (non-RDKB only; may be `NULL`)
- `pcParameterName` — Full TR181 path or `RFC_xxxx` key; must not end with `.`
- `pstParamData` — Output buffer; must be non-NULL; caller allocates

**Returns (non-RDKB):**

| Value | Meaning |
|-------|---------|
| `WDMP_SUCCESS` | Value found and copied to `pstParamData` |
| `WDMP_ERR_DEFAULT_VALUE` | Value returned from defaults file |
| `WDMP_FAILURE` | Key not found in any store |

**Returns (RDKB):** `SUCCESS` (0) or `FAILURE` (-1)

**Thread Safety:** Read-only file access. Safe to call from multiple threads concurrently.

**Example:**
```c
#include "rfcapi.h"
#include <stdio.h>

void check_account_id(void) {
    RFC_ParamData_t param;
    WDMP_STATUS ret;

    ret = getRFCParameter("mycomponent",
        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID",
        &param);

    if (ret == WDMP_SUCCESS) {
        printf("AccountID = %s\n", param.value);
    } else {
        printf("Not found: %s\n", getRFCErrorString(ret));
    }
}
```

---

### `setRFCParameter()`

Writes an RFC parameter value to the tr69hostif HTTP server.

**Signature:**
```c
WDMP_STATUS setRFCParameter(const char *pcCallerID,
                             const char *pcParameterName,
                             const char *pcParameterValue,
                             DATA_TYPE eDataType);
```

**Parameters:**
- `pcCallerID` — Component name (used for logging)
- `pcParameterName` — Full TR181 path; must not be NULL or empty
- `pcParameterValue` — Value string; must not be NULL
- `eDataType` — Data type from `DATA_TYPE` enum

**Returns:**

| Value | Meaning |
|-------|---------|
| `WDMP_SUCCESS` | Parameter written successfully |
| `WDMP_FAILURE` | Write rejected or server error |

**Thread Safety:** HTTP POST to localhost; safe for concurrent callers.

**Example:**
```c
WDMP_STATUS ret = setRFCParameter(
    "mycomponent",
    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable",
    "true",
    WDMP_BOOLEAN);

if (ret != WDMP_SUCCESS) {
    fprintf(stderr, "Set failed: %s\n", getRFCErrorString(ret));
}
```

---

### `isRFCEnabled()`

Convenience wrapper: reads a boolean RFC feature flag and returns `true` if its value is `"true"`.

**Signature:**
```c
bool isRFCEnabled(const char *pcParameterName);
```

**Example:**
```c
if (isRFCEnabled("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MTLS.mTlsXConfDownload.Enable")) {
    /* enable mTLS path */
}
```

---

### `getRFCErrorString()`

Converts a `WDMP_STATUS` code to a human-readable string.

**Signature:**
```c
const char *getRFCErrorString(WDMP_STATUS code);
```

---

### `isFileInDirectory()`

Checks whether a file exists within a specified directory.

**Signature:**
```c
bool isFileInDirectory(const char *filename, const char *directory);
```

---

## File Store Details

| File | Key pattern | Written by |
|------|-------------|------------|
| `/opt/secure/RFC/rfcVariable.ini` | `RFC_xxxx` (no dot) | rfcMgr XConf apply |
| `/opt/secure/RFC/tr181store.ini` | `Device.*` TR181 paths | rfcMgr XConf apply |
| `/tmp/rfcdefaults.ini` | Any | Generated at runtime from `/etc/rfcdefaults/*.ini` |
| `/opt/secure/RFC/bootstrap.ini` | Bootstrap TR181 keys | Platform provisioning |

---

## Default Value Resolution

`getRFCParameter` merges all `.ini` files under `/etc/rfcdefaults/` into `/tmp/rfcdefaults.ini` on first access if the merged file does not exist. Component default files must be named `<componentname>.ini` and placed in `/etc/rfcdefaults/`.

```mermaid
graph TD
    A["/etc/rfcdefaults/\nauth.ini\ntelemetry.ini\nip.ini\n..."] -->|"concat at runtime"| B["/tmp/rfcdefaults.ini"]
    B -->|"fallback lookup"| C["getRFCParameter()"]
```

---

## Platform Variants

### RDK-V / RDK-C
- Uses `WDMP_STATUS` return type via `wdmp-c` library
- Full set: `getRFCParameter`, `setRFCParameter`, `isRFCEnabled`, `isFileInDirectory`
- `setRFCParameter` sends HTTP POST to `http://127.0.0.1:11999`

### RDK-B (`RDKB_SUPPORT`)
- Simplified `int` return type
- `setRFCParameter` not exposed from this library (rfcMgr applies directly)
- `MAX_PARAM_LEN` applies identically

---

## Testing

```bash
# Build gtest binary
./configure --enable-gtestapp=yes
make

# Run rfcapi unit tests
./rfcMgr/gtest/rfcapi_gtest

# With memory checking
valgrind --leak-check=full ./rfcMgr/gtest/rfcapi_gtest
```

Mocks for `curl`, `rdk_debug`, and file I/O are in `rfcMgr/gtest/mocks/`.

---

## See Also

- [tr181api Reference](../tr181api/docs/README.md) — Higher-level typed TR181 API
- [RFC Module Overview](../../README.md)
- [RFC API Header](../rfcapi.h)

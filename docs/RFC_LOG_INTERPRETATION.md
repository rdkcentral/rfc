# RFC Log Interpretation Guide

This guide helps you understand RFC (Remote Feature Control) logs and determine if the system is working correctly.

## Overview

RFC is a remote feature control system that allows enabling/disabling features on RDK devices without code changes. The system fetches feature configurations from an xconf server and stores them locally as `.ini` files.

## Key Components

1. **RFCManager** - C++ service that fetches configurations from xconf server
2. **getRFC.sh** - Shell utility that reads RFC feature configuration files
3. **isFeatureEnabled.sh** - Checks if a specific feature is enabled

## Understanding RFC Logs

### Normal Startup Sequence

A successful RFC startup shows the following sequence:

```
[RFC] RFC: Starting service, creating lock
[RFC] Waiting for IP Acquistion
[RFC] Checking IP and Route configuration
[RFC] DNS Nameservers are available
[RFC] Starting execution of RFCManager
[RFC] RFC:Device is Online
[RFC] getRFCParameter: http server is ready
```

**What this means**: RFC Manager is starting successfully and the device has network connectivity.

### Configuration Download

When RFC downloads configurations, you'll see:

```
[RFC] Requesting /opt/secure/RFC/.RFC_<FEATURE>.ini
[RFC] Sourced /opt/secure/RFC/.RFC_<FEATURE>.ini
```

**What this means**: RFC is successfully reading feature configuration files.

### Feature Not Configured - "NOT IN RFC!"

```
[IFE]:: Requesting WIFI_TM_DC (RFC_ENABLE_WIFI_TM_DC) NOT IN RFC!
[IFE]:: isFeatureEnabled WIFI_TM_DC Returns 0
```

**What this means**: 
- The feature configuration file `/opt/secure/RFC/.RFC_<FEATURE>.ini` does not exist
- This is **EXPECTED BEHAVIOR** when a feature is not configured on the xconf server
- The system gracefully defaults to disabled (returns 0)
- This is **NOT an error** - it's normal operation for unconfigured features

### Feature Enabled

```
[IFE]:: Requesting FEATURE_NAME (RFC_ENABLE_FEATURE_NAME) = true
[IFE]:: isFeatureEnabled FEATURE_NAME Returns 1
```

**What this means**: The feature is configured and enabled.

### Feature Disabled

```
[IFE]:: Requesting FEATURE_NAME (RFC_ENABLE_FEATURE_NAME) = false
[IFE]:: isFeatureEnabled FEATURE_NAME Returns 0
```

**What this means**: The feature is configured but explicitly disabled.

## How to Verify RFC is Working

### 1. Check RFC Manager Started Successfully

Look for these log messages in sequence:
```
RFC: Starting service, creating lock
RFC:Device is Online
getRFCParameter: http server is ready
```

If you see these, RFC Manager is running correctly.

### 2. Check Network Connectivity

Look for:
```
Checking IP and Route configuration found
DNS Nameservers are available
```

If missing, RFC cannot connect to the xconf server.

### 3. Check Configuration Download

Look for successful MTLS connection:
```
MTLS is enable
MTLS creds for SSR fetched ret=0
```

And successful configuration retrieval (HTTP 200 responses):
```
curl response : 0 http response code: 200
```

If you see HTTP errors (404, 500, etc.), RFC cannot fetch configurations.

### 4. Check Feature Files

Verify feature configuration files exist:
```bash
ls -la /opt/secure/RFC/
```

You should see `.RFC_*.ini` files for each configured feature.

## Common Scenarios

### Scenario 1: RFC Working, Feature Not Configured

**Log Pattern:**
```
[RFC] RFC:Device is Online
[RFC] curl response : 0 http response code: 200
[IFE]:: Requesting FEATURE (RFC_ENABLE_FEATURE) NOT IN RFC!
[IFE]:: isFeatureEnabled FEATURE Returns 0
```

**Interpretation**: 
- RFC system is working correctly ✓
- The feature is simply not configured on the xconf server
- This is expected if the feature isn't meant to be used on this device
- **No action needed** unless you expected the feature to be configured

### Scenario 2: RFC Working, Feature Enabled

**Log Pattern:**
```
[RFC] RFC:Device is Online
[RFC] Requesting /opt/secure/RFC/.RFC_FEATURE.ini
[RFC] Sourced /opt/secure/RFC/.RFC_FEATURE.ini
[IFE]:: Requesting FEATURE (RFC_ENABLE_FEATURE) = true
[IFE]:: isFeatureEnabled FEATURE Returns 1
```

**Interpretation**: 
- RFC system is working correctly ✓
- Feature is configured and enabled ✓
- Everything is functioning as expected

### Scenario 3: RFC Cannot Connect to Server

**Log Pattern:**
```
[RFC] RFC:Device is Online
[RFC] curl response : <non-zero> http response code: <error>
```

or

```
[RFC] Checking IP and Route configuration
(missing: "Checking IP and Route configuration found")
```

**Interpretation**: 
- RFC Manager started but cannot reach xconf server
- **Action needed**: Check network connectivity, DNS, firewall rules

### Scenario 4: RFC Manager Not Starting

**Log Pattern:**
```
IARM_Bus_IsConnected invalid state
(missing: "RFC: Starting service, creating lock")
```

**Interpretation**: 
- RFC Manager failed to start
- **Action needed**: Check for errors in startup logs, verify IARM bus is running

## Return Values

When `isFeatureEnabled.sh` is called:

- **Returns 1**: Feature is configured and enabled (`RFC_ENABLE_FEATURE=true`)
- **Returns 0**: Feature is not configured OR configured as disabled (`RFC_ENABLE_FEATURE=false` or file doesn't exist)
- **Returns -1**: Error in script parameters

## Troubleshooting

### Feature Not Working as Expected

1. **Check if feature is configured:**
   ```bash
   ls /opt/secure/RFC/.RFC_<FEATURE>.ini
   ```

2. **Check feature value:**
   ```bash
   cat /opt/secure/RFC/.RFC_<FEATURE>.ini
   ```
   Look for `RFC_ENABLE_<FEATURE>=true` or `RFC_ENABLE_<FEATURE>=false`

3. **Check xconf server configuration:**
   - Verify feature is defined on the xconf server
   - Check device is assigned to correct feature rollout

4. **Check RFC Manager logs:**
   ```bash
   cat /opt/logs/rfcscript.log | grep -A5 -B5 <FEATURE>
   ```

### "NOT IN RFC!" is Not an Error

Many users misinterpret "NOT IN RFC!" as an error. This message simply means:
- The feature configuration file was not found
- The system defaults to disabled (safe default)
- This is normal for features that aren't configured for this device

**Only take action if:**
- You expected the feature to be configured
- The feature should be enabled for this device type
- The xconf server should have this feature defined

## Example Analysis

Given this log snippet:
```
260120-01:23:29.914881 [mod=RFCMGR, lvl=INFO] RFC:Device is Online
260120-01:23:29.915133 [mod=RFCAPI, lvl=INFO] getRFCParameter: http server is ready
260120-01:23:29.931946 [mod=RFCAPI, lvl=INFO] curl response : 0 http response code: 200
2026-01-20T01:26:25.464Z [RFC] Requesting /opt/secure/RFC/.RFC_WIFI_TM_DC.ini
2026-01-20T01:26:25.478Z [IFE]:: Requesting WIFI_TM_DC (RFC_ENABLE_WIFI_TM_DC) NOT IN RFC!
2026-01-20T01:26:25.491Z [IFE]:: isFeatureEnabled WIFI_TM_DC Returns 0
```

**Analysis:**
- ✓ RFC Manager started successfully
- ✓ Device is online with network connectivity
- ✓ RFC can communicate with xconf server (HTTP 200)
- ✓ RFC is functioning correctly
- ℹ️ WIFI_TM_DC feature is not configured (this is expected behavior)
- ℹ️ Feature defaults to disabled (returns 0)

**Conclusion**: RFC is working correctly. The WIFI_TM_DC feature is simply not configured on the xconf server for this device, which is normal behavior.

## Additional Resources

- **getRFC.sh**: Script that sources RFC configuration files
- **isFeatureEnabled.sh**: Script that checks if a feature is enabled
- **rfcMgr**: Main RFC Manager C++ service
- **Configuration files**: Located in `/opt/secure/RFC/` with naming pattern `.RFC_<FEATURE>.ini`

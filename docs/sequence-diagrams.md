# Sequence Diagrams

> End-to-end execution flows for the RFC daemon across all operational phases.

---

## Table of Contents

- [1. Complete Startup to Shutdown](#1-complete-startup-to-shutdown)
- [2. Device Online Detection](#2-device-online-detection)
- [3. Device Identity Collection](#3-device-identity-collection)
- [4. Xconf Query & Response Processing](#4-xconf-query--response-processing)
- [5. mTLS Certificate Acquisition](#5-mtls-certificate-acquisition)
- [6. Parameter Application](#6-parameter-application)
- [7. Reboot Evaluation & Scheduling](#7-reboot-evaluation--scheduling)
- [8. Post-Processing & Cleanup](#8-post-processing--cleanup)

---

## 1. Complete Startup to Shutdown

The full lifecycle of a single `rfcMgr` invocation from process start to exit.

```mermaid
sequenceDiagram
    participant OS as Operating System
    participant Main as rfc_main.cpp
    participant RFCMgr as RFCManager
    participant Proc as RuntimeFeatureControl<br/>Processor
    participant Xconf as Xconf Server
    participant FS as File System

    Note over OS,FS: Phase 1 — Process Initialization
    OS->>Main: exec /usr/bin/rfcMgr
    Main->>Main: fork() — daemonize
    Main->>Main: atexit(cleanup_lock_file)
    Main->>Main: signal(SIGINT, signal_handler)
    Main->>Main: signal(SIGTERM, signal_handler)
    Main->>RFCMgr: new RFCManager()
    Main->>FS: createDirectoryIfNotExists("/opt/secure/RFC")
    Main->>FS: createDirectoryIfNotExists("/tmp/RFC")
    Main->>FS: Create lock file /tmp/.rfcServiceLock

    Note over OS,FS: Phase 2 — Network Readiness
    Main->>RFCMgr: CheckDeviceIsOnline()
    loop Until network available
        RFCMgr->>RFCMgr: Poll network interfaces
        RFCMgr->>RFCMgr: sleep(10)
    end
    RFCMgr-->>Main: RFCMGR_DEVICE_ONLINE

    Note over OS,FS: Phase 3 — RFC Processing
    Main->>RFCMgr: RFCManagerProcess()
    RFCMgr->>RFCMgr: sleep(120) [RDKC only]
    RFCMgr->>Proc: new Processor() [platform-specific]
    RFCMgr->>Proc: InitializeRuntimeFeatureControlProcessor()
    RFCMgr->>Proc: ProcessRuntimeFeatureControlReq()
    Proc->>Xconf: HTTP GET + mTLS
    Xconf-->>Proc: HTTP 200 + JSON
    Proc->>FS: Apply parameters (set_RFCProperty)
    Proc->>FS: Update hash & timestamp
    Proc-->>RFCMgr: SUCCESS

    Note over OS,FS: Phase 4 — Post-Processing & Exit
    RFCMgr->>RFCMgr: Evaluate reboot need
    RFCMgr->>RFCMgr: RFCManagerPostProcess()
    RFCMgr-->>Main: Return
    Main->>FS: Remove lock file
    Main->>OS: exit(0)
```

---

## 2. Device Online Detection

### RDKC Camera Path

```mermaid
sequenceDiagram
    participant MGR as RFCManager
    participant NET as Network Stack
    participant SYS as System (getifaddrs)

    MGR->>MGR: CheckDeviceIsOnline()
    
    loop Poll until IP acquired
        MGR->>SYS: getifaddrs(&ifaddr)
        SYS-->>MGR: Linked list of interfaces
        
        loop For each interface
            MGR->>MGR: Skip loopback (lo)
            MGR->>MGR: Check AF_INET or AF_INET6
            
            alt AF_INET (IPv4)
                MGR->>MGR: inet_ntop() → IP string
                alt Is valid non-link-local IP
                    MGR->>MGR: Log "IPv4: <address>"
                    MGR-->>MGR: break — IP found
                end
            else AF_INET6
                MGR->>MGR: inet_ntop() → IP string
                alt Is valid non-link-local IP
                    MGR->>MGR: Log "IPv6: <address>"
                    MGR-->>MGR: break — IP found
                end
            end
        end

        alt No valid IP found
            MGR->>SYS: freeifaddrs(ifaddr)
            MGR->>MGR: sleep(10)
        else Valid IP found
            MGR->>SYS: freeifaddrs(ifaddr)
            MGR-->>MGR: Return RFCMGR_DEVICE_ONLINE
        end
    end
```

### Platform Comparison

```mermaid
sequenceDiagram
    participant MGR as RFCManager

    alt RDKC Camera
        MGR->>MGR: getifaddrs() loop<br/>Every 10s until non-loopback IP found
    else RDKB Broadband
        MGR->>MGR: CheckIPConnectivity()<br/>via dmcli eRouter query
        MGR->>MGR: CheckIProuteConnectivity()<br/>via ip route get
        MGR->>MGR: isDnsResolve()<br/>via nslookup
    else STB Set-Top Box
        MGR->>MGR: Check /tmp/route_available exists
        MGR->>MGR: Check /etc/resolv.dnsmasq exists
    end
```

---

## 3. Device Identity Collection

```mermaid
sequenceDiagram
    participant Init as InitializeRuntime<br/>FeatureControlProcessor
    participant XH as XconfHandler::<br/>initializeXconfHandler()
    participant Platform as Platform APIs
    participant Files as Device Files

    Init->>Init: Skip GetBootstrapXconfUrl() [RDKC]

    Init->>XH: initializeXconfHandler()

    rect rgb(200, 230, 255)
        Note over XH,Platform: MAC Address
        alt STB
            XH->>Platform: GetEstbMac() → /tmp/.estb_mac
        else RDKB
            XH->>Platform: getErouterMac()
        else RDKC
            XH->>Platform: popen("mfrApi_test 3 9")
            Platform-->>XH: "AA:BB:CC:DD:EE:FF"
        end
        Note over XH: _estb_mac_address = result
    end

    rect rgb(255, 243, 200)
        Note over XH,Platform: Common Fields (all platforms)
        XH->>Platform: GetFirmwareVersion()
        Platform-->>XH: e.g. "XHC1_2.0.1"
        XH->>Platform: GetBuildType()
        Platform-->>XH: eDEV / eVBN / ePROD
        XH->>Platform: GetModelNum()
        Platform-->>XH: e.g. "XHC1"
    end

    rect rgb(200, 230, 255)
        Note over XH,Files: Manufacturer (not RDKC)
        alt STB / RDKB
            XH->>Platform: GetMFRName()
        else RDKC
            Note over XH: SKIP — not used in camera URL
        end
    end

    rect rgb(200, 230, 255)
        Note over XH,Files: Partner ID
        alt STB / RDKB
            XH->>Platform: GetPartnerId() → /opt/partnerid
        else RDKC
            XH->>Files: fopen("/opt/usr_config/partnerid.txt")
            Files-->>XH: "Comcast"
        end
        Note over XH: _partner_id = result
    end

    XH-->>Init: return 0

    rect rgb(220, 255, 220)
        Note over Init,Files: Account ID
        alt STB / RDKB
            Init->>Init: read_RFCProperty() from TR-181 DB
        else RDKC
            Init->>Files: ifstream("/opt/usr_config/service_number.txt")
            Files-->>Init: "5789171196032993066"
        end
        Note over Init: _accountId = result
    end

    rect rgb(220, 255, 220)
        Note over Init,Files: Account Hash (RDKC only)
        Init->>Files: ifstream("/opt/usr_config/accounthash.txt")
        Files-->>Init: "1EMvt8zMMd8muKCBJRnp1z6nZTgsBJ1VhL"
        Note over Init: _accountHash = result
    end
```

---

## 4. Xconf Query & Response Processing

```mermaid
sequenceDiagram
    participant Proc as RuntimeFeatureControl<br/>Processor
    participant Hash as Hash/Time Store
    participant URL as URL Builder
    participant mTLS as mtlsUtils
    participant HTTP as libcurl
    participant Xconf as Xconf Server
    participant JSON as JSON Parser
    participant Store as Parameter Store

    Note over Proc,Store: Step 1 — Load Stored State
    Proc->>Hash: GetStoredHashAndTime()
    alt RDKC
        Hash->>Hash: Read /tmp/RFC/.hashValue
        Hash->>Hash: Read /tmp/RFC/.timeValue
        Note over Hash: Default: hash="UPGRADE_HASH", time="0"
    else Other
        Hash->>Hash: Check XconfSelector slot
        Hash->>Hash: read_RFCProperty(ConfigSetHash)
    end
    Hash-->>Proc: {hash, timestamp}

    Note over Proc,Store: Step 2 — Build Request URL
    Proc->>URL: CreateXconfHTTPUrl() [virtual]
    alt RDKC Override
        URL->>URL: GetAccountHash()
        URL->>URL: Build: server?estbMacAddress=MAC<br/>&firmwareVersion=FW&env=BUILD<br/>&model=MODEL&accountHash=HASH<br/>&partnerId=PID&accountId=AID<br/>&experience=EXP&version=2
    else Default
        URL->>URL: Build: server?estbMacAddress=MAC<br/>&firmwareVersion=FW&env=BUILD<br/>&model=MODEL&ecmMacAddress=ECM<br/>&partnerId=PID&accountId=AID<br/>&experience=EXP&manufacturer=MFR<br/>&osclass=OS&version=2
    end
    URL-->>Proc: URL string

    Note over Proc,Store: Step 3 — Secure HTTP Request
    Proc->>mTLS: getMtlscert(&sec) [RDKC]
    mTLS-->>Proc: {cert_path, password, status}
    Proc->>HTTP: ExecuteRequest(url, &mTLS_creds, &httpCode)
    HTTP->>Xconf: GET url (with mTLS cert)
    Xconf-->>HTTP: HTTP 200 + JSON body
    HTTP-->>Proc: response file path

    Note over Proc,Store: Step 4 — Parse & Apply
    Proc->>JSON: ProcessJsonResponse(response)
    JSON->>JSON: Parse features array
    JSON->>JSON: Extract parameters per feature
    JSON->>JSON: Build effectiveImmediate set [RDKC]

    loop For each feature parameter
        Proc->>Store: clearDB() → remove old params
        Proc->>Store: set_RFCProperty(name, key, value)
        alt RDKC
            Store->>Store: Read tr181store.ini
            Store->>Store: Update/append key=value line
            Store->>Store: Write tr181store.ini
        else RDKB
            Store->>Store: rbus_open → rbus_set → rbus_close
        else STB
            Store->>Store: setRFCParameter() via hostif
        end
    end

    Note over Proc,Store: Step 5 — Persist Metadata
    Proc->>Hash: updateHashInDB(new_hash)
    Proc->>Hash: updateTimeInDB(new_time)
    Proc->>Proc: StoreXconfEndpointMetadata() [no-op on RDKC]
```

---

## 5. mTLS Certificate Acquisition

```mermaid
sequenceDiagram
    participant Proc as Processor
    participant mTLS as getMtlscert()
    participant FS as File System
    participant Shell as Shell (utils.sh)

    Proc->>mTLS: getMtlscert(&sec)

    alt LIBRDKCERTSELECTOR path
        mTLS->>mTLS: rdkcertselector_new()
        mTLS->>mTLS: Iterate cert selector choices
        mTLS-->>Proc: MTLS_SUCCESS + cert details
    else RDKC path
        mTLS->>FS: access("/opt/certs/devicecert_1.pk12")
        alt Dynamic XPKI exists
            mTLS->>Shell: popen(". /lib/rdk/utils.sh && getxpkiPass")
            Shell-->>mTLS: password string
            alt Password OK
                Note over mTLS: sec.cert = "/opt/certs/devicecert_1.pk12"<br/>sec.password = password
                mTLS-->>Proc: MTLS_SUCCESS
            else Password failed
                Note over mTLS: Fall through to static
            end
        end

        mTLS->>FS: access("/etc/ssl/certs/staticXpkiCrt.pk12")
        alt Static XPKI exists
            mTLS->>Shell: popen(". /lib/rdk/utils.sh && getstaticxpkiPass")
            Shell-->>mTLS: password string
            alt Password OK
                Note over mTLS: sec.cert = "/etc/ssl/certs/staticXpkiCrt.pk12"<br/>sec.password = password
                mTLS-->>Proc: MTLS_SUCCESS
            else Password failed
                mTLS-->>Proc: MTLS_FAILURE
            end
        else No static cert
            mTLS-->>Proc: MTLS_FAILURE
        end
    end

    alt MTLS_SUCCESS
        Proc->>Proc: ExecuteRequest(&file_dwnl, &sec, &httpCode)
    else MTLS_FAILURE
        Proc->>Proc: ExecuteRequest(&file_dwnl, NULL, &httpCode)
        Note over Proc: Proceed without mTLS
    end
```

---

## 6. Parameter Application

```mermaid
sequenceDiagram
    participant Proc as Processor
    participant JSON as JSON Parser
    participant INI as tr181store.ini
    participant API as rfcapi / rbus / hostif

    Proc->>JSON: Read features[] from response
    
    loop For each feature in features[]
        JSON->>JSON: Extract feature name, configData[], effectiveImmediate
        
        loop For each param in configData[]
            JSON->>JSON: Get {key, value, dataType}
            
            Proc->>Proc: set_RFCProperty(featureName, key, value)
            
            alt RDKC Platform
                Proc->>INI: Read entire file into lines[]
                
                alt Key exists in file
                    Proc->>INI: Replace line: key=value
                else Key not found
                    Proc->>INI: Append: key=value
                end
                
                Proc->>INI: Write all lines back
            else RDKB Platform
                Proc->>API: rbus_open()
                Proc->>API: rbus_set(key, value)
                Proc->>API: rbus_close()
            else STB Platform
                Proc->>API: setRFCParameter(key, value, type)
            end
        end
    end
```

---

## 7. Reboot Evaluation & Scheduling

```mermaid
sequenceDiagram
    participant Proc as Processor
    participant JSON as Feature JSON
    participant Device as Device Properties
    participant Cron as RfcRebootCronschedule.sh
    participant MGR as RFCManager

    Note over Proc,MGR: Step 1 — Build effectiveImmediate set (RDKC)
    Proc->>JSON: Parse features[]
    loop For each feature
        alt feature.effectiveImmediate == true
            loop For each param in feature.configData
                Proc->>Proc: _effectiveImmediateParams.insert(param.key)
            end
        end
    end

    Note over Proc,MGR: Step 2 — Evaluate per-param reboot need
    loop For each applied parameter
        alt param.key in _effectiveImmediateParams
            Proc->>Device: getDevicePropertyData("RDKC_DEVICE_PROVISION_STATUS")
            Device-->>Proc: "1" (provisioned) / "0" (not provisioned)
            
            alt Device is provisioned
                alt param is NOT AccountID or MD5AccountHash
                    Proc->>Proc: _rfcRebootCronNeeded = true
                    Note over Proc: Reboot required for this parameter
                else Identity param — skip
                    Note over Proc: No reboot for identity changes
                end
            else Not provisioned
                Note over Proc: No reboot — device not provisioned
            end
        else Not in effectiveImmediate set
            Note over Proc: No reboot needed
        end
    end

    Note over Proc,MGR: Step 3 — Trigger reboot (if needed)
    Proc-->>MGR: Return from ProcessRuntimeFeatureControlReq()
    MGR->>Proc: getRfcRebootCronNeeded()
    
    alt _rfcRebootCronNeeded == true
        MGR->>Cron: v_secure_system("sh /lib/rdk/RfcRebootCronschedule.sh &")
        Note over Cron: Schedule device reboot via cron
    else No reboot needed
        Note over MGR: Continue to post-processing
    end
```

---

## 8. Post-Processing & Cleanup

```mermaid
sequenceDiagram
    participant MGR as RFCManager
    participant Proc as Processor
    participant MM as MaintenanceManager
    participant Cron as Cron System
    participant FS as File System

    Note over MGR,FS: After ProcessRuntimeFeatureControlReq() returns

    alt RDKC Platform
        MGR->>MGR: Skip MaintenanceManager notification
        MGR->>Proc: getRfcRebootCronNeeded()
        alt Reboot needed
            MGR->>FS: v_secure_system("sh /lib/rdk/RfcRebootCronschedule.sh &")
        end
    else STB Platform
        MGR->>MM: SendEventToMaintenanceManager(MAINT_RFC_COMPLETE)
        alt RFC error occurred
            MGR->>MM: SendEventToMaintenanceManager(MAINT_RFC_ERROR)
        end
        alt Reboot required
            MGR->>MM: SendEventToMaintenanceManager(MAINT_CRITICAL_UPDATE)
            MGR->>MM: SendEventToMaintenanceManager(MAINT_REBOOT_REQUIRED)
        end
    end

    Note over MGR,FS: Post-Processing
    MGR->>MGR: RFCManagerPostProcess()
    MGR->>Cron: Configure periodic cron schedule
    
    alt RDKC
        Note over Cron: Entry: schedule /usr/bin/rfcMgr
    else Other
        Note over Cron: Entry: schedule /usr/bin/rfcMgr >> /rdklogs/logs/dcmrfc.log.0 2>&1
    end

    Note over MGR,FS: Cleanup
    MGR-->>FS: Return → main()
    FS->>FS: atexit() → cleanup_lock_file()
    FS->>FS: Remove /tmp/.rfcServiceLock
```

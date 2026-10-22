/*##############################################################################
 # If not stated otherwise in this file or this component's LICENSE file the
 # following copyright and licenses apply:
 #
 # Copyright 2020 RDK Comcast
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
 ##############################################################################
 */
#include "mtlsUtils.h"
#include "rfc_common.h"
#ifdef LIBRDKCONFIG_BUILD
#include "rdkconfig.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifdef LIBRDKCERTSELECTOR
#define FILESCHEME "file://"
/* Description: Use for get all mtls related certificate and key.
 * @param sec: This is a pointer hold the certificate, key and type of certificate.
 * @return : MTLS_CERT_FETCH_SUCCESS on success, MTLS_CERT_FETCH_FAILURE on mtls cert failure , STATE_RED_CERT_FETCH_FAILURE on state red cert failure
 * */
MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel) {
    char *certUri = NULL;
    char *certPass = NULL;
    char *engine = NULL;
    char *certFile = NULL;
    rdkcertselectorStatus_t certStat = rdkcertselector_getCert(*pthisCertSel, &certUri, &certPass);
    if (certStat != certselectorOk || certUri == NULL || certPass == NULL) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "%s, Failed to retrieve certificate for MTLS\n",  __FUNCTION__);
        rdkcertselector_free(pthisCertSel);
        if(*pthisCertSel == NULL){
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "%s, Cert selector memory free\n", __FUNCTION__);
        }else{
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"%s, Cert selector memory free failed\n", __FUNCTION__);
        }
        return MTLS_CERT_FETCH_FAILURE; // Return error
    }
    certFile = certUri;
    if (strncmp(certFile, FILESCHEME, sizeof(FILESCHEME)-1) == 0) {
        certFile += (sizeof(FILESCHEME)-1); // Remove file scheme prefix
    }
    strncpy(sec->cert_name, certFile, sizeof(sec->cert_name) - 1);
    sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';
    strncpy(sec->key_pas, certPass, sizeof(sec->key_pas) - 1);
    sec->key_pas[sizeof(sec->key_pas) - 1] = '\0';
    engine = rdkcertselector_getEngine(*pthisCertSel);
    if (engine == NULL) {
        sec->engine[0] = '\0';
    }else{
        strncpy(sec->engine, engine, sizeof(sec->engine) - 1);
        sec->engine[sizeof(sec->engine) - 1] = '\0';
    }
    strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
    sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "%s, MTLS dynamic/static cert success. cert=%s, type=%s, engine=%s\n", __FUNCTION__, sec->cert_name, sec->cert_type, sec->engine);
    return MTLS_CERT_FETCH_SUCCESS; // Return success
}
#endif
#if defined(RDKC)
std::string getEstbMacAddress()
{
    std::string mac;
    char tmpbuf[200] = {0};
    FILE *fp = popen("mfrApi_test 3 9", "r");
    if (fp)
    {
        if (fgets(tmpbuf, sizeof(tmpbuf), fp))
        {
            size_t mlen = strlen(tmpbuf);
            if (mlen > 0 && tmpbuf[mlen - 1] == '\n') tmpbuf[mlen - 1] = '\0';
            if (strlen(tmpbuf) > 0)
                mac = tmpbuf;
        }
        pclose(fp);
    }
    if (mac.empty())
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s] RDKC: mfrApi_test failed to get MAC\n", __FUNCTION__);
    return mac;
}
std::string getModelNumber()
{
    std::string model;
    char tmpbuf[200] = {0};
    FILE *fp = popen("mfrApi_test 3 2", "r");
    if (fp)
    {
        if (fgets(tmpbuf, sizeof(tmpbuf), fp))
        {
            size_t mlen = strlen(tmpbuf);
            if (mlen > 0 && tmpbuf[mlen - 1] == '\n') tmpbuf[mlen - 1] = '\0';
            if (strlen(tmpbuf) > 0)
                model = tmpbuf;
        }
        pclose(fp);
    }
    if (model.empty())
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s] RDKC: mfrApi_test failed to get model number\n", __FUNCTION__);
    return model;
}
std::string getPartnerIdFromFile()
{
    std::string partnerId;
    char tmpbuf[200] = {0};
    FILE *fp = fopen("/opt/usr_config/partnerid.txt", "r");
    if (fp)
    {
        if (fgets(tmpbuf, sizeof(tmpbuf), fp))
        {
            size_t plen = strlen(tmpbuf);
            if (plen > 0 && tmpbuf[plen - 1] == '\n') tmpbuf[plen - 1] = '\0';
            partnerId = tmpbuf;
        }
        fclose(fp);
    }
    if (partnerId.empty())
        partnerId = "Unknown";
    return partnerId;
}
std::string getAccountHashFromFile()
{
    std::string accountHash;
    std::ifstream ifs("/opt/usr_config/accounthash.txt");
    if (ifs.is_open())
    {
        std::getline(ifs, accountHash);
        ifs.close();
    }
    if (accountHash.empty())
        accountHash = "Unknown";
    return accountHash;
}
bool isDeviceProvisioned()
{
    /* Mirrors shell checkCameraProvisionStatus():
     * 1. If live_video.conf exists and is non-empty, check "enabled=1"
     * 2. Else if wpa_supplicant.conf backup exists and is non-empty, provisioned
     * 3. Else not provisioned
     * Check secure partition path first (matches setConfigFilesPath logic). */
    const char* SECURE_EVO_FILE = "/opt/SecurePartition/usr_config/live_video.conf";
    const char* EVO_FILE = "/opt/usr_config/live_video.conf";
    const char* WPA_CONFIG_BKP_ENV_PATH = "/mnt/ramdisk/env/wpa_supplicant.conf";
    struct stat st;
    const char* evoPath = nullptr;
    // Determine which provisioning file to use (secure partition preferred)
    if (stat(SECURE_EVO_FILE, &st) == 0 && st.st_size > 0)
    {
        evoPath = SECURE_EVO_FILE;
    }
    else if (stat(EVO_FILE, &st) == 0 && st.st_size > 0)
    {
        evoPath = EVO_FILE;
    }
    if (evoPath)
    {
        std::ifstream file(evoPath);
        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                // Match shell: grep -w "enabled" | cut -d "=" -f2
                if (line.find("enabled") != std::string::npos)
                {
                    size_t pos = line.find('=');
                    if (pos != std::string::npos)
                    {
                        std::string value = line.substr(pos + 1);
                        value.erase(0, value.find_first_not_of(" \t\r\n"));
                        value.erase(value.find_last_not_of(" \t\r\n") + 1);
                        return (value == "1");
                    }
                }
            }
        }
        return false;
    }
    // Fallback: device is provisioned if WPA config backup exists and is non-empty
    else if (stat(WPA_CONFIG_BKP_ENV_PATH, &st) == 0 && st.st_size > 0)
    {
        return true;
    }
    return false;
}
#endif /* RDKC */
#if defined(RDKB_SUPPORT)
std::string getWanInterfaceName()
{
    std::string interfaceName;
    // Execute sysevent get current_wan_ifname
    FILE* pipe = popen("sysevent get current_wan_ifname", "r");
    if (pipe) {
        char buffer[128] = {0};
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            interfaceName = buffer;
            // Remove trailing whitespace/newlines
            while (!interfaceName.empty() && (interfaceName.back() == '\n' || interfaceName.back() == ' ')) {
                interfaceName.pop_back();
            }
        }
        int exitStatus = pclose(pipe);
        // Check if sysevent failed (non-zero exit status) or returned empty string
        if (exitStatus != 0 || interfaceName.empty()) {
            interfaceName = "erouter0";
        }
    } else {
        // If popen failed, default to erouter0
        interfaceName = "erouter0";
    }
    return interfaceName;
}
std::string getWanMacInterfaceName()
{
    std::string macInterface;
    // Check if rdkb_extender is true
    const char* rdkbExtender = getenv("rdkb_extender");
    if (rdkbExtender && std::string(rdkbExtender) == "true") {
        macInterface = "eth0";
    } else {
        // Get wan_physical_ifname from syscfg
        FILE* pipe = popen("syscfg get wan_physical_ifname", "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                macInterface = buffer;
                // Remove trailing whitespace/newlines
                while (!macInterface.empty() && (macInterface.back() == '\n' || macInterface.back() == ' ')) {
                    macInterface.pop_back();
                }
            }
            pclose(pipe);
        }
        // Default to erouter0 if empty
        if (macInterface.empty()) {
            macInterface = "erouter0";
        }
     }
    return macInterface;
}
std::string getBoxTypeFromFile()
{
    std::string boxType;
    std::ifstream file("/etc/device.properties");
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find("BOX_TYPE=") == 0) {
                size_t equalPos = line.find('=');
                if (equalPos != std::string::npos && equalPos + 1 < line.length()) {
                    boxType = line.substr(equalPos + 1);
                    // Trim whitespace
                    boxType.erase(0, boxType.find_first_not_of(" \t\r\n"));
                    boxType.erase(boxType.find_last_not_of(" \t\r\n") + 1);
                }
                break;
            }
        }
        file.close();
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s] Failed to open /etc/device.properties\n", __FUNCTION__);
    }
    return boxType;
}
std::string getErouterMac()
{
    std::string erouterMac;
    std::string boxType = getBoxTypeFromFile();
    // Fallback to environment variable if file reading fails
    if (boxType.empty()) {
        const char* envBoxType = getenv("BOX_TYPE");
        if (envBoxType) boxType = envBoxType;
    }
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s] Recevied BOX_TYPE %s\n",__FUNCTION__,boxType.c_str());
    // Get wan_interface using getWanMacInterfaceName
    std::string wanInterface = getWanMacInterfaceName();
    // Check if BOX_TYPE is one of the specific types that use the new logic
    if (boxType == "HUB4" || boxType == "SR300" || boxType == "SR213" ||
        boxType == "SE501" || boxType == "WNXL11BWL" || boxType == "SCER11BEL" || boxType == "SCXF11BFL") {
        // FEATURE_RDKB_WAN_MANAGER Get WANINTERFACE (
        std::string wanInterfaceName = getWanInterfaceName();
        std::string cmd = std::string("cat /sys/class/net/") + wanInterfaceName + "/address | tr '[a-f]' '[A-F]'";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                erouterMac = buffer;
                while (!erouterMac.empty() && (erouterMac.back() == '\n' || erouterMac.back() == ' ')) {
                    erouterMac.pop_back();
                }
            }
            pclose(pipe);
        }
        // If empty, fallback to sysevent get eth_wan_mac
        if (erouterMac.empty()) {
            pipe = popen("sysevent get eth_wan_mac | tr '[a-f]' '[A-F]'", "r");
            if (pipe) {
                char buffer[128] = {0};
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    erouterMac = buffer;
                    while (!erouterMac.empty() && (erouterMac.back() == '\n' || erouterMac.back() == ' ')) {
                        erouterMac.pop_back();
                    }
                }
                pclose(pipe);
            }
        }
    } else {
        // Use ifconfig with wan_interface for other BOX_TYPE values
        std::string cmd = std::string("ifconfig ") + wanInterface + " | grep HWaddr | cut -d \" \" -f7";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                erouterMac = buffer;
                while (!erouterMac.empty() && (erouterMac.back() == '\n' || erouterMac.back() == ' ')) {
                    erouterMac.pop_back();
                }
            }
            pclose(pipe);
        }
    }
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s] Recevied eRouterMac(eSTBMac): %s\n", __FUNCTION__,erouterMac.c_str());
    return erouterMac;
}
std::string geteCMMac()
{
    std::string macAddress;
    std::string boxType = getBoxTypeFromFile();
    if (boxType.empty()) {
        const char* envBoxType = getenv("BOX_TYPE");
        if (envBoxType) boxType = envBoxType;
    }
    // Compute CMINTERFACE based on BOX_TYPE and WAN0_IS_DUMMY
    std::string cmInterface;
    if (boxType == "XF3") {
        cmInterface = "erouter0";
    } else {
        const char* wan0IsDummy = getenv("WAN0_IS_DUMMY");
        if (wan0IsDummy && std::string(wan0IsDummy) == "true") {
            cmInterface = "privbr";
        } else {
            cmInterface = "wan0";
        }
    }
    std::string wanInterface = getWanInterfaceName();
    std::string cmd;
    if (boxType == "XF3") {
        cmd = "dmcli eRT retv Device.DPoE.Mac_address";
    } else if (boxType == "XB6" || boxType == "TCCBR") {
        cmd = "dmcli eRT retv Device.X_CISCO_COM_CableModem.MACAddress";
    } else if (boxType == "VNTXER5" || boxType == "SCER11BEL" || boxType == "SCXF11BFL") {
        cmd = "dmcli eRT retv Device.DeviceInfo.X_COMCAST-COM_CM_MAC";
    } else if (boxType == "HUB4" || boxType == "SR300" || boxType == "SR213" || boxType == "SE501" || boxType == "WNXL11BWL") {
        cmd = std::string("cat /sys/class/net/") + wanInterface + "/address | tr '[a-f]' '[A-F]'";
    } else {
        cmd = std::string("ifconfig ") + cmInterface + " | grep HWaddr | awk '{print $NF}'";
    }
   if (!cmd.empty()) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                macAddress = buffer;
                while (!macAddress.empty() && (macAddress.back() == '\n' || macAddress.back() == ' ')) {
                    macAddress.pop_back();
                }
            }
            pclose(pipe);
        }
    }
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s] Received eCM MAC: %s for BOX_TYPE: %s\n", __FUNCTION__, macAddress.c_str(), boxType.c_str());
    return macAddress;
}
#endif
#ifdef __cplusplus
}
#endif
#if defined(RDKC)
bool shouldScheduleCameraReboot(const std::string &key,
                                const std::set<std::string> &effectiveImmediateParams,
                                const std::string &currentValue,
                                const std::string &newValue)
{
    if (effectiveImmediateParams.count(key) == 0)
        return false;
    if (!isDeviceProvisioned())
        return false;
    static const std::string accountIdKey =
        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    static const std::string accountHashKey =
        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MD5AccountHash";
    if (key == accountIdKey || key == accountHashKey)
    {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
                "%s: RDKC: Skip scheduling reboot for Account Id/Hash change\n", __FUNCTION__);
        return false;
    }
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
            "%s: RDKC: Enabling RfcRebootCronNeeded for %s old=%s new=%s\n",
            __FUNCTION__, key.c_str(), currentValue.c_str(), newValue.c_str());
    return true;
}
#endif /* RDKC */

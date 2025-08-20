/*##############################################################################
 # If not stated otherwise in this file or this component's LICENSE file the
 # following copyright and licenses apply:
 #
 # Copyright 2024 RDK Management
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


#include "rfc_xconf_handler.h"
#include "rfc_common.h"
#include "rfcapi.h"
#include "rfc_mgr_json.h"
#include "mtlsUtils.h"
#include <sys/stat.h>
#include <sys/types.h>
#if defined(RDKB_SUPPORT)
#include <rbus/rbus.h>
#include <rbus/rbus_value.h>
#endif
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif
#include <json_parse.h>
#include <rdk_fwdl_utils.h>
#include <downloadUtil.h>
#include <common_device_api.h>
#include <system_utils.h>
#include <urlHelper.h>	
    
int RuntimeFeatureControlProcessor:: InitializeRuntimeFeatureControlProcessor(void)
{
     std::string rfc_file;
     bool dbgServices = isDebugServicesEnabled();
	
    int rc = GetBootstrapXconfUrl(_boot_strap_xconf_url);
    if(rc != 0)
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get XCONF_BS_URL from Bootstrap config.\n", __FUNCTION__, __LINE__);

     if(0 != initializeXconfHandler())
     {
	return FAILURE;
     }
    
    if((filePresentCheck(RFC_PROPERTIES_PERSISTENCE_FILE) == RDK_API_SUCCESS) && (_ebuild_type != ePROD || dbgServices == true))
    {
	rfc_file = RFC_PROPERTIES_PERSISTENCE_FILE;
	rfc_state = Local;
    }
    else
    {
        rfc_file = RFC_PROPERTIES_FILE;
	rfc_state = Init;
    }
    
	/* Get the RFC Parameters */
    if(SUCCESS != GetServURL(rfc_file.c_str()))
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Xconf Initialization Failed for Xconf Server URL\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }

    /* get experience */ 
    if(-1 == GetExperience())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Xconf Initialization Failed for Experience\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }
	
	/* last Firmware Version */
    if(SUCCESS != GetLastProcessedFirmware(RFC_LAST_VERSION))
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Xconf Initialization Failed for Last firmware\n", __FUNCTION__, __LINE__);
    }

    GetAccountID();
    GetRFCPartnerID();
    GetOsClass();

#if !defined(RDKB_SUPPORT)    
    _is_first_request = IsNewFirmwareFirstRequest();
#endif

    return SUCCESS;
}



bool RuntimeFeatureControlProcessor::checkWhoamiSupport()
{
    char value[8] = {0};
    int ret = getDevicePropertyData("WHOAMI_SUPPORT", value, sizeof(value));
    if (ret != 1) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get WHOAMI_SUPPORT property. Status: %d\n", __FUNCTION__, __LINE__, ret);
        return false;
    }
    bool enabled = (strcasecmp(value, "true") == 0);
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Whoami support is %s\n", __FUNCTION__, __LINE__, enabled ? "ENABLED" : "DISABLED");
    return enabled;
}

bool RuntimeFeatureControlProcessor::isDebugServicesEnabled(void)
{
    bool result = false;
    int ret = -1;
    char rfc_data[RFC_VALUE_BUF_SIZE];

    *rfc_data = 0;
    ret = read_RFCProperty("DEBUGSRV", RFC_DEBUGSRV, rfc_data, sizeof(rfc_data));
    if (ret == -1) {
        SWLOG_ERROR("%s: rfc Debug services =%s failed Status %d\n", __FUNCTION__, RFC_DEBUGSRV, ret);
    } else {
        SWLOG_INFO("%s: rfc Debug services = %s\n", __FUNCTION__, rfc_data);
        if (strncmp(rfc_data, "true", sizeof(rfc_data)+1 ) == 0) {
            result = true;
        }
    }
    return result;
}


bool RuntimeFeatureControlProcessor::IsNewFirmwareFirstRequest(void)
{
    bool result = false;
    /* Get Firmware Version */
    if((_last_firmware.empty()) || (!_firmware_version.empty()  && ( _last_firmware.compare( _firmware_version) != 0)))
    {
        result = true;        
    }
    return result;
}

int RuntimeFeatureControlProcessor::GetLastProcessedFirmware(const char *lastVesrionFile) 
{
    std::ifstream inputFile;

    inputFile.open(lastVesrionFile);
    if (!inputFile.is_open()) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to open file.\n" );
        return FAILURE;
    }

    std::getline(inputFile, _last_firmware);

    if ( _last_firmware.empty()) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Last Firmware not found in the file.\n" );
        return FAILURE;
    }
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] GetLastProcessedFirmware: [%s]\n", __FUNCTION__, __LINE__, _last_firmware.c_str());

    inputFile.close();

    return SUCCESS;

}

#if defined(RDKB_SUPPORT)
bool RuntimeFeatureControlProcessor::ExecuteCommand(const std::string& command, std::string& output)
{
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }

    char buffer[128];
    output = "";

    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            output += buffer;
        }
    }

    int status = pclose(pipe);
    return (status == 0);
}

bool RuntimeFeatureControlProcessor::ParseConfigValue(const std::string& configKey, const std::string& configValue, int rebootValue, bool& rfcRebootCronNeeded)
{
    std::string paramName;
    size_t tr181Pos = configKey.find("tr181");

    if (tr181Pos != std::string::npos) {
        size_t dotPos = configKey.find(".", tr181Pos);
        if (dotPos != std::string::npos) {
            paramName = configKey.substr(dotPos + 1);
        }
    }

    if (paramName.empty()) {
        return false;
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Parameter name %s\n", paramName.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Parameter value %s\n", configValue.c_str());

    // Initialize rbus connection
    rbusHandle_t rbusHandle;
    rbusError_t rc = rbus_open(&rbusHandle, "RFC_Manager");
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to open rbus handle: %s\n", rbusError_ToString(rc));
        return false;
    }

    // Check for WanFailOverSupportEnable
    std::string wanFailOverSupportEnable;
    rbusValue_t wfoValue = NULL;
    rc = rbus_get(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.WanFailOverSupport.Enable", &wfoValue);
    if (rc == RBUS_ERROR_SUCCESS && wfoValue != NULL) {
        if (rbusValue_GetType(wfoValue) == RBUS_BOOLEAN) {
            wanFailOverSupportEnable = rbusValue_GetBoolean(wfoValue) ? "true" : "false";
        } else if (rbusValue_GetType(wfoValue) == RBUS_STRING) {
            wanFailOverSupportEnable = rbusValue_GetString(wfoValue, NULL);
        }
        rbusValue_Release(wfoValue);
    }

    if (wanFailOverSupportEnable == "true") {
        if (paramName == "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.UPnP.Refactor.Enable" ||
            paramName == "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Xupnp") {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Parameter %s is not applied in WFO builds\n", paramName.c_str());
            rbus_close(rbusHandle);
            return true;
        }
    }

    // Check for WanUnificationEnable
    std::string wanUnificationEnable;
    rbusValue_t wuValue = NULL;
    rc = rbus_get(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.WanUnification.Enable", &wuValue);
    if (rc == RBUS_ERROR_SUCCESS && wuValue != NULL) {
        if (rbusValue_GetType(wuValue) == RBUS_BOOLEAN) {
            wanUnificationEnable = rbusValue_GetBoolean(wuValue) ? "true" : "false";
        } else if (rbusValue_GetType(wuValue) == RBUS_STRING) {
            wanUnificationEnable = rbusValue_GetString(wuValue, NULL);
        }
        rbusValue_Release(wuValue);
    }

    if (wanUnificationEnable == "true" &&
        paramName == "Device.X_RDKCENTRAL-COM_EthernetWAN.SelectedOperationalMode") {

        std::string enableDocsis = "true";
        std::string enableEth = "true";
        std::string persistSelectedInterface = "TRUE";

        if (configValue == "DOCSIS") {
            enableEth = "false";
        } else if (configValue == "Ethernet") {
            enableDocsis = "false";
        } else if (configValue == "Auto") {
            persistSelectedInterface = "FALSE";
        }

        // Set DOCSIS interface using rbus
        rbusValue_t docsisValue;
        rbusValue_Init(&docsisValue);
        rbusValue_SetBoolean(docsisValue, (enableDocsis == "true"));
        rc = rbus_set(rbusHandle, "Device.X_RDK_WanManager.Interface.[DOCSIS].Selection.Enable", docsisValue, NULL);
        if (rc != RBUS_ERROR_SUCCESS) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set DOCSIS interface: %s\n", rbusError_ToString(rc));
        }
        rbusValue_Release(docsisValue);

        // Set WAN over Ethernet interface using rbus
        rbusValue_t ethValue;
        rbusValue_Init(&ethValue);
        rbusValue_SetBoolean(ethValue, (enableEth == "true"));
        rc = rbus_set(rbusHandle, "Device.X_RDK_WanManager.Interface.[WANOE].Selection.Enable", ethValue, NULL);
        if (rc != RBUS_ERROR_SUCCESS) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set WANOE interface: %s\n", rbusError_ToString(rc));
        }
        rbusValue_Release(ethValue);

        // Set PersistSelectedInterface using psmcli (keep this as is since it's using PSM)
        std::string psmcliCmd = "psmcli set \"dmsb.wanmanager.group.1.PersistSelectedInterface\" \"" + persistSelectedInterface + "\"";
        std::string output;
        ExecuteCommand(psmcliCmd, output);

        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "WanUnification Enabled Setting WanManager DML instead of Device.X_RDKCENTRAL-COM_EthernetWAN.SelectedOperationalMode to %s\n", configValue.c_str());
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Setting Device.X_RDK_WanManager.Interface.[DOCSIS].Selection.Enable to %s\n", enableDocsis.c_str());
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Setting Device.X_RDK_WanManager.Interface.[WANOE].Selection.Enable %s\n", enableEth.c_str());
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Setting dmsb.wanmanager.group.1.PersistSelectedInterface %s\n", persistSelectedInterface.c_str());

        rbus_close(rbusHandle);
        return true;
    }

    // Get parameter type and current value using rbus
    rbusValue_t paramVal = NULL;
    rc = rbus_get(rbusHandle, paramName.c_str(), &paramVal);
    if (rc != RBUS_ERROR_SUCCESS || paramVal == NULL) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "rbus_get failed for %s: %s\n", paramName.c_str(), rbusError_ToString(rc));
        rbus_close(rbusHandle);
        return true;
    }

    rbusValueType_t paramType = rbusValue_GetType(paramVal);
    std::string paramTypeStr;
    std::string paramValue;

    // Extract parameter value based on type
    switch (paramType) {
        case RBUS_STRING:
            paramTypeStr = "string";
            paramValue = rbusValue_GetString(paramVal, NULL);
            break;
        case RBUS_BOOLEAN:
            paramTypeStr = "bool";
            paramValue = rbusValue_GetBoolean(paramVal) ? "true" : "false";
            break;
        case RBUS_INT32:
            paramTypeStr = "int";
            paramValue = std::to_string(rbusValue_GetInt32(paramVal));
            break;
        case RBUS_UINT32:
            paramTypeStr = "uint";
            paramValue = std::to_string(rbusValue_GetUInt32(paramVal));
            break;
        case RBUS_SINGLE:
        case RBUS_DOUBLE:
            paramTypeStr = "double";
            paramValue = std::to_string(rbusValue_GetDouble(paramVal));
            break;
        default:
            paramTypeStr = "";
            paramValue = "";
            break;
    }

    rbusValue_Release(paramVal);

    if (paramTypeStr.empty()) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "rbus_get failed for %s - invalid or unsupported parameter type\n", paramName.c_str());
        rbus_close(rbusHandle);
        return true;
    }

    std::ofstream paramFile("/tmp/.paramRFC");
    if (paramFile.is_open()) {
        paramFile << "name: " <<  paramName << "\n";
        paramFile << "type: " << paramTypeStr << "\n";
        paramFile << "value: " << paramValue << "\n";
        paramFile.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Parameter info written to /tmp/.paramRFC\n");
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to write to /tmp/.paramRFC\n");
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "paramType is %s\n", paramTypeStr.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "RFC: old parameter value %s\n", paramValue.c_str());

    bool isRfcNameSpace = (paramName.find(".X_RDKCENTRAL-COM_RFC.") != std::string::npos);

    if (paramValue != configValue || isRfcNameSpace) {
        rbusValue_t newValue;
        rbusValue_Init(&newValue);

        bool setSuccess = true;
        try {
            switch (paramType) {
                case RBUS_STRING:
                    rbusValue_SetString(newValue, configValue.c_str());
                    break;
                case RBUS_BOOLEAN:
                    rbusValue_SetBoolean(newValue, (configValue == "true" || configValue == "1"));
                    break;
                case RBUS_INT32:
                    rbusValue_SetInt32(newValue, std::stoi(configValue));
                    break;
                case RBUS_UINT32:
                    rbusValue_SetUInt32(newValue, std::stoul(configValue));
                    break;
                case RBUS_SINGLE:
                    rbusValue_SetSingle(newValue, std::stof(configValue));
                    break;
                case RBUS_DOUBLE:
                    rbusValue_SetDouble(newValue, std::stod(configValue));
                    break;
                default:
                    setSuccess = false;
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Unsupported parameter type: %d\n", paramType);
                    break;
            }
        } catch (const std::exception& e) {
            setSuccess = false;
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to convert value %s for parameter %s: %s\n",
                    configValue.c_str(), paramName.c_str(), e.what());
        }

        if (setSuccess) {
            rc = rbus_set(rbusHandle, paramName.c_str(), newValue, NULL);
            rbusValue_Release(newValue);

            if (rc == RBUS_ERROR_SUCCESS) {
                if (isRfcNameSpace && paramValue == configValue) {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "RFC: rbus SET called for RFC namespace param: %s value=%s\n",
                           paramName.c_str(), configValue.c_str());
                } else {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "RFC: updated for %s from value old=%s, to new=%s\n",
                           paramName.c_str(), paramValue.c_str(), configValue.c_str());
                }

                // Special handling for account ID
                if (paramName == RFC_ACCOUNT_ID_KEY_STR) {
                    std::string cmd = "t2CountNotify \"SYST_INFO_ACCID_set\"";
                    std::string notifyOutput;
                    ExecuteCommand(cmd, notifyOutput);
                }

                // Special handling for Syndication.PartnerId
                if (paramName == "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId") {
                    if (paramValue == "unknown" || paramValue == "Unknown") {
                        rebootValue = 1;
                    }
                }

                if (rebootValue == 1) {
                    if (!rfcRebootCronNeeded) {
                        rfcRebootCronNeeded = true;
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "RFC: Enabling RfcRebootCronNeeded since %s old value=%s, new value=%s, RebootValue=%d\n",
                               paramName.c_str(), paramValue.c_str(), configValue.c_str(), rebootValue);
                    }
                }
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "RFC: rbus SET failed for %s with value %s: %s\n",
                       paramName.c_str(), configValue.c_str(), rbusError_ToString(rc));
            }
        } else {
            rbusValue_Release(newValue);
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "RFC: Failed to set value for %s due to conversion error\n", paramName.c_str());
        }
    } else {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "RFC: For param %s new and old values are same value %s\n",
               paramName.c_str(), configValue.c_str());
    }

    rbus_close(rbusHandle);
    return true;
}

int RuntimeFeatureControlProcessor::ProcessJsonResponseB(char* featureXConfMsg)
{
    std::string rfcList;
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Xconf Response: %s\n", __FUNCTION__, __LINE__, featureXConfMsg);

    // Initialize rbus connection
    rbusHandle_t rbusHandle;
    rbusError_t rc = rbus_open(&rbusHandle, "RFC_Manager");
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to open rbus handle: %s\n", rbusError_ToString(rc));
        return FAILURE;
    }

    // Set initial RFC features
    rbusValue_t falseVal;
    rbusValue_Init(&falseVal);
    rbusValue_SetBoolean(falseVal, false);

    rc = rbus_set(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.CodebigSupport", falseVal, NULL);
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set CodebigSupport: %s\n", rbusError_ToString(rc));
    }

    rc = rbus_set(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ContainerSupport", falseVal, NULL);
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set ContainerSupport: %s\n", rbusError_ToString(rc));
    }

    // Set ClearDB and ClearDBEnd to true
    rbusValue_t trueVal;
    rbusValue_Init(&trueVal);
    rbusValue_SetBoolean(trueVal, true);

    rc = rbus_set(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ClearDB", trueVal, NULL);
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set ClearDB: %s\n", rbusError_ToString(rc));
    }

    // Process JSON data directly
    JSON *pJson = ParseJsonStr(featureXConfMsg);
    if (!pJson) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to parse JSON response\n");
        rbusValue_Release(falseVal);
        rbusValue_Release(trueVal);
        rbus_close(rbusHandle);
        return FAILURE;
    }

    bool rfcRebootCronNeeded = false;

    // Process features from the JSON
    JSON *features = GetRuntimeFeatureControlJSON(pJson);
    JSON *bkp_features = features;

    if (features) {
        int numFeatures = GetJsonArraySize(features);
        for (int index = 0; index < numFeatures; index++) {
            JSON* feature = GetJsonArrayItem(features, index);
            if (feature) {
                RuntimeFeatureControlObject *rfcObj = new RuntimeFeatureControlObject;

                if (SUCCESS != getRFCName(feature, rfcObj)) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Name\n", __FUNCTION__, __LINE__);
                    delete rfcObj;
                    continue;
                }

                if (SUCCESS != getRFCEnableParam(feature, rfcObj)) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Enable Param\n", __FUNCTION__, __LINE__);
                    delete rfcObj;
                    continue;
                }

                if (SUCCESS != getEffectiveImmediateParam(feature, rfcObj)) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Effective Immediate\n", __FUNCTION__, __LINE__);
                    delete rfcObj;
                    continue;
                }

                if (SUCCESS != getFeatureInstance(feature, rfcObj)) {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC Feature Instance not configured\n", __FUNCTION__, __LINE__);
                } else {
                    std::string filename = ".RFC_" + rfcObj->name + ".ini";
                    rfcList += rfcObj->featureInstance + "=true,";
                    writeRemoteFeatureCntrlFile(filename, rfcObj);
                }

                writeRemoteFeatureCntrlFile(VARFILE, rfcObj);

                // Special telemetry handling
                if (rfcObj->name == "PeriodicFWCheck" && rfcObj->enable) {
                    std::string notifyCmd = "t2CountNotify \"SYS_INFO_RFC_PeriodicFWCheck\"";
                    std::string output;
                    ExecuteCommand(notifyCmd, output);
                } else if (rfcObj->name == "IPv6onLnF" && rfcObj->enable) {
                    std::string notifyCmd = "t2CountNotify \"INFO_IPv6_LNF_Support\"";
                    std::string output;
                    ExecuteCommand(notifyCmd, output);
                }

                // Apply configuration if needed
                std::string key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature." + rfcObj->name + ".Enable";
                std::string value = rfcObj->enable ? "true" : "false";
                int effectiveImmediate = rfcObj->effectiveImmediate ? 1 : 0;

                ParseConfigValue(key, value, effectiveImmediate, rfcRebootCronNeeded);

                delete rfcObj;
            }
        }
    }

    rc = rbus_set(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ClearDBEnd", trueVal, NULL);
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to set ClearDBEnd: %s\n", rbusError_ToString(rc));
    }

    processXconfResponseConfigDataPart(bkp_features);

    rfcCheckAccountId();

    WriteFile(FEATURE_FILE_LIST, rfcList);

    // Notify telemetry about remote features
    NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "STAGING");

    // Update Version File
    WriteFile(".version", _firmware_version);

    HandleScheduledReboot(rfcRebootCronNeeded);
    FreeJson(pJson);
    rbusValue_Release(trueVal);
    rbus_close(rbusHandle);
    return SUCCESS;
}

void RuntimeFeatureControlProcessor::HandleScheduledReboot(bool rfcRebootCronNeeded)
{
    if (rfcRebootCronNeeded) {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "RFC: RfcRebootCronNeeded=true. Calling script to schedule reboot in maintenance window\n");

        // Execute the reboot cron schedule script
        std::string cmd = "sh /etc/RfcRebootCronschedule.sh &";
        std::string output;
        ExecuteCommand(cmd, output);
    }
}

void RuntimeFeatureControlProcessor::saveAccountIdToFile(const std::string& accountId, const std::string& paramName, const std::string& paramType)
{
    std::ofstream paramFile("/tmp/.paramRFC");
    if (paramFile.is_open()) {
        paramFile << "name: " << paramName << ",\n";
        paramFile << "type: " << paramType << ",\n";
        paramFile << "value: " << accountId << "\n";
        paramFile.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "saveAccountIdToFile: Parameter info written to /tmp/.paramRFC\n");
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "saveAccountIdToFile: Failed to write to /tmp/.paramRFC\n");
    }
}

std::string RuntimeFeatureControlProcessor::readAccountIdFromFile()
{
    std::string accountId;

    FILE* paramFile = fopen("/tmp/.paramRFC", "r");
    if (paramFile != nullptr)
    {
        char line[1024];
        while (fgets(line, sizeof(line), paramFile) != nullptr)
        {
            std::string lineStr(line);
            size_t valuePos = lineStr.find("value:");
            if (valuePos != std::string::npos)
            {
                std::string valueStr = lineStr.substr(valuePos + 6);
                size_t start = valueStr.find_first_not_of(" \t\n\r");
                if (start != std::string::npos)
                {
                    size_t end = valueStr.find_last_not_of(" \t\n\r");
                    accountId = valueStr.substr(start, end - start + 1);
                }
                break;
            }
        }
        fclose(paramFile);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "readAccountIdFromFile: Extracted AccountId=%s from file\n", accountId.c_str());
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "readAccountIdFromFile: Failed to open /tmp/.paramRFC\n");
    }

    return accountId;
}

void RuntimeFeatureControlProcessor::rfcCheckAccountId()
{
    int i = 0;
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    std::string str = "AccountID";
    std::string paramValue;

    std::string bkAccountId = readAccountIdFromFile();

    i = read_RFCProperty(str.c_str(), RFC_ACCOUNT_ID_KEY_STR, tempbuf, szBufSize);

    if (i == READ_RFC_FAILURE)
    {
        paramValue = "Unknown";
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "rfcCheckAccountId: read_RFCProperty() failed\n");
    }
    else
    {
        paramValue = tempbuf;
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "rfcCheckAccountId: read_RFCProperty() success, value=%s\n", paramValue.c_str());
    }

    saveAccountIdToFile(paramValue, RFC_ACCOUNT_ID_KEY_STR, "string");

    // Compare with backup account ID
    if (paramValue != bkAccountId)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "rfcCheckAccountId: Account Id mismatch: old=%s, new=%s\n", bkAccountId.c_str(), paramValue.c_str());
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "rfcCheckAccountId: bkAccountId=%s, paramValue=%s\n", bkAccountId.c_str(), paramValue.c_str());
}

#endif

void RuntimeFeatureControlProcessor::GetAccountID() 
{
    int i = 0;
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    std::string str = "AccountID";

    i = read_RFCProperty(str.c_str(), RFC_ACCOUNT_ID_KEY_STR, tempbuf, szBufSize);
    if (i == READ_RFC_FAILURE) 
    {
        i = snprintf(tempbuf, szBufSize, "Unknown");
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetAccountID: read_RFCProperty() failed Status %d\n", i);
    } 
    else 
    {
        i = strnlen(tempbuf, szBufSize);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetAccountID: AccountID = %s\n", tempbuf);
        _accountId = tempbuf;
#ifdef RDKB_SUPPORT	
        if (access("/tmp/.timeValue", F_OK) != 0)
        {
            // Time file doesn't exist, set AccountID to Unknown
            _accountId = "Unknown";
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetAccountID: .timeValue file not found, setting AccountID to Unknown\n");
        }
        saveAccountIdToFile(_accountId, RFC_ACCOUNT_ID_KEY_STR, "string");
#endif	
        if((_accountId.empty()) || (_last_firmware.compare( _firmware_version) != 0))
        {
            _accountId="Unknown";
        }
    }

    return;
}

void RuntimeFeatureControlProcessor::GetRFCPartnerID()
{
    int i = 0;
    char tempbuf[1024] = {0};
    int partBufSize = sizeof(tempbuf);
    std::string str = "PartnerName";

    if (checkWhoamiSupport())
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetPartnerID: WhoAmI support is Enabled\n");
         i = read_RFCProperty(str.c_str(), RFC_PARTNERNAME_KEY_STR, tempbuf, partBufSize);
         if (i == READ_RFC_FAILURE)
         {
             RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetRFCPartnerID: read_RFCProperty() failed Status %d\n", i);
         }
        else
         {
             RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetRFCPartnerID: read_RFCProperty() success\n");
             _partnerId = tempbuf;
        }
    }
    else
    {
        _partnerId = _partner_id;
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetRFCPartnerID: get from _partner_id\n");
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetRFCPartnerID: PartnerID = %s\n", _partnerId.c_str());

    return;
}

bool RuntimeFeatureControlProcessor::isMaintenanceEnabled()
{
    const std::string prop_file = DEVICE_PROPERTIES_FILE;
    std::ifstream devicepropFile;
    bool found = false;

    devicepropFile.open(prop_file.c_str());
    if (!devicepropFile.is_open())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d]Failed to open file.\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }
    std::string line;

    while (std::getline(devicepropFile, line))
    {
        if (line == "ENABLE_MAINTENANCE=true")
        {
            found = true;
            break;
        }
    }
    devicepropFile.close();

    return found;
}

void RuntimeFeatureControlProcessor::GetOsClass( void )
{

    int i = 0;
    char osclassbuf[100] = {0};
    int osclassbufSize = sizeof(osclassbuf);
    std::string str = "OsClass";
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetOsClass: Enter\n");

    if (checkWhoamiSupport())
    {
         RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetOsClass: WhoAmI support if Enabled\n");
         i = read_RFCProperty(str.c_str(), RFC_OSCLASS_KEY_STR, osclassbuf, osclassbufSize);
         if (i == READ_RFC_FAILURE)
         {
             RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "GetOsClass: read_RFCProperty() failed Status %d\n", i);
         }
         else
         {
             RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetOsClass: read_RFCProperty() success\n");
             _osclass = osclassbuf;
         }
    }
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "GetOsClass: Exit\n");
}

int RuntimeFeatureControlProcessor::GetExperience( void )
{
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    DownloadData DwnLoc;
    JSON *pJson;
    JSON *pItem;
    int i = -1;
    char result_str[] = "result";
    char experience_str[] = "experience";

    char post_data[] = "{\"jsonrpc\":\"2.0\",\"id\":\"3\",\"method\":\"org.rdk.AuthService.getExperience\", \"params\":{}}";

    if( allocDowndLoadDataMem( &DwnLoc, DEFAULT_DL_ALLOC ) == 0 )
    {
        getJsonRpc( post_data, &DwnLoc );
        pJson = ParseJsonStr( (char *)DwnLoc.pvOut );
        if( pJson != NULL )
        {
            pItem = GetJsonItem( pJson, result_str );
            if( pItem != NULL )
            {
                i = GetJsonVal( pItem, experience_str , tempbuf, szBufSize );
            }
            FreeJson( pJson );
        }
        if( DwnLoc.pvOut != NULL )
        {
            free( DwnLoc.pvOut );
        }
    }
    if( !*tempbuf )  // we got nothing back, "X1" is default
    {
        *tempbuf = 'X';
        *(tempbuf + 1) = '1';
        *(tempbuf + 2) = 0;
        i=3;
    }
    _experience = tempbuf;

    return i;
}

int RuntimeFeatureControlProcessor::GetServURL(const char *rfcPropertiesFile)
{
    const std::string m_file = rfcPropertiesFile;
    std::ifstream inputFile(m_file.c_str());
    if (!inputFile.is_open())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open file.\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }

#ifdef RDKB_SUPPORT
    _xconf_server_url.clear();
    std::string _xconf_server_url_eu;

    std::string line;
    while (std::getline(inputFile, line))
    {
        if (line.find("RFC_CONFIG_SERVER_URL=") == 0)
        {
            _xconf_server_url = line.substr(22);
        }
        else if (line.find("RFC_CONFIG_SERVER_URL_EU=") == 0)
        {
            _xconf_server_url_eu = line.substr(25);
        }
    }
    inputFile.close();

    if (rfc_state != Local)
    {
        std::string tmp_URL;
        // Execute dmcli command to get the XconfURL
        FILE* pipe = popen("dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.XconfURL | grep string | cut -d\":\" -f3- | cut -d\" \" -f2- | tr -d ' '", "r");
        if (pipe)
        {
            char buffer[256] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != NULL)
            {
                tmp_URL = buffer;
                // Remove newline if present
                size_t pos = tmp_URL.find_last_not_of("\r\n");
                if (pos != std::string::npos)
                {
                    tmp_URL.erase(pos + 1);
                }
            }
            pclose(pipe);
        }

        if (!tmp_URL.empty())
        {
            _xconf_server_url = tmp_URL + "/featureControl/getSettings";
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Using TR181 URL: [%s]\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
        }
        else
        {
            if (_partnerId == "sky-uk" && !_xconf_server_url_eu.empty())
            {
                _xconf_server_url = _xconf_server_url_eu;
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Using EU URL for sky-uk\n", __FUNCTION__, __LINE__);
            }

            RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] RFC: TR181 URL is empty\n", __FUNCTION__, __LINE__);
        }

        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Initial URL: [%s]\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
    }
    else // rfc_state == Local
    {
        if (_partnerId == "sky-uk" && !_xconf_server_url_eu.empty())
        {
            _xconf_server_url = _xconf_server_url_eu;
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Using EU URL for sky-uk in LOCAL mode\n", __FUNCTION__, __LINE__);
        }
    }
#else
    _xconf_server_url.clear();
    std::string line;
    while (std::getline(inputFile, line))
    {
        if (line.find("RFC_CONFIG_SERVER_URL=") == 0)
        {
            _xconf_server_url = line.substr(22);
            break; // Found what we need, exit loop
        }
    }
    inputFile.close();
#endif

    if (_xconf_server_url.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] URL not found in the file.\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] _xconf_server_url: [%s]\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
    return SUCCESS;
}

int RuntimeFeatureControlProcessor::GetBootstrapXconfUrl(std ::string &XconfUrl) 
{
    int i = 0;
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    std::string str = "XconfUrl";
    const int MAX_RETRIES = 10;
    const int RETRY_DELAY_SECONDS = 10;

    for (int retryCount = 0; retryCount < MAX_RETRIES; retryCount++)
    {
        i = read_RFCProperty(str.c_str(), BOOTSTRAP_XCONF_URL_KEY_STR, tempbuf, szBufSize);
        if (i == READ_RFC_FAILURE)
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "RFC Read Failed for Bootstrap XconfUrl, retry %d/%d\n", retryCount + 1, MAX_RETRIES);
            if (retryCount < MAX_RETRIES - 1)
            {
                sleep(RETRY_DELAY_SECONDS);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "All retries exhausted for Bootstrap XconfUrl\n");
                return -1;
            }
        }
        else
        {
            i = strnlen(tempbuf, szBufSize);
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "XconfUrl: = %s, found after %d attempts\n", tempbuf, retryCount + 1);
            XconfUrl = tempbuf;
            return 0;
        }
    }

    return -1;
}

bool RuntimeFeatureControlProcessor::checkBootstrap(const std::string& filename, const std::string& target)
{
    std::ifstream file(filename);
    std::string line;
    bool result=false;

    if (file.is_open()) 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File Open %s\n", __FUNCTION__, __LINE__, filename.c_str());
        while (std::getline(file, line))
        {
            if (line.find(target) != std::string::npos)
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Bootstrap Found %s\n", __FUNCTION__, __LINE__, target.c_str());
                result = true;
                break;
            }
        }
        file.close();
    }

    return result;
}

bool RuntimeFeatureControlProcessor::getRebootRequirement()
{
    return isRebootRequired;
}

int RuntimeFeatureControlProcessor::getEffectiveImmediateParam(JSON *feature, RuntimeFeatureControlObject *rfcObj)
{
    char buffer[6] = {0};
    char rfcFeatureImmdStr[] = RFC_FEATURE_EFF_IMMD_STR;
    int result = FAILURE;
    int size =  GetJsonVal(feature,rfcFeatureImmdStr, buffer , 6);
    if(size)
    {
        rfcObj->effectiveImmediate = ((strcasecmp(buffer, "true") == 0)||(strcmp(buffer, "1") == 0)) ? true : false;
        result = SUCCESS;
    }
    return result;
}

int RuntimeFeatureControlProcessor::getRFCEnableParam(JSON *feature, RuntimeFeatureControlObject *rfcObj)
{
    char buffer[6] = {0};
    char rfcFeatureEnableStr[] = RFC_FEATURES_ENABLE_STR;
    int result = FAILURE;
    int size =  GetJsonVal(feature, rfcFeatureEnableStr, buffer , 6);
    if(size)
    {
        rfcObj->enable = ((strcasecmp(buffer, "true") == 0)||(strcmp(buffer, "1") == 0)) ? true : false;
        result = SUCCESS;
    }
    return result;
}

int RuntimeFeatureControlProcessor::getFeatureInstance(JSON *feature, RuntimeFeatureControlObject *rfcObj)
{
    char buffer[RFC_MAX_LEN] = {0};
    char rfcFeatureInstanceStr[] = RFC_FEATURE_INSTANCE_STR;
    int result = FAILURE;
    int size =  GetJsonVal(feature, rfcFeatureInstanceStr, buffer , RFC_MAX_LEN);
    if(size)
    {
        rfcObj->featureInstance = buffer;
        result = SUCCESS;
    }
    return result;
}

int RuntimeFeatureControlProcessor::getRFCName(JSON *feature, RuntimeFeatureControlObject *rfcObj)
{
    char buffer[RFC_MAX_LEN] = {0};
    char rfcFeatureNameStr[] = RFC_FEATURES_NAME_STR;
    int result = FAILURE;
    int size =  GetJsonVal(feature, rfcFeatureNameStr, buffer , RFC_MAX_LEN);
    if(size)
    {
        rfcObj->name = buffer;
        result = SUCCESS;
    }
    return result;
}

bool RuntimeFeatureControlProcessor::isXconfSelectorSlotProd()
{
    int i = 0;
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    std::string prod_str = "prod";
    bool result = false;

    /* Get Hash Value*/
    i = read_RFCProperty("XconfSelector", XCONF_SELECTOR_KEY_STR, tempbuf, szBufSize);
    if (i == READ_RFC_FAILURE) 
    {
        return result;
    } 
    else 
    {
        i = strnlen(tempbuf, szBufSize);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] XconfSelector = %s\n", __FUNCTION__, __LINE__, tempbuf);
    }

    if(0 == prod_str.compare(tempbuf))
    {
        result=true;
    }

    return result;
}

void RuntimeFeatureControlProcessor::clearDB(void)
{

    // Store permanent parameters before clearing (equivalent to rfcStashStoreParams)	
    rfcStashStoreParams();	
    // clear RFC data store before storing new values
    // this is required as sometime key value pairs will simply
    // disappear from the config data, as mac is mostly removed
    // to disable a feature rather than having different value
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Clearing DB\n", __FUNCTION__,__LINE__);
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Resetting all rfc values in backing store\n", __FUNCTION__,__LINE__);

#ifndef RDKC
    std::string name = "rfc";
    const std::string clearValue = "true";
    std::string ClearDB = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ClearDB";
    std::string BootstrapClearDB = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.Control.ClearDB";
    std::string ConfigChangeTimeKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigChangeTime";    

    std::time_t timestamp = std::time(nullptr);
    std::string ConfigChangeTime = std::to_string(timestamp);

    std::ofstream touch_file(TR181STOREFILE);
    touch_file.close();	

    set_RFCProperty(name, ClearDB, clearValue);
    set_RFCProperty(name, BootstrapClearDB, clearValue);
    set_RFCProperty(name, ConfigChangeTimeKey, ConfigChangeTime);

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Clearing DB Value: %s\n", __FUNCTION__,__LINE__,ClearDB.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Bootstrap Clearing DB Value: %s\n", __FUNCTION__,__LINE__,BootstrapClearDB.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigChangeTime: %s\n", __FUNCTION__,__LINE__,ConfigChangeTime.c_str());

#else
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Clearing tr181 store\n", __FUNCTION__,__LINE__);
    if (std::remove(TR181STOREFILE) == 0)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File %s removed successfully\n",__FUNCTION__, __LINE__, TR181STOREFILE);
    }
#endif

    // Now retrieve parameters that must persist
    rfcStashRetrieveParams();
}

void RuntimeFeatureControlProcessor::rfcStashStoreParams(void)
{
    // Store parameters that should survive the DB clear
    // Implementation depends on which parameters need to persist
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Storing permanent parameters\n", __FUNCTION__, __LINE__);

    // Call existing GetAccountID() to ensure _accountId is populated
    GetAccountID();

    // Store the current AccountID before clearing DB
    stashAccountId = _accountId;

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Stashed AccountID: %s\n", __FUNCTION__, __LINE__, stashAccountId.c_str());
}

void RuntimeFeatureControlProcessor::rfcStashRetrieveParams(void)
{
    // Restore the stored permanent parameters
    // Implementation depends on which parameters need to persist
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Retrieving permanent parameters\n", __FUNCTION__, __LINE__);

    if (!stashAccountId.empty() && stashAccountId != "Unknown")
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Restoring AccountID: %s\n", __FUNCTION__, __LINE__, stashAccountId.c_str());

        std::string name = "rfc";

        WDMP_STATUS status = set_RFCProperty(name, RFC_ACCOUNT_ID_KEY_STR, stashAccountId);
        if (status != WDMP_SUCCESS)
        {
#if !defined(RDKB_SUPPORT)		
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to restore AccountID: %s\n", __FUNCTION__, __LINE__, getRFCErrorString(status));
#endif	    
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully restored AccountID\n", __FUNCTION__, __LINE__);
            // Update the in-memory copy as well
            _accountId = stashAccountId;
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] No valid AccountID to restore\n", __FUNCTION__, __LINE__);
    }
}

void RuntimeFeatureControlProcessor::clearDBEnd(void){
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Clearing DB End\n", __FUNCTION__,__LINE__);

    std::string name = "rfc";
    const std::string clearValue = "true";
    std::string ClearDBEndKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ClearDBEnd";
    std::string BootstrapClearDBEndKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.Control.ClearDBEnd";
    std::string reloadCacheKey = "RFC_CONTROL_RELOADCACHE";
    
    set_RFCProperty(name, ClearDBEndKey, clearValue);
    set_RFCProperty(name, BootstrapClearDBEndKey, clearValue);
    set_RFCProperty(name, reloadCacheKey, clearValue);

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Clearing DBEnd Key Value: %s\n", __FUNCTION__,__LINE__,ClearDBEndKey.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Bootstrap Clearing DBEnd Key Value: %s\n", __FUNCTION__,__LINE__,BootstrapClearDBEndKey.c_str());
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Reload Cache Key: %s\n", __FUNCTION__,__LINE__,reloadCacheKey.c_str());
}

void RuntimeFeatureControlProcessor::updateHashInDB(std::string configSetHash)
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Config Set Hash = %s\n", __FUNCTION__, __LINE__, configSetHash.c_str());

#if !defined(RDKB_SUPPORT)
    std::string ConfigSetHashName = "ConfigSetHash";
    std::string ConfigSetHash_key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetHash";
    set_RFCProperty(ConfigSetHashName, ConfigSetHash_key, configSetHash);
#else
    const std::string RFC_RAM_PATH = "/tmp/RFC";
    std::string filePath = RFC_RAM_PATH + "/.hashValue";

    std::ofstream file(filePath);

    if (file.is_open()) {
        // Write the hash value to the file
        file << configSetHash;
        file.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully wrote hash to %s\n", __FUNCTION__, __LINE__, filePath.c_str());
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Error: Unable to open file %s for writing\n", __FUNCTION__, __LINE__, filePath.c_str());
    }
#endif
    if(true == StringCaseCompare(bkup_hash, configSetHash))
    {
        isRebootRequired = false;
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Reboot is not required\n", __FUNCTION__, __LINE__);
    }
}

void RuntimeFeatureControlProcessor::updateTimeInDB(std::string timestampString)
{
#if !defined(RDKB_SUPPORT)
    std::string ConfigSetTimeName = "ConfigSetTime";
    std::string ConfigSetTime_Key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime";
    set_RFCProperty(ConfigSetTimeName, ConfigSetTime_Key, timestampString);
#else
    const std::string RFC_RAM_PATH = "/tmp/RFC";
    std::string filePath = RFC_RAM_PATH + "/.timeValue";

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << timestampString;
        file.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully wrote timestamp to %s\n", __FUNCTION__, __LINE__, filePath.c_str());
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Error: Unable to open file %s for writing\n", __FUNCTION__, __LINE__, filePath.c_str());
    }
#endif

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Timestamp as string: = %s\n", __FUNCTION__, __LINE__, timestampString.c_str());
}

void RuntimeFeatureControlProcessor::updateHashAndTimeInDB(char *curlHeaderResp)
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Xconf Header Response: %s\n", __FUNCTION__, __LINE__, curlHeaderResp);
    std::string httpHeader = curlHeaderResp;

    std::string key1 = "configSetHash:";
    std::string key2 = "configsethash:";
    std::size_t start = httpHeader.find(key1);
    if (start == std::string::npos) {
        // some xconf have lowercase string.
        start = httpHeader.find(key2);
    }

    if (start != std::string::npos) {
        // Start position of the value
        start += key1.length();

        // Find the end of the line where the value ends (handle both \r\n and \n line endings)
        std::size_t end = httpHeader.find("\r\n", start);
        if (end == std::string::npos) {
            end = httpHeader.find("\n", start);  // Fallback to Unix line ending
        }

        // Extract the value
        std::string configSetHashValue = httpHeader.substr(start, end - start);

        // Trim any leading or trailing spaces and \r characters
        configSetHashValue.erase(0, configSetHashValue.find_first_not_of(" \t\r"));
        configSetHashValue.erase(configSetHashValue.find_last_not_of(" \t\r") + 1);

        // Output the value
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "configSetHash value: %s\n", configSetHashValue.c_str());

        updateHashInDB(configSetHashValue);
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "configSetHash not found in httpHeader!");
    }

    
    std::time_t timestamp = std::time(nullptr); // Get current timestamp
    std::string timestampString = std::to_string(timestamp);
    updateTimeInDB(timestampString);

    std::fstream fs;
    fs.open(RFC_SYNC_DONE, std::ios::out);
    fs.close();
}

bool RuntimeFeatureControlProcessor::IsDirectBlocked()
{
    bool directret = false;

#if !defined(RDKB_SUPPORT)
    const unsigned int direct_block_time = 86400;
    struct stat fileStat;

    if (stat(DIRECT_BLOCK_FILENAME, &fileStat) == 0) {
        // Get current time
        time_t currentTime;
        time(&currentTime);

        // Calculate time difference
        long modtime = difftime(currentTime, fileStat.st_mtime);
        long remtime = (direct_block_time / 3600) - (modtime / 3600);

        if (modtime <= (long)direct_block_time)
        {
            std::string remtime_str = std::to_string(remtime);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] RFC: Last direct failed blocking is still valid for %s hrs, preventing direct\n", __FUNCTION__, __LINE__, remtime_str.c_str());
            directret = true;
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] RFC: Last direct failed blocking has expired, removing %s, allowing direct \n", __FUNCTION__, __LINE__, DIRECT_BLOCK_FILENAME);
            if (remove(DIRECT_BLOCK_FILENAME) != 0)
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d]Failed to remove file %s\n", __FUNCTION__, __LINE__, DIRECT_BLOCK_FILENAME);
            }
        }
    }
#endif

    return directret;
}

int RuntimeFeatureControlProcessor::ProcessRuntimeFeatureControlReq()
{
    int retries = 0;
	int sleep_time = 0;
    /* Check if New Firmware Request*/

    rfcSelectOpt = (rfc_state == Local) ? "local" : "prod";
    int result = FAILURE;

    bool skip_direct = IsDirectBlocked();
    bool dbgServices = isDebugServicesEnabled();

    if(skip_direct == false)
    {
        while(retries < RETRY_COUNT)
        {
            if((filePresentCheck(RFC_PROPERTIES_PERSISTENCE_FILE) == RDK_API_SUCCESS) && (_ebuild_type != ePROD || dbgServices == true))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Setting URL from local override to %s\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
                NotifyTelemetry2Value("SYST_INFO_RFC_XconflocalURL", _xconf_server_url.c_str());
            }
            else 
            {
                if (!_boot_strap_xconf_url.empty())
                {
                    _xconf_server_url.clear();
                    _xconf_server_url = _boot_strap_xconf_url + "/featureControl/getSettings";
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Setting URL from Bootstrap config XCONF_BS_URL:%s to %s \n", __FUNCTION__, __LINE__, _boot_strap_xconf_url.c_str(), _xconf_server_url.c_str());
                    NotifyTelemetry2Value("SYST_INFO_RFC_XconfBSURL", _xconf_server_url.c_str());
                }
            }
            std::stringstream url = CreateXconfHTTPUrl();
            

            DownloadData DwnLoc, HeaderDwnLoc;
            InitDownloadData(&DwnLoc);
            InitDownloadData(&HeaderDwnLoc);

            if(allocDowndLoadDataMem( &DwnLoc, DEFAULT_DL_ALLOC ) != 0)
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to allocate memory using malloc\n", __FUNCTION__, __LINE__ );
                break;
            } 
            if(allocDowndLoadDataMem( &HeaderDwnLoc, DEFAULT_DL_ALLOC ) != 0)
            {
                if( DwnLoc.pvOut != NULL )
                {
                    free( DwnLoc.pvOut );
                }
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to allocate memory using malloc\n", __FUNCTION__, __LINE__ );
                break;
            }

            int rc = DownloadRuntimeFeatutres(&DwnLoc, &HeaderDwnLoc, url.str());
            if(rc == SUCCESS)
            {
                PreProcessJsonResponse((char *)DwnLoc.pvOut);
                if(rfc_state == Redo)
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC requires new Xconf request to server %s!\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
                }
                else if(rfc_state == Redo_With_Valid_Data)
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC requires new Xconf request with accountId %s, partnerId %s!\n", __FUNCTION__, __LINE__, _accountId.c_str(), _partner_id.c_str());
                    rfc_state = Init;
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Continue processing RFC response.\n", __FUNCTION__, __LINE__);
#if defined(RDKB_SUPPORT)
                    if(ProcessJsonResponseB((char *)DwnLoc.pvOut) == SUCCESS)
                    {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC processing Successfully.\n", __FUNCTION__, __LINE__);
                        updateHashAndTimeInDB((char *)HeaderDwnLoc.pvOut);
                        result = SUCCESS;
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Processing Response Failed!!\n", __FUNCTION__, __LINE__);
                        updateHashInDB("CLEARED");
                        updateTimeInDB("0");
                    }
#else		    
                    if(ProcessJsonResponse((char *)DwnLoc.pvOut) == SUCCESS)
                    {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC processing Successfully.\n", __FUNCTION__, __LINE__);
                        updateHashAndTimeInDB((char *)HeaderDwnLoc.pvOut);
                        result = SUCCESS;
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Processing Response Failed!!\n", __FUNCTION__, __LINE__);
                        updateHashInDB("CLEARED");
                        updateTimeInDB("0");
                    }
#endif		    
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] COMPLETED RFC PASS\n", __FUNCTION__, __LINE__);
   	            NotifyTelemetry2Count("SYST_INFO_RFC_Complete");
                    set_RFCProperty(XCONF_SELECTOR_NAME, XCONF_SELECTOR_KEY_STR, rfcSelectOpt.c_str());
                    set_RFCProperty(XCONF_URL_TR181_NAME, XCONF_URL_KEY_STR, _xconf_server_url.c_str());
                    
                    break;
                }
            }
            if( DwnLoc.pvOut != NULL )
            {
                free( DwnLoc.pvOut );
            }
            if( HeaderDwnLoc.pvOut != NULL )
            {
                free( HeaderDwnLoc.pvOut );
            }
            if(rc == NO_RFC_UPDATE_REQUIRED)
            {
                result = SUCCESS;
                break;
            }
            retries++;
#ifdef RDKB_SUPPORT
            if (retries == 1) {
                sleep_time = RETRY_DELAY;
            } else {
                sleep_time = RDKB_RETRY_DELAY;
            }
#else
            sleep_time = RETRY_DELAY;
#endif
            sleep(sleep_time);
        }
    }
    return result;
}

bool urlencodingInterface(const std::string& input, std::stringstream& output) {
    char* temp = urlEncodeString(input.c_str());
    if (temp) {
        output << temp;
        free(temp);
        return true;
    }
    return false;
}

void EncodeString(const std::string& key, const std::string& value, std::stringstream& output, const std::string& appender = "") {
    if (!key.empty()) {
        output << key;
        if (!value.empty()) {
            urlencodingInterface(value, output);
        }
        if (!appender.empty()) {
            output << appender;
        }
    }
}

std::stringstream RuntimeFeatureControlProcessor::CreateXconfHTTPUrl() 
{
    std::stringstream url;
    url << _xconf_server_url << "?";
    url << "estbMacAddress=" << _estb_mac_address << "&";
    url << "firmwareVersion=" << _firmware_version << "&";
    url << "env=" << _build_type_str << "&";
    url << "model=" << _model_number << "&";
    url << "manufacturer=" << _manufacturer << "&";
    url << "controllerId=" << RFC_VIDEO_CONTROL_ID << "&";
    url << "channelMapId=" << RFC_CHANNEL_MAP_ID << "&";
    url << "VodId=" << RFC_VIDEO_VOD_ID << "&";
    url << "partnerId=" << _partner_id << "&";
    url << "osClass=" << _osclass << "&";
    url << "accountId=" << _accountId << "&";
    url << "Experience=" << _experience << "&";
    url << "version=" << 2;
    
    #ifndef URLENCODING_DISABLED
    std::stringstream encodedUrl;
    encodedUrl << _xconf_server_url << "?";

    EncodeString("estbMacAddress=", _estb_mac_address, encodedUrl, "&");
    EncodeString("firmwareVersion=", _firmware_version, encodedUrl, "&");
    EncodeString("env=", _build_type_str, encodedUrl, "&");
    EncodeString("model=", _model_number, encodedUrl, "&");
    EncodeString("manufacturer=", _manufacturer, encodedUrl, "&");

    encodedUrl << "controllerId=" << RFC_VIDEO_CONTROL_ID << "&";
    encodedUrl << "channelMapId=" << RFC_CHANNEL_MAP_ID << "&";
    encodedUrl << "VodId=" << RFC_VIDEO_VOD_ID << "&";

    EncodeString("partnerId=", _partner_id, encodedUrl, "&");
    EncodeString("osClass=", _osclass, encodedUrl, "&");
    EncodeString("accountId=", _accountId, encodedUrl, "&");
    EncodeString("Experience=", _experience, encodedUrl, "&");
    encodedUrl << "version=2";

    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Encoding is enabled plain URL: %s\n", __FUNCTION__, __LINE__, url.str().c_str());
    
    return encodedUrl; // Use encoded URL
    #endif
    
    return url;
}

void RuntimeFeatureControlProcessor::GetStoredHashAndTime( std ::string &valueHash, std::string &valueTime ) 
{
    if(!_last_firmware.compare( _firmware_version ))
    {
        /*Both the input strings are equal.*/
        if((rfc_state == Init) && (isXconfSelectorSlotProd() == true))
        {
            valueTime = "0";
            valueHash = "OVERRIDE_HASH";
        }
        else
        {
            RetrieveHashAndTimeFromPreviousDataSet(valueHash, valueTime);
        }
    }
    else
    {
        valueTime = "0";
        valueHash = "UPGRADE_HASH";
    }

    std::string InvalidStr = "Unkown";
    if ((!_partner_id.compare(InvalidStr)) || (!_accountId.compare(InvalidStr)))
    {
        valueHash="OVERRIDE_HASH";
    }

    return;

}

void RuntimeFeatureControlProcessor::RetrieveHashAndTimeFromPreviousDataSet(std::string &valueHash, std::string &valueTime)
{
#if defined(RDKB_SUPPORT)
    const std::string RFC_RAM_PATH = "/tmp/RFC";

    // Initialize default values
    valueHash = "UPGRADE_HASH";
    valueTime = "0";

    // Read hash value
    std::string hashFilePath = RFC_RAM_PATH + "/.hashValue";
    if (access(hashFilePath.c_str(), R_OK) == 0) {
        std::ifstream hashFile(hashFilePath);
        if (hashFile.is_open()) {
            std::getline(hashFile, valueHash);
            hashFile.close();
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetHash: Hash = %s (from file)\n", __FUNCTION__, __LINE__, valueHash.c_str());
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open hash file: %s\n", __FUNCTION__, __LINE__, hashFilePath.c_str());
        }
    } else {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Hash file not found, using default value\n", __FUNCTION__, __LINE__);
    }

    // Read time value
    std::string timeFilePath = RFC_RAM_PATH + "/.timeValue";
    if (access(timeFilePath.c_str(), R_OK) == 0) {
        std::ifstream timeFile(timeFilePath);
        if (timeFile.is_open()) {
            std::getline(timeFile, valueTime);
            timeFile.close();
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetTime: Set Time = %s (from file)\n", __FUNCTION__, __LINE__, valueTime.c_str());
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open time file: %s\n", __FUNCTION__, __LINE__, timeFilePath.c_str());
        }
    } else {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Time file not found, using default value\n", __FUNCTION__, __LINE__);
    }

#else
    int i = 0;
    char tempbuf[1024] = {0};
    int szBufSize = sizeof(tempbuf);
    std::string ConfigSetHash = "ConfigSetHash";
    std::string ConfigSetTime = "ConfigSetTime";

    /* Get Hash Value*/
    i = read_RFCProperty(ConfigSetHash.c_str(), RFC_CONFIG_SET_HASH, tempbuf, szBufSize);
    if (i == READ_RFC_FAILURE)
    {
        i = snprintf(tempbuf, szBufSize, "UPGRADE_HASH");
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetHash: read_RFCProperty() failed Status %d\n", __FUNCTION__, __LINE__, i);
    }
    else
    {
        i = strnlen(tempbuf, szBufSize);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetHash: Hash = %s\n", __FUNCTION__, __LINE__, tempbuf);
    }
    valueHash = tempbuf;

    /* Get Time Value */
    memset(tempbuf, 0, 1024);
    i = read_RFCProperty(ConfigSetTime.c_str(), RFC_CONFIG_SET_TIME, tempbuf, szBufSize);
    if (i == READ_RFC_FAILURE)
    {
        i = snprintf(tempbuf, szBufSize, "0");
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetTime: read_RFCProperty() failed Status %d\n", __FUNCTION__, __LINE__, i);
    }
    else
    {
        i = strnlen(tempbuf, szBufSize);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ConfigSetTime: Set Time = %s\n", __FUNCTION__, __LINE__, tempbuf);
    }
    valueTime = tempbuf;
#endif

    // Save backup hash for both implementations
    bkup_hash = valueHash;

    return;
}

void RuntimeFeatureControlProcessor::InitDownloadData(DownloadData *pDwnData)
{
    pDwnData->pvOut = nullptr;
    pDwnData->datasize = 0;
    pDwnData->memsize = 0;
}

int RuntimeFeatureControlProcessor::DownloadRuntimeFeatutres(DownloadData *pDwnLoc, DownloadData *pHeaderDwnLoc, const std::string& url_str) 
{
    int ret_value = FAILURE;
    void *curl = nullptr;
    hashParam_t *hashParam = nullptr;
    MtlsAuth_t sec;
    int httpCode = -1;

    hashParam = (hashParam_t *)malloc(sizeof(hashParam_t));
    if(!hashParam)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Memory allocation Failed\n",__FUNCTION__, __LINE__);
        return ret_value;
    }

    memset(&sec, '\0', sizeof(MtlsAuth_t));
    int ret = getMtlscert(&sec);
    if(ret == MTLS_FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] MTLS  certification Failed\n",__FUNCTION__, __LINE__);
    }

    if((pDwnLoc->pvOut != NULL) && (pHeaderDwnLoc->pvOut != NULL))
    {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Curl Initialized\n", __FUNCTION__, __LINE__);
            FileDwnl_t file_dwnl;

            memset(&file_dwnl, '\0', sizeof(FileDwnl_t));

            file_dwnl.chunk_dwnl_retry_time = 0;
            file_dwnl.pDlData = pDwnLoc;
            file_dwnl.pDlHeaderData = pHeaderDwnLoc;
            *(file_dwnl.pathname) = 0;

            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Xconf Request : [%s]\n", __FUNCTION__, __LINE__, url_str.c_str());
            strncpy(file_dwnl.url, url_str.c_str(), sizeof(file_dwnl.url)-1);
            
            std::string hashValue;
            std::string valueTime;
            GetStoredHashAndTime(hashValue, valueTime);

	    std::string configsethashParam = (std::string("configsethash:") + hashValue.c_str());
	    std::string configsettimeParam = (std::string("configsettime:") + valueTime.c_str());

            hashParam->hashvalue = strdup((char *)configsethashParam.c_str());
            hashParam->hashtime = strdup((char *)configsettimeParam.c_str());

            file_dwnl.hashData = hashParam;

            /* Handle MTLS failure case as well. */
            int curl_ret_code = 0;
            if (ret == MTLS_FAILURE)
            {
                /* RDKE-419: No valid data in 'sec' buffer, pass NULL */
                curl_ret_code = ExecuteRequest(&file_dwnl, NULL, &httpCode);
            }
            else
            {
                curl_ret_code = ExecuteRequest(&file_dwnl, &sec, &httpCode);
            }
	    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC Xconf Connection Response cURL Return : %d HTTP Code : %d\n",__FUNCTION__, __LINE__, curl_ret_code, httpCode);
            CURLcode curl_code = (CURLcode)curl_ret_code;
            const char *error_msg = curl_easy_strerror(curl_code);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] curl_easy_strerror =%s\n", __FUNCTION__, __LINE__, error_msg);

            if(curl)
            {
                doStopDownload(curl);
                curl = nullptr;
            }
            if(file_dwnl.hashData != nullptr)
            {
                free(file_dwnl.hashData);
            }
	    
	    if (_url_validation_in_progress)
	    {
		_url_validation_in_progress = false;
		if((httpCode == 304) || (httpCode == 200))
		{
		    return SUCCESS;
		}
		return FAILURE;
	    }
	    
            switch(curl_ret_code)
            {
                case  6:
                case 18:
                case 28:
                case 35:
                case 51:
                case 53:
                case 54:
                case 58:
                case 59:
                case 60:
                case 64:
                case 66:
                case 77:
                case 80:
                case 82:
                case 83:
                case 90:
                case 91:
                NotifyTelemetry2ErrorCode(curl_ret_code);
            }

            if((curl_ret_code == 0) && (httpCode == 404))
            {
		RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Received HTTP %d Response from Xconf Server. Retry logic not needed!!!\n",__FUNCTION__, __LINE__,httpCode);
	        cleanAllFile();
		RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[Features Enabled]-[NONE]:\n");
		NotifyTelemetry2Count("SYST_INFO_RFC_FeaturesNone");
            }
            else if(httpCode == 304)
            {
		RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] HTTP request success. Response unchanged (%d). No processing\n",__FUNCTION__, __LINE__,httpCode);
                NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "ACTIVE");
                ret_value= NO_RFC_UPDATE_REQUIRED;
            }
            else if((curl_ret_code != 0) || (httpCode != 200))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] HTTP request failed\n",__FUNCTION__, __LINE__);
            }
            else 
            {
		RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] HTTP request success, Processing updated (%d) response...\n",__FUNCTION__, __LINE__,httpCode);
                cleanAllFile();
                if (std::remove(TR181LISTFILE) == 0)
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File %s removed successfully\n",__FUNCTION__, __LINE__, TR181LISTFILE);
                }
                ret_value= SUCCESS;
            }
    }
    return ret_value;
}

void RuntimeFeatureControlProcessor::NotifyTelemetry2ErrorCode(int CurlReturn)
{
    std::string CurlReturnStr = std::to_string(CurlReturn);; // Replace with the actual TLSRet value
    std::string FQDN = _xconf_server_url; // Replace with the actual FQDN value

    RemoveSubstring(FQDN,"/featureControl/getSettings");

    const char* arg1 = "certerr_split";
    std::string arg2 = "RFC, " + CurlReturnStr + ", " + FQDN;
    v_secure_system("/usr/bin/telemetry2_0_client %s %s", arg1, arg2.c_str());
}



void RuntimeFeatureControlProcessor::PreProcessJsonResponse(char *xconfResp)
{

    if((rfc_state == Init) || ( _is_first_request == true))
    {
        /* Parse JSON */
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Xconf Response: %s\n", __FUNCTION__, __LINE__, xconfResp);
        JSON *pJson = ParseJsonStr(xconfResp);
        if(pJson)
        {
            JSON *features = GetRuntimeFeatureControlJSON(pJson);
            if(features)
            {
                CreateConfigDataValueMap(features);
                if( _is_first_request == true)
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] GetValidAccountId\n", __FUNCTION__, __LINE__);
                    GetValidAccountId();
                    GetValidPartnerId();
                    _is_first_request = false;
                }
        
                if(rfc_state == Init)
                {
                    if(rfcSelectOpt == "local")
                    {
                        rfc_state = Finish;
                    }
                    else
                    {
                        GetXconfSelect();
                    }
                }
                FreeJson(pJson);
                _RFCKeyAndValueMap.clear();
            }
        }
    }
    else if(rfc_state == Redo) 
    {
        rfc_state = Finish;
    }
}

void RuntimeFeatureControlProcessor::GetValidAccountId()
{
    /* Get Valid Account ID*/

    std::string value = _RFCKeyAndValueMap[RFC_ACCOUNT_ID_KEY_STR];

    if(value.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Not found Account ID\n", __FUNCTION__, __LINE__);
        value = "Unknown";
    }

    if(true == CheckSpecialCharacters(value))
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid characters in newly received accountId: %s\n", __FUNCTION__, __LINE__,value.c_str());    
    }
    else
    {
        _valid_accountId = value;
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] NEW valid Account ID: %s\n", __FUNCTION__, __LINE__, _valid_accountId.c_str());

        std::string unknown_str = "Unknown";
        if(false == StringCaseCompare( _valid_accountId, unknown_str))
        {
            if (checkWhoamiSupport()) {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Do not retry when WAI is in use\n", __FUNCTION__, __LINE__);
            }else {
                rfc_state = Redo_With_Valid_Data;
            }
            _accountId = _valid_accountId;
        }
        else
        {
            if(false == StringCaseCompare( _accountId,unknown_str))
            {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Prior Account Id from device=%s, is not valid.\n", __FUNCTION__, __LINE__, _accountId.c_str());
            }
            else
            {
                _valid_accountId = _accountId;
                if (checkWhoamiSupport()) {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Do not retry when WAI is in use\n", __FUNCTION__, __LINE__);
                }else {
                    rfc_state = Redo_With_Valid_Data;
                }
            }
        }
    }
    return;
}

void RuntimeFeatureControlProcessor::GetValidPartnerId()
{
    /* Get Valid Partner ID*/

    std::string value = _RFCKeyAndValueMap[RFC_PARTNER_ID_KEY_STR];

    if(value.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Not found Parner ID\n", __FUNCTION__, __LINE__);
        _valid_partnerId = "Unknown";
    }
    else
    {
       if(true == CheckSpecialCharacters(value))
       {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid characters in newly received accountId: %s\n", __FUNCTION__, __LINE__,value.c_str());    
       }
       else
       {
            _valid_partnerId = value;
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] NEW valid Partner ID: %s\n", __FUNCTION__, __LINE__, _valid_partnerId.c_str());

            std::string unknown_str = "Unknown";

            if(false == StringCaseCompare( _valid_partnerId, unknown_str))
            {
                if (checkWhoamiSupport()) {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Do not retry when WAI is in use\n", __FUNCTION__, __LINE__);
                }else {
                    rfc_state = Redo_With_Valid_Data;
                }
                _partner_id = _valid_partnerId;
            }
            else
            {
                if(false == StringCaseCompare( _partner_id,unknown_str))
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Prior Partner Id from device=%s, is not valid.\n", __FUNCTION__, __LINE__, _partner_id.c_str());
                }
                else
                {
                    _valid_partnerId = _partner_id;
                    if (checkWhoamiSupport()) {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Do not retry when WAI is in use\n", __FUNCTION__, __LINE__);
                    }else {
                        rfc_state = Redo_With_Valid_Data;
                    }
                }
            }
        }
    }
    return;
}

void RuntimeFeatureControlProcessor::GetXconfSelect()
{
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] GetXconfSelect rfc_state: %d\n", __FUNCTION__, __LINE__, rfc_state);
    std::string XconfUrl = _RFCKeyAndValueMap[XCONF_URL_KEY_STR];
    std::string ci_str = "ci";
    std::string auto_str = "automation";

    if(!XconfUrl.empty())
    {
        _xconf_server_url.clear();
        _xconf_server_url = XconfUrl;
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] NEW Xconf URL configured=%s\n", __FUNCTION__, __LINE__, _xconf_server_url.c_str());
    }

    std::string XconfSelector = _RFCKeyAndValueMap[XCONF_SELECTOR_KEY_STR];

    if(!XconfSelector.empty())
    {
        if(ci_str == XconfSelector)
        {
            rfcSelectorSlot="16";
            rfc_state = Redo;
        }
        else if(auto_str == XconfSelector)
        {
            rfcSelectorSlot="19";
            rfc_state = Redo;
        }
        else
        {
            rfcSelectorSlot="8";
            rfc_state = Finish;
        }
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC Configured for Slot %s, URL %s, State rfc_state=%d\n", __FUNCTION__, __LINE__, rfcSelectorSlot.c_str(), _xconf_server_url.c_str(), rfc_state);
    }

    // Override not configured through Production Xconf, check if there is local override
    if(rfc_state == Init)
    {
        rfc_state = Finish;
    }
}

int RuntimeFeatureControlProcessor::ProcessJsonResponse(char *featureXConfMsg)
{
    std::string rfcList;

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Xconf Response: %s\n", __FUNCTION__, __LINE__, featureXConfMsg);
    JSON *pJson = ParseJsonStr(featureXConfMsg);

    if(!pJson)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }

    JSON *features = GetRuntimeFeatureControlJSON(pJson);
    JSON *bkp_features = features;

    if(!features)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Feature Parsing Failed\n", __FUNCTION__, __LINE__);
        return FAILURE;
    }

    int numFeatures = GetJsonArraySize(features);
    for (int index = 0; index < numFeatures; index++) 
    {
        JSON* feature = GetJsonArrayItem(features, index);
        if(feature)
        {
            RuntimeFeatureControlObject *rfcObj = new RuntimeFeatureControlObject;
            if(SUCCESS != getRFCName(feature, rfcObj))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Name\n", __FUNCTION__, __LINE__);
                delete rfcObj;
                continue;
            }
            if(SUCCESS != getRFCEnableParam(feature, rfcObj))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Enable Param\n", __FUNCTION__, __LINE__);
                delete rfcObj;
                continue;
            }          

            if(SUCCESS != getEffectiveImmediateParam(feature, rfcObj))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Effective Immediate\n", __FUNCTION__, __LINE__);
                delete rfcObj;
                continue;
            }

            if(SUCCESS != getFeatureInstance(feature, rfcObj))
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC Feature Instance not configured\n", __FUNCTION__, __LINE__);
            }
            else
            {
                std::string filename = ".RFC_" + rfcObj->name + ".ini";
                rfcList += rfcObj->featureInstance + "=true,";
                writeRemoteFeatureCntrlFile(filename,rfcObj);
            }
            writeRemoteFeatureCntrlFile(VARFILE,rfcObj);
            delete rfcObj;

        }
    }
    processXconfResponseConfigDataPart(bkp_features);
    WriteFile(FEATURE_FILE_LIST, rfcList);
    /* prepare json file for parsing and report feature instance list */

    NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "STAGING");

    /* Update Version File*/
    WriteFile(".version", _firmware_version);
    FreeJson(pJson);

    return SUCCESS;
}

JSON* RuntimeFeatureControlProcessor::GetRuntimeFeatureControlJSON(JSON *pJson)
{
    char featureCntlTag[] = FEATURE_CONTROL_TAG;
    char featureTag[] = FEATURES_TAG;

    JSON *featureControl = GetJsonItem(pJson, featureCntlTag);
    if(featureControl == nullptr)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Feature Control\n",__FUNCTION__, __LINE__);
        FreeJson(pJson);
        return nullptr;
    }

    JSON *features = GetJsonItem(featureControl, featureTag);
    if (!features || !IsJsonArray(features)) 
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] JSON Parsing Failed for Features Array\n", __FUNCTION__, __LINE__);
        FreeJson(pJson);
        return nullptr;
    }

    return features;
}

void RuntimeFeatureControlProcessor::NotifyTelemetry2Count(std ::string markerName)
{
    int count = 1;
    v_secure_system("/usr/bin/telemetry2_0_client %s %d", markerName.c_str(), count);
}


void RuntimeFeatureControlProcessor::NotifyTelemetry2Value(std ::string markerName, std ::string value)
{
    v_secure_system("/usr/bin/telemetry2_0_client %s %s", markerName.c_str(), value.c_str());
}

void RuntimeFeatureControlProcessor::processXconfResponseConfigDataPart(JSON *features)
{
    CreateConfigDataValueMap(features);
    std::string name = "rfc";
    std::list<std::string> paramList;
    bool configChanged = false;

    if( _RFCKeyAndValueMap.empty())
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Config Data Map is Empty\n", __FUNCTION__, __LINE__);
        return;    
    }
#if !defined(RDKB_SUPPORT)
    clearDB();
#endif    

    std::string newKey;
    std::string newValue;
    std::string currentValue;
   
     // Iterating through the map
    for (const auto& pair : _RFCKeyAndValueMap) {

        // Storing pair.first and pair.second in separate variables
        newKey = pair.first;
        newValue = pair.second;

        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Feature Name [%s] Value[%s]\n", __FUNCTION__, __LINE__, newKey.c_str(), newValue.c_str());

        configChanged = isConfigValueChange(name, newKey , newValue, currentValue);
        if(configChanged == false)
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] configValue not changed for [%s]\n", __FUNCTION__, __LINE__,newKey.c_str());
        }
        else
        {
	    if(newKey == BOOTSTRAP_XCONF_URL_KEY_STR)
	    {
		RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Feature Name [%s] Current Value[%s] New Value[%s] \n", __FUNCTION__, __LINE__, newKey.c_str(), currentValue.c_str(), newValue.c_str());
		RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Processing Xconf URL %s\n", __FUNCTION__, __LINE__, newValue.c_str());
	        if (ProcessXconfUrl(newValue.c_str()) != SUCCESS)
		{
		    continue;
		}
	    }
		
            WDMP_STATUS status = set_RFCProperty(name, newKey, newValue);
            if (status != WDMP_SUCCESS)
            {
#if !defined(RDKB_SUPPORT)
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] SET failed for key=%s with status=%s\n", __FUNCTION__, __LINE__, newKey.c_str(), getRFCErrorString(status));
#else		
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] SET failed for key=%s with status=%d\n", __FUNCTION__, __LINE__, newKey.c_str(), status);
#endif			    
            }
            else
            {
                if (newValue != currentValue)
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] updated for %s from value old=%s, to new=%s\n", __FUNCTION__, __LINE__,newKey.c_str(), currentValue.c_str(), newValue.c_str());
                    std::string account_key_str = RFC_ACCOUNT_ID_KEY_STR;
                    bool isAccountKey = (newKey.find(account_key_str) != std::string::npos) ? true : false;
                    if(isAccountKey == true)
                    {
                        NotifyTelemetry2Count("SYST_INFO_ACCID_set");
                    }
#if !defined(RDKB_SUPPORT)		    
                    if (isMaintenanceEnabled())
                    {
                        isRebootRequired = true;
                    }
#endif		    
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] reapplied for %s the same value old=%s, new=%s\n", __FUNCTION__, __LINE__,newKey.c_str(), currentValue.c_str(), newValue.c_str());
                }
            }
        }
        std::string data = "TR181: " + newKey + " " + newValue;

        paramList.push_back(data);
    }

    updateTR181File(TR181_FILE_LIST, paramList);
#if !defined(RDKB_SUPPORT)
    clearDBEnd();
#endif     
}

void RuntimeFeatureControlProcessor::CreateConfigDataValueMap(JSON *features)
{
    JSON *pConfigData = nullptr, *child = nullptr;
    char configData[]="configData";
    int numFeatures = GetJsonArraySize(features);

    for (int index = 0; index < numFeatures; index++) 
    {
        JSON* feature = GetJsonArrayItem(features, index);
        if(feature)
        {
            pConfigData = GetJsonItem(feature,configData);
            if(!pConfigData)
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Config Data not found.\n", __FUNCTION__, __LINE__);
                break;    
            }
            child = pConfigData->child;

            while(child)
            {
                 std::string key = child->string;
                 std::string value = child->valuestring;
                 RemoveSubstring(key, "tr181.");
                 _RFCKeyAndValueMap[key] =  child->valuestring;
                 child = child->next;
            }
        }
    }
    return;
}
bool RuntimeFeatureControlProcessor::isConfigValueChange(std ::string name, std ::string key, std ::string &value, std ::string &currentValue)
{
    int i = 0, len = 0;
    char tempbuf[4096] = {0};
    int szBufSize = sizeof(tempbuf);

    i = read_RFCProperty(name.c_str(), key.c_str(), tempbuf, szBufSize);
    if (i == READ_RFC_FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC Read Failed for name=%s, key=%s, value=%s\n", __FUNCTION__, __LINE__, name.c_str(), key.c_str(), value.c_str());
        return true;
    }

    len = strnlen(tempbuf, szBufSize);
    currentValue.assign(tempbuf, len);
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Current Value= %s\n", tempbuf);

    bool is_Bootstrap = checkBootstrap(BS_STORE_FILENAME, key);
    std::string substr = ".X_RDKCENTRAL-COM_RFC.";
    bool enable_Check = (key.find(substr) != std::string::npos) ? true : false;

    if (!is_Bootstrap && !enable_Check)
    {
        if (value != currentValue)
        {
            std::string account_key_str = RFC_ACCOUNT_ID_KEY_STR;
            std::string unknown_str = "Unknown";
            bool isAccountKey = (key.find(account_key_str) != std::string::npos) ? true : false;

            if(isAccountKey == true)
            {
                if(true == StringCaseCompare(value, unknown_str))
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC: AccountId %s is replaced with Authservice %s", __FUNCTION__, __LINE__,  value.c_str(), currentValue.c_str());
                    value = currentValue;
                }
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] For param %s new and old values are same value %s", __FUNCTION__, __LINE__,  key.c_str(), currentValue.c_str());
            return false;
        }
    }

    return true;
}

WDMP_STATUS RuntimeFeatureControlProcessor::set_RFCProperty(std::string name, std::string key, std::string value)
{
#if defined(RDKB_SUPPORT)
    rbusHandle_t handle;
    rbusValue_t rbusValue;
    rbusError_t rc;
    WDMP_STATUS status = WDMP_FAILURE;
    (void)name;

    // Initialize rbus connection
    rc = rbus_open(&handle, "RFC_Manager");
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open rbus handle: %s\n",
                __FUNCTION__, __LINE__, rbusError_ToString(rc));
        return WDMP_FAILURE;
    }

    // First get the parameter to determine its type
    rc = rbus_get(handle, key.c_str(), &rbusValue);
    if (rc == RBUS_ERROR_SUCCESS) {
        // Create new value with same type as existing parameter
        rbusValue_t newValue;
        rbusValue_Init(&newValue);

        // Set the new value based on the type of the existing parameter
        rbusValueType_t type = rbusValue_GetType(rbusValue);
        switch (type) {
            case RBUS_STRING:
                rbusValue_SetString(newValue, value.c_str());
                break;
            case RBUS_BOOLEAN:
                rbusValue_SetBoolean(newValue, (value == "true" || value == "1"));
                break;
            case RBUS_INT32:
                rbusValue_SetInt32(newValue, std::stoi(value));
                break;
            case RBUS_UINT32:
                rbusValue_SetUInt32(newValue, std::stoul(value));
                break;
            case RBUS_INT64:
                rbusValue_SetInt64(newValue, std::stoll(value));
                break;
            case RBUS_UINT64:
                rbusValue_SetUInt64(newValue, std::stoull(value));
                break;
            case RBUS_SINGLE:
                rbusValue_SetSingle(newValue, std::stof(value));
                break;
            case RBUS_DOUBLE:
                rbusValue_SetDouble(newValue, std::stod(value));
                break;
            default:
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Unsupported parameter type: %d\n",
                        __FUNCTION__, __LINE__, type);
                rbusValue_Release(newValue);
                rbusValue_Release(rbusValue);
                rbus_close(handle);
                return WDMP_FAILURE;
        }

        // Set the parameter with the new value
        rc = rbus_set(handle, key.c_str(), newValue, NULL);
        if (rc == RBUS_ERROR_SUCCESS) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully set parameter %s\n",
                    __FUNCTION__, __LINE__, key.c_str());
            status = WDMP_SUCCESS;
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to set parameter %s: %s\n",
                    __FUNCTION__, __LINE__, key.c_str(), rbusError_ToString(rc));
            status = WDMP_FAILURE;
        }

        // Release the values
        rbusValue_Release(newValue);
        rbusValue_Release(rbusValue);
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get parameter %s: %s\n",
                __FUNCTION__, __LINE__, key.c_str(), rbusError_ToString(rc));
        status = WDMP_FAILURE;
    }

    // Close the rbus connection
    rbus_close(handle);

    return status;
#else
    RFC_ParamData_t param;
    param.type = WDMP_NONE;
    memset(&param, 0, sizeof(RFC_ParamData_t));
    WDMP_STATUS status = getRFCParameter(NULL, key.c_str(), &param);
    if(param.type != WDMP_NONE)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Parameter Type=%d\n", __FUNCTION__, __LINE__, param.type);
        WDMP_STATUS status = setRFCParameter(name.c_str(), key.c_str(), value.c_str(), param.type);
        if (status != WDMP_SUCCESS)
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] setRFCParameter failed. key=%s and status=%s\n", __FUNCTION__, __LINE__, key.c_str(), getRFCErrorString(status));
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] setRFCParameter Success\n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to retrieve : Reason:%s\n", __FUNCTION__, __LINE__, getRFCErrorString(status));
    }
    return status;
#endif
}

void RuntimeFeatureControlProcessor::updateTR181File(const std::string& filename, std::list<std::string>& paramList) 
{
    std::string fullPath = std::string(DIRECTORY_PATH) + filename;
    std::ofstream outFile(fullPath.c_str(), std::ios::app); // Open file in append mode

    if (outFile.is_open()) 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File Open %s\n", __FUNCTION__, __LINE__, fullPath.c_str());
        while (!paramList.empty()) 
        {
            std::string data = paramList.front();
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Writing %s in file\n", __FUNCTION__, __LINE__, data.c_str());
            outFile << data << std::endl; // Write the front element to the file
            paramList.pop_front(); // Remove the front element
        }
        outFile.close();
    } 
    else 
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Unable to open file:%s\n", __FUNCTION__, __LINE__,fullPath.c_str());
    }
}

void RuntimeFeatureControlProcessor::NotifyTelemetry2RemoteFeatures(const char *rfcFeatureList, std ::string rfcstatus)
{
    std::ifstream inputFile;

    inputFile.open(rfcFeatureList);
    if (!inputFile.is_open())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open file.\n" , __FUNCTION__, __LINE__);
        return;
    }
    std::string line;
    std::getline(inputFile, line);
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[Features Enabled]-[%s]: %s \n", rfcstatus.c_str(),line.c_str());

    inputFile.close();
    if (line.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC Feature List is not found in the file.\n" , __FUNCTION__, __LINE__);
        return;
    }
    if (rfcstatus == "STAGING") {
        v_secure_system("/usr/bin/telemetry2_0_client rfc_staging_split %s", line.c_str());
    } else {
        v_secure_system("/usr/bin/telemetry2_0_client rfc_split %s", line.c_str());
    }
}

void RuntimeFeatureControlProcessor::WriteFile(const std::string& filename, const std::string& data) 
{
    std::string fullPath = std::string(DIRECTORY_PATH) + filename;
    std::ofstream outFile(fullPath.c_str());

    if (outFile.is_open()) 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File Open %s\n", __FUNCTION__, __LINE__, fullPath.c_str());
        outFile << data << "\n";
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Writing %s in file\n", __FUNCTION__, __LINE__,data.c_str());
        outFile.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File %s written successfully\n", __FUNCTION__, __LINE__,fullPath.c_str());
    } 
    else 
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Unable to open file[%s]\n", __FUNCTION__, __LINE__,fullPath.c_str());
    }
}

void RuntimeFeatureControlProcessor::writeRemoteFeatureCntrlFile(const std::string& filename, RuntimeFeatureControlObject *feature) 
{
    std::string fullPath = std::string(DIRECTORY_PATH) + filename;
    std::ofstream outFile(fullPath.c_str(), std::ios::app);

    if (outFile.is_open()) 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File Open %s\n", __FUNCTION__, __LINE__, fullPath.c_str());
        std::string enbStr = feature->enable ? "true" : "false";
        std::string effectStr = feature->effectiveImmediate ? "true" : "false";

        std::string enbData = "export RFC_ENABLE_" + feature->name + "=" + enbStr;
        std::string effectData = "export RFC_" + feature->name + "_effectiveImmediate=" + effectStr;

        outFile << enbData << "\n";
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Writing %s in file\n", __FUNCTION__, __LINE__, enbData.c_str());
        outFile << effectData << "\n";
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Writing %s in file\n", __FUNCTION__, __LINE__, effectData.c_str());
        outFile.close();

        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File %s written successfully\n", __FUNCTION__, __LINE__,fullPath.c_str());
    } 
    else 
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Unable to open file:%s\n", __FUNCTION__, __LINE__,fullPath.c_str());
    }
}

int RuntimeFeatureControlProcessor::getJsonRpc(char *post_data, DownloadData* pJsonRpc )
{
    void *Curl_req = NULL;
    int httpCode = 0;
    FileDwnl_t req_data;
    int curl_ret_code = -1;
    char header[]  = "Content-Type: application/json";
    char token_header[300] = {0};

    if (pJsonRpc->pvOut != NULL)
    {
        req_data.pHeaderData = header;
        req_data.pPostFields = post_data;
        req_data.pDlData = pJsonRpc;
        req_data.pDlHeaderData = NULL;
        req_data.hashData = NULL;
        snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
        Curl_req = doCurlInit();
        if(Curl_req != NULL)
        {
            curl_ret_code = getJsonRpcData(Curl_req, &req_data, token_header, &httpCode );
            doStopDownload(Curl_req);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] doCurlInit fail\n", __FUNCTION__, __LINE__ );
        }
    }
    else
    {

        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to allocate memory using malloc\n", __FUNCTION__, __LINE__ );
    }
    return curl_ret_code;
}

int RuntimeFeatureControlProcessor::getJRPCTokenData( char *token, char *pJsonStr, unsigned int token_size )
{
    JSON *pJson = nullptr;
    char status[8];
    int ret = -1;
    char token_str[] =  "token";
    char succ_str[] = "success";

    if (token == nullptr || pJsonStr == nullptr) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Paramter is NULL\n", __FUNCTION__, __LINE__ );
        return ret;
    }
    *status = 0;
    pJson = ParseJsonStr( pJsonStr );
    if( pJson != nullptr )
    {
        GetJsonVal(pJson, token_str, token, token_size);
        GetJsonVal(pJson, succ_str, status, sizeof(status));
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] status:%s \n", __FUNCTION__, __LINE__ , status);
        FreeJson( pJson );
        ret = 0;
    }
    return ret;
}

void RuntimeFeatureControlProcessor::cleanAllFile()
{
    DIR* dir = opendir("/opt/secure/RFC");
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            // Check if filename starts with ".RFC_"
            if (strncmp(entry->d_name, ".RFC_", 5) == 0)
            {
                std::string fullPath = std::string("/opt/secure/RFC/") + entry->d_name;
                unlink(fullPath.c_str());
            }
        }
        closedir(dir);
    }

    if (std::remove(VARIABLEFILE) == 0)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] File %s removed successfully\n", __FUNCTION__, __LINE__, VARIABLEFILE);
    }
}

int RuntimeFeatureControlProcessor::ProcessXconfUrl(const char *XconfUrl)
{
    int rc=FAILURE, retries=0;
    /* Before using CreateXconfHTTPUrl, _xconf_server_url variable must be set to required base url.
    Hence setting _xconf_server_url to XconfUrl & reverting back once execution is complete*/
    std::string FQDN = _xconf_server_url;
    _xconf_server_url = std::string(XconfUrl) + "/featureControl/getSettings"; 
    std::stringstream url = CreateXconfHTTPUrl();
    _xconf_server_url = FQDN;

    DownloadData DwnLoc, HeaderDwnLoc;
    InitDownloadData(&DwnLoc);
    InitDownloadData(&HeaderDwnLoc);

    if(allocDowndLoadDataMem( &DwnLoc, DEFAULT_DL_ALLOC ) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to allocate memory using malloc\n", __FUNCTION__, __LINE__ );
    } 
    if(allocDowndLoadDataMem( &HeaderDwnLoc, DEFAULT_DL_ALLOC ) != 0)
    {
        if( DwnLoc.pvOut != NULL )
        {
            free( DwnLoc.pvOut );
        }
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to allocate memory using malloc\n", __FUNCTION__, __LINE__ );
    }

    do{
        _url_validation_in_progress = true;
        rc = DownloadRuntimeFeatutres(&DwnLoc, &HeaderDwnLoc, url.str());
        if ( rc != SUCCESS)
        {
	    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Validation failed for Xconf URL %s\n", __FUNCTION__, __LINE__, url.str().c_str());
            if (++retries < 2) {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Retrying Validation for Xconf URL %s\n", __FUNCTION__, __LINE__, url.str().c_str());
                sleep(10);
            }
        }
	else{
	    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Xconf URL validation successful for url: %s\n", __FUNCTION__, __LINE__, url.str().c_str());
	    break;
	}
    }while( retries < 2);

    if( DwnLoc.pvOut != NULL )
    {
        free( DwnLoc.pvOut );
    }
    if( HeaderDwnLoc.pvOut != NULL )
    {
        free( HeaderDwnLoc.pvOut );
    }
    return rc;
}

#ifdef __cplusplus
}
#endif

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


#include <iostream>
#include <string>
#include <algorithm>
#include "rfc_common.h"
#include <unistd.h>
#include <rbus/rbus.h>


    std::string getSyseventValue(const std::string& key) {
        std::string cmd = "sysevent get " + key;
        std::string result;

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "";
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result = buffer;
            // Remove trailing newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }

        pclose(pipe);
        return result;
    }

    // Wait for RFC completion function
    void waitForRfcCompletion() {
        // Check RFC blob processing status
        std::string rfcBlobProcessing = getSyseventValue("rfc_blob_processing");

        // Check if file exists
        bool fileExists = false;
        std::ifstream f("/tmp/rfc_agent_initialized");
        if (f.good()) {
            fileExists = true;
            f.close();
        }

        // Main condition check
        if (!rfcBlobProcessing.empty() &&
            rfcBlobProcessing != "Completed" &&
            fileExists) {

            bool loop = true;
            int retry = 1;

            while (loop) {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Waiting rfc_blob_processing \n", __FUNCTION__,__LINE__);
                std::string rfcStatus = getSyseventValue("rfc_blob_processing");

                if (rfcStatus == "Completed") {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Webconfig rfc processing completed, breaking the loop \n", __FUNCTION__,__LINE__);
                    break;
                } else if (retry > 6) {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] webconfig rfc processing not completed after 10 min , breaking the loop \n", __FUNCTION__,__LINE__);
                    break;
                } else {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Webconfig rfc processing not completed.. Retry: %s \n", __FUNCTION__, __LINE__, std::to_string(retry).c_str());
                    retry++;
                    sleep(100); // Sleep for 100 seconds
                }
            }
        } else {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] rfc-agent process not running or rfc_blob_processing is empty. Not waiting rfc_blob_processing \n", __FUNCTION__,__LINE__);
        }
    }



/* Description: Reading rfc data
 * @param type : rfc type
 * @param key: rfc key
 * @param data : Store rfc value
 * @return int 1 READ_RFC_SUCCESS on success and READ_RFC_FAILURE -1 on failure
 * */
int read_RFCProperty(const char* type, const char* key, char *out_value, int datasize)
{
    int ret = READ_RFC_FAILURE;

    if(key == nullptr || out_value == nullptr || datasize == 0)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] read_RFCProperty() one or more input values are invalid\n", __FUNCTION__, __LINE__);
        return ret;
    }

#if defined(RDKB_SUPPORT)
    rbusHandle_t handle;
    rbusValue_t value;
    rbusError_t rc;

    // Just use the key directly without any modification
    const char* paramName = key;
    (void)type;

    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Looking up parameter: %s\n", __FUNCTION__, __LINE__, paramName);

    // Initialize rbus connection
    rc = rbus_open(&handle, "RFC_Manager");
    if (rc != RBUS_ERROR_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open rbus handle: %s\n", __FUNCTION__, __LINE__, rbusError_ToString(rc));
        return ret;
    }

    // Get the parameter value
    rc = rbus_get(handle, paramName, &value);
    if (rc == RBUS_ERROR_SUCCESS) {
        // Process the retrieved value
        if (rbusValue_GetType(value) == RBUS_STRING) {
            const char* strValue = rbusValue_GetString(value, NULL);

            if (strValue) {
                int data_len = strlen(strValue);

                // Check if value is quoted and remove quotes if needed
                if (data_len >= 2 && (strValue[0] == '"') && (strValue[data_len - 1] == '"')) {
                    // Remove quotes around data
                    int copyLen = (data_len - 2 < datasize - 1) ? data_len - 2 : datasize - 1;
                    strncpy(out_value, strValue + 1, copyLen);
                    out_value[copyLen] = '\0';
                } else {
                    // Copy the value directly
                    int copyLen = (data_len < datasize - 1) ? data_len : datasize - 1;
                    strncpy(out_value, strValue, copyLen);
                    out_value[copyLen] = '\0';
                }

                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] read_RFCProperty() param=%s, value=%s\n",  __FUNCTION__, __LINE__, paramName, out_value);
                ret = READ_RFC_SUCCESS;
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Null string value for param=%s\n",  __FUNCTION__, __LINE__, paramName);
                *out_value = '\0';
            }
        } else if (rbusValue_GetType(value) == RBUS_BOOLEAN) {
            // Handle boolean type
            bool boolValue = rbusValue_GetBoolean(value);
            snprintf(out_value, datasize, "%s", boolValue ? "true" : "false");

            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] read_RFCProperty() param=%s, value=%s (boolean)\n", __FUNCTION__, __LINE__, paramName, out_value);
            ret = READ_RFC_SUCCESS;
        } else if (rbusValue_GetType(value) == RBUS_INT32) {
            // Handle integer type
            int32_t intValue = rbusValue_GetInt32(value);
            snprintf(out_value, datasize, "%d", intValue);

            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] read_RFCProperty() param=%s, value=%s (int)\n", __FUNCTION__, __LINE__, paramName, out_value);
            ret = READ_RFC_SUCCESS;
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Unsupported value type for param=%s\n", __FUNCTION__, __LINE__, paramName);
            *out_value = '\0';
        }

        // Free the value
        rbusValue_Release(value);
    } else {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get parameter: %s, error: %s\n", __FUNCTION__, __LINE__, paramName, rbusError_ToString(rc));
        *out_value = '\0';
    }

    // Close the rbus connection
    rbus_close(handle);
#else
    // Keep the original non-broadband implementation
    RFC_ParamData_t param;
    memset(&param, 0, sizeof(RFC_ParamData_t));

    int data_len;
    WDMP_STATUS status = getRFCParameter(type, key, &param);
    if(status == WDMP_SUCCESS || status == WDMP_ERR_DEFAULT_VALUE)
    {
        data_len = strlen(param.value);
        if(data_len >= 2 && (param.value[0] == '"') && (param.value[data_len - 1] == '"'))
        {
            // remove quotes around data
            snprintf(out_value, datasize, "%s", &param.value[1]);
            *(out_value + data_len - 2) = 0;
        }
        else
        {
            snprintf(out_value, datasize, "%s", param.value);
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] read_RFCProperty() name=%s,type=%d,value=%s,status=%d\n",
                __FUNCTION__, __LINE__, param.name, param.type, param.value, status);
        ret = READ_RFC_SUCCESS;
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] error:read_RFCProperty(): status= %d\n",
                __FUNCTION__, __LINE__, status);
        *out_value = 0;
    }
#endif

    return ret;
}

int executeCommandAndGetOutput(SYSCMD eSysCmd,  const char *pArgs, std::string& result) 
{
    constexpr size_t initialBufferSize = 256;
    std::vector<char> buffer(initialBufferSize);
    int ret_val = FAILURE;
    FILE *fp = nullptr;

    switch( eSysCmd )
    {
        case eRdkSsaCli :
           if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", RDKSSACLI_CMD, pArgs );
               }
               else
               {
                   fp = NULL;
                   RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] %s requires an input argument\n", __FUNCTION__, __LINE__, RDKSSACLI_CMD );
               }
               break;

    case eWpeFrameworkSecurityUtility :
           fp = v_secure_popen( "r", WPEFRAMEWORKSECURITYUTILITY );
           break;
    }
    if (!fp) 
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] popen failed",__FUNCTION__, __LINE__);
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] popen success",__FUNCTION__,__LINE__);
        while (true) 
        {
            if (fgets(buffer.data(), buffer.size(), fp) == nullptr) 
            { 
                break; // No more data 
            }

            result += buffer.data();

            // Resize buffer if necessary
            if (result.size() >= buffer.size() - 1) 
            {
                buffer.resize(buffer.size() * 2); // Double the buffer size
            }
        }
        v_secure_pclose(fp);
        ret_val = SUCCESS;
    }

    return ret_val;
}

bool CheckSpecialCharacters(const std::string& str) {
    for (char c : str)
    {
        if(!std::isalnum(c))
        {
            return true; // Return true if a non-alphanumeric character is found
        }
    }
    return false; // Return false if no non-alphanumeric characters are found
}

bool StringCaseCompare(const std::string& str1, const std::string& str2) 
{
    return str1.size() == str2.size() &&
           std::equal(str1.begin(), str1.end(), str2.begin(),
           [](char a, char b){ return std::tolower(a) == std::tolower(b); });
}

void RemoveSubstring(std::string& str, const std::string& toRemove) 
{
    size_t pos = str.find(toRemove);

    while (pos != std::string::npos) {
        str.erase(pos, toRemove.length());
        pos = str.find(toRemove);
    }
}
std::string getCronFromDCMSettings() {
    std::string cron = "";
    std::string dcmSettingsPath = "/tmp/DCMSettings.conf";
    std::string persistentPath = std::getenv("PERSISTENT_PATH") ? std::getenv("PERSISTENT_PATH") : "";
    std::string tmpDCMSettingsPath = persistentPath + "/tmp/DCMSettings.conf";
    std::string searchString = "urn:settings:CheckSchedule:cron";

    // Check if DCMSettings.conf exists
    std::ifstream dcmFile(dcmSettingsPath);
    if (dcmFile.is_open()) {
        std::string line;
        std::ofstream tmpFile(tmpDCMSettingsPath);

        // Search for the cron setting in DCMSettings.conf
        while (std::getline(dcmFile, line)) {
            if (line.find(searchString) != std::string::npos) {
                // Write the line to the temporary file
                if (tmpFile.is_open()) {
                    tmpFile << line << std::endl;
                }

                // Extract the cron value
                size_t pos = line.find('=');
                if (pos != std::string::npos && pos + 1 < line.length()) {
                    cron = line.substr(pos + 1);
                }

                break; // Found what we're looking for
            }
        }

        dcmFile.close();
        if (tmpFile.is_open()) {
            tmpFile.close();
        }
    } else {
        // If DCMSettings.conf doesn't exist, try reading from the temp file
        std::ifstream tmpFile(tmpDCMSettingsPath);
        if (tmpFile.is_open()) {
            std::string line;
            while (std::getline(tmpFile, line)) {
                if (line.find(searchString) != std::string::npos) {
                    // Extract the cron value
                    size_t pos = line.find('=');
                    if (pos != std::string::npos && pos + 1 < line.length()) {
                        cron = line.substr(pos + 1);
                    }
                    break;
                }
            }
            tmpFile.close();
        }
    }

    return cron;
}


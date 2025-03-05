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

/* Description: Reading rfc data
 * @param type : rfc type
 * @param key: rfc key
 * @param data : Store rfc value
 * @return int 1 READ_RFC_SUCCESS on success and READ_RFC_FAILURE -1 on failure
 * */
int read_RFCProperty(const char* type, const char* key, char *out_value, int datasize) 
{
    RFC_ParamData_t param;
    memset(&param, 0, sizeof(RFC_ParamData_t));
    int ret = READ_RFC_FAILURE;


    if(key == nullptr || out_value == nullptr || datasize == 0 || type == nullptr) 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "read_RFCProperty() one or more input values are invalid\n");
        return ret;
    }
    //SWLOG_INFO("key=%s\n", key);
#if !defined(RDKB_SUPPORT)    
    int data_len;
    WDMP_STATUS status = getRFCParameter(type, key, &param);
    if(status == WDMP_SUCCESS || status == WDMP_ERR_DEFAULT_VALUE) 
    {
        data_len = strlen(param.value);
        if(data_len >= 2 && (param.value[0] == '"') && (param.value[data_len - 1] == '"')) 
        {
            // remove quotes arround data
            snprintf( out_value, datasize, "%s", &param.value[1] );
            *(out_value + data_len - 2) = 0;
        }
        else 
        {
            snprintf( out_value, datasize, "%s", param.value );
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "read_RFCProperty() name=%s,type=%d,value=%s,status=%d\n", param.name, param.type, param.value, status);
        ret = READ_RFC_SUCCESS;
    }
    else 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "error:read_RFCProperty(): status= %d\n", status);
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

rfc::DeviceType rfc::GetDeviceType() 
{
    std::string deviceType;
    std::ifstream deviceProps("/etc/device.properties");

    if (deviceProps.is_open()) {
        std::string line;
        while (std::getline(deviceProps, line)) 
	{
            if (line.find("DEVICE_TYPE=") == 0) 
	    {
                deviceType = line.substr(12);
                break;
            }
            else if (line.find("\"$DEVICE_TYPE\"") != std::string::npos) 
	    {
                size_t pos = line.find("=");
                if (pos != std::string::npos) {
                    size_t start = line.find("\"", pos);
                    if (start != std::string::npos) {
                        size_t end = line.find("\"", start + 1);
                        if (end != std::string::npos) {
                            deviceType = line.substr(start + 1, end - start - 1);
                            break;
                        }
                    }
                }
            }
        }
        deviceProps.close();
    }

    deviceType.erase(deviceType.begin(), std::find_if(deviceType.begin(), deviceType.end(),  { return !std::isspace(ch); }));
    deviceType.erase(std::find_if(deviceType.rbegin(), deviceType.rend(),  { return !std::isspace(ch); }).base(), deviceType.end());

    if (deviceType == "broadband") 
    {
        return DEVICE_TYPE_BROADBAND;
    }
    else if (deviceType == "XHC1") {
        return DEVICE_TYPE_CAMERA;
    }
    else 
    {
        return DEVICE_TYPE_VIDEO;
    }
}

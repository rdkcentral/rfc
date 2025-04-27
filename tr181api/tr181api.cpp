/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "tr181api.h"
#include <wdmp-c.h>
#include "rfcapi.h"
#include "semaphore.h"
#include <fcntl.h>
#include <unistd.h>
#include "rdk_debug.h"
#include <fstream>
#include <unordered_map>

#define TR181_CLEAR_PARAM "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.ClearParam"

#ifdef USE_NONSECURE_TR181_LOCALSTORE
   #define TR181_LOCAL_STORE_FILE "/opt/persistent/tr181localstore.ini"
#else
   #define TR181_LOCAL_STORE_FILE "/opt/secure/RFC/tr181localstore.ini"
#endif

#define RFCDEFAULTS_ETC_DIR "/etc/rfcdefaults/"
#define LOG_TR181API  "LOG.RDK.TR181API"
using namespace std;
const char *semName = "localstore";

#ifdef TR181API_LOGGING
static ofstream logofs;

static void openLogFile()
{
   if (!logofs.is_open())
      logofs.open("/opt/secure/RFC/tr181api.log", ios_base::app);
}

static string prefix()
{
    time_t timer;
    char buffer[50];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 50, "rfcapi:%Y-%m-%d %H:%M:%S ", tm_info);
    return string(buffer);
}
#endif

TR181_PARAM_TYPE getType(DATA_TYPE type)
{
   switch(type)
   {
      case WDMP_STRING: return TR181_STRING;
      case WDMP_INT: return TR181_INT;
      case WDMP_UINT: return TR181_UINT;
      case WDMP_BOOLEAN: return TR181_BOOLEAN;
      case WDMP_DATETIME: return TR181_DATETIME;
      case WDMP_BASE64: return TR181_BASE64;
      case WDMP_LONG: return TR181_LONG;
      case WDMP_ULONG: return TR181_ULONG;
      case WDMP_FLOAT: return TR181_FLOAT;
      case WDMP_DOUBLE: return TR181_DOUBLE;
      case WDMP_BYTE: return TR181_BYTE;
      case WDMP_NONE:
      default: return TR181_NONE;
   }
}

tr181ErrorCode_t getErrorCode(WDMP_STATUS status)
{
    switch(status)
    {
        case WDMP_SUCCESS: return tr181Success;
        case WDMP_FAILURE: return tr181Failure;
        case WDMP_ERR_TIMEOUT: return tr181Timeout;
        case WDMP_ERR_INVALID_PARAMETER_NAME: return tr181InvalidParameterName;
        case WDMP_ERR_INVALID_PARAMETER_VALUE: return tr181InvalidParameterValue;
        case WDMP_ERR_INVALID_PARAMETER_TYPE: return tr181InvalidType;
        case WDMP_ERR_NOT_WRITABLE: return tr181NotWritable;
        case WDMP_ERR_VALUE_IS_EMPTY: return tr181ValueIsEmpty;
        case WDMP_ERR_VALUE_IS_NULL: return tr181ValueIsNull;
        case WDMP_ERR_DEFAULT_VALUE: return tr181DefaultValue;
        case WDMP_ERR_INTERNAL_ERROR:
        default: return tr181InternalError;
    }
}

tr181ErrorCode_t getParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData)
{
   RFC_ParamData_t param;
   WDMP_STATUS wdmpStatus = getRFCParameter(pcCallerID, pcParameterName, &param);
   if (wdmpStatus == WDMP_SUCCESS || wdmpStatus == WDMP_ERR_DEFAULT_VALUE)
   {
      pstParamData->type = getType(param.type);
      strncpy(pstParamData->value, param.value, MAX_PARAM_LEN);
      pstParamData->value[MAX_PARAM_LEN - 1] = '\0';
      return tr181Success;
   }

   return getErrorCode(wdmpStatus);
}

tr181ErrorCode_t setParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue)
{
   WDMP_STATUS wdmpStatus = setRFCParameter(pcCallerID, pcParameterName, pcParameterValue, WDMP_STRING);
   if (wdmpStatus == WDMP_SUCCESS)
      return tr181Success;

   return getErrorCode(wdmpStatus);
}

tr181ErrorCode_t clearParam(char *pcCallerID, const char* pcParameterName)
{
   WDMP_STATUS wdmpStatus = setRFCParameter(pcCallerID, TR181_CLEAR_PARAM, pcParameterName, WDMP_STRING);
   if (wdmpStatus == WDMP_SUCCESS)
      return tr181Success;

   return getErrorCode(wdmpStatus);
}

const char * getTR181ErrorString(tr181ErrorCode_t code)
{
      const char * err_string;
   switch(code)
   {
      case tr181Success:
         err_string = " Success";
            break;
      case tr181InternalError:
         err_string = " Internal Error";
            break;
      case tr181InvalidParameterName:
         err_string = " Invalid Parameter Name";
            break;
      case tr181InvalidParameterValue:
         err_string = " Invalid Parameter Value";
            break;
      case tr181Failure:
         err_string = " Failure";
            break;
      case tr181InvalidType:
         err_string = " Invalid type";
            break;
      case tr181NotWritable:
         err_string = " Not writable";
            break;
      case tr181ValueIsEmpty:
         err_string = " Value is empty";
            break;
      case tr181ValueIsNull:
         err_string = " Value is Null";
            break;
      case tr181DefaultValue:
         err_string = " Default Value";
            break;
      default:
         err_string = " Unknown error code";
   }
   return err_string;
}

tr181ErrorCode_t getValue(const char* fileName, const char* pcParameterName, TR181_ParamData_t *pstParam)
{
    ifstream ifs_rfcVar(fileName);
    if (!ifs_rfcVar.is_open())
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_TR181API, "%s: Trying to open a non-existent file %s \n", __FUNCTION__, fileName);
        return tr181Failure;
    }
    else
    {
        string line;
        while (getline(ifs_rfcVar, line))
        {
            size_t splitterPos = line.find('=');
            if (splitterPos < line.length())
            {
                string key = line.substr(0, splitterPos);
                if ( !key.compare(pcParameterName) )
                {
                   ifs_rfcVar.close();
                   string value = line.substr(splitterPos+1, line.length());
                   RDK_LOG(RDK_LOG_DEBUG, LOG_TR181API, "Found Key = %s : Value = %s\n", key.c_str(), value.c_str());
                   if(value.length() > 0)
                   {
                      pstParam->type = TR181_NONE; //The caller must know what type they are expecting

                      strncpy(pstParam->value, value.c_str(), MAX_PARAM_LEN);
                      pstParam->value[MAX_PARAM_LEN - 1] = '\0';
                      return tr181Success;
                   }
                   return tr181ValueIsEmpty;
                }
            }
        }
        ifs_rfcVar.close();
    }
    return tr181Failure;
}

tr181ErrorCode_t getDefaultValue(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData)
{
   if (pcCallerID == NULL )
   {
       RDK_LOG (RDK_LOG_ERROR, LOG_TR181API, "%s: pcCallerID is NULL\n", __FUNCTION__);
       return tr181Failure;
   }
   char defaultsFilename[256] = RFCDEFAULTS_ETC_DIR;
   strncat(defaultsFilename, pcCallerID, sizeof(defaultsFilename) - strlen(RFCDEFAULTS_ETC_DIR) - 5);
   strncat(defaultsFilename, ".ini", sizeof(defaultsFilename) - strlen(defaultsFilename) - 1);
   return getValue(defaultsFilename, pcParameterName, pstParamData);
}

tr181ErrorCode_t setValue(const char* pcParameterName, const char* pcParamValue)
{
    string key(pcParameterName), value(pcParamValue);

    std::unordered_map<std::string, std::string> m_dict;

#ifdef TR181API_LOGGING
   openLogFile();

    logofs << prefix() << "paramName=" << key << " value=" << value << "\n";
    std::ifstream f1(TR181_LOCAL_STORE_FILE);
    if (f1.is_open())
        logofs << prefix() << "file content before write:\n" << f1.rdbuf();
#endif

    ifstream ifs_tr181(TR181_LOCAL_STORE_FILE);
    if (!ifs_tr181.is_open()) {
        RDK_LOG (RDK_LOG_INFO, LOG_TR181API, "%s: Trying to open a non-existent file [%s] \n", __FUNCTION__, TR181_LOCAL_STORE_FILE);
    }
    else
    {
        string line;
        while (getline(ifs_tr181, line)) {
            size_t splitterPos = line.find('=');
            if (splitterPos < line.length()) {
                string key = line.substr(0, splitterPos);
                string value = line.substr(splitterPos+1, line.length());
                m_dict[key] = value;
                RDK_LOG(RDK_LOG_DEBUG, LOG_TR181API, "Key = %s : Value = %s\n", key.c_str(), value.c_str());
            }
        }
        ifs_tr181.close();
    }

    if (!key.compare(TR181_CLEAR_PARAM))
    {
        bool foundInLocalStore = false;
        unordered_map<string, string> temp_local_dict(m_dict);

        for (unordered_map<string, string>::iterator it=temp_local_dict.begin(); it!=temp_local_dict.end(); ++it)
        {
            //Check if the param to be cleared is present in tr181localstore.ini
            //(Or) if the param to be cleared is a wild card, clear all the params from local store that match with the wild card.
            if(!it->first.compare(value) ||  (value.back() == '.' && it->first.find(value) != string::npos) )
            {
                RDK_LOG(RDK_LOG_INFO, LOG_TR181API, "Clearing param: %s\n", it->first.c_str());
                m_dict.erase(it->first);
                foundInLocalStore = true;
            }
        }
        if (!foundInLocalStore)
        {
            RDK_LOG(RDK_LOG_INFO, LOG_TR181API, "Key %s not present. Nothing to clear.\n", value.c_str());
            return tr181Success;
        }
    }
    else
    {
        m_dict[key] = std::move(value);
    }

    FILE *f = fopen(TR181_LOCAL_STORE_FILE, "w");
    if (f == NULL)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_TR181API, "Failed to open : %s \n", TR181_LOCAL_STORE_FILE);
        return tr181Failure;
    }

    for (unordered_map<string, string>::iterator it=m_dict.begin(); it!=m_dict.end(); ++it)
    {
        fprintf(f, "%s=%s\n", it->first.c_str(), it->second.c_str());
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);

#ifdef TR181API_LOGGING
    std::ifstream f2(TR181_LOCAL_STORE_FILE);
    if (f2.is_open())
        logofs << prefix() << "file content after write:\n" << f2.rdbuf()  << "\n";

    logofs.flush();
    logofs.close();
#endif

    return tr181Success;
}

tr181ErrorCode_t getLocalParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData)
{
    tr181ErrorCode_t status = tr181Failure;
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 1);
    if (sem_id == SEM_FAILED){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_open failed \n", __FUNCTION__);
        return tr181Failure;
    }

    if (sem_wait(sem_id) < 0)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_wait failed \n", __FUNCTION__);
    }

    status = getValue(TR181_LOCAL_STORE_FILE, pcParameterName, pstParamData);
    if (status != tr181Success)
        status = getDefaultValue(pcCallerID, pcParameterName, pstParamData);

    if (sem_post(sem_id) < 0)
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_post failed \n", __FUNCTION__);

    if (sem_close(sem_id) != 0){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_close failed \n", __FUNCTION__);
    }
   return status;
}

tr181ErrorCode_t setLocalParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue)
{
    tr181ErrorCode_t status = tr181Failure;
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 1);
    if (sem_id == SEM_FAILED){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_open failed \n", __FUNCTION__);
        return tr181Failure;
    }

    if (sem_wait(sem_id) < 0)
    { 
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_wait failed \n", __FUNCTION__);
    }

    status = setValue(pcParameterName, pcParameterValue);

    if (sem_post(sem_id) < 0)
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_post failed \n", __FUNCTION__);

    if (sem_close(sem_id) != 0){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_close failed \n", __FUNCTION__);
    }
   return status;
}

tr181ErrorCode_t clearLocalParam(char *pcCallerID, const char* pcParameterName)
{
    tr181ErrorCode_t status = tr181Failure;
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 1);
    if (sem_id == SEM_FAILED){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_open failed \n", __FUNCTION__);
        return tr181Failure;
    }

    if (sem_wait(sem_id) < 0)
    { 
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_wait failed \n", __FUNCTION__);
    }

    status = setValue(TR181_CLEAR_PARAM, pcParameterName);

    if (sem_post(sem_id) < 0)
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_post failed \n", __FUNCTION__);

    if (sem_close(sem_id) != 0){
        RDK_LOG(RDK_LOG_ERROR, LOG_TR181API, "%s: sem_close failed \n", __FUNCTION__);
    }
   return status;
}


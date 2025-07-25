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

#ifndef TR181API_H_
#define TR181API_H_

#include <string.h>
#include <stdlib.h>

#if defined(GTEST_ENABLE)
#include <wdmp-c.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_PARAM_LEN     (2*1024)

typedef enum
{   
    TR181_STRING = 0,
    TR181_INT,
    TR181_UINT,
    TR181_BOOLEAN,
    TR181_DATETIME,
    TR181_BASE64,
    TR181_LONG,
    TR181_ULONG,
    TR181_FLOAT,
    TR181_DOUBLE,
    TR181_BYTE,
    TR181_NONE,
    TR181_BLOB
} TR181_PARAM_TYPE;

typedef struct _TR181_Param_t {
   char value[MAX_PARAM_LEN];
   TR181_PARAM_TYPE type;
} TR181_ParamData_t;

/*Error Code type*/
typedef enum _tr181ErrorCodes
{
    tr181Success = 0,
    tr181Failure,
    tr181Timeout,
    tr181InvalidParameterName,
    tr181InvalidParameterValue,
    tr181InvalidType,
    tr181NotWritable,
    tr181ValueIsEmpty,
    tr181ValueIsNull,
    tr181InternalError,
    tr181DefaultValue,
} tr181ErrorCode_t;

//NOTE: The pcCallerID is the component name. This name should match the name of the defaults ini file if the component is using a defaults ini file.
//      For example authservice comonent uses defaults file "authservice.ini". For this case the pcCallerID should be "authservice"
tr181ErrorCode_t getParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData);
tr181ErrorCode_t setParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue);

//NOTE: To clear whole domain/feature, pass the wild card parameter to pcParameterName. eg. clearParam("sysint", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TelemetryEndpoint.");
tr181ErrorCode_t clearParam(char *pcCallerID, const char* pcParameterName);
const char * getTR181ErrorString(tr181ErrorCode_t code);

tr181ErrorCode_t getLocalParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData);
tr181ErrorCode_t setLocalParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue);

//NOTE: To clear whole domain/feature, pass the wild card parameter to pcParameterName. eg. clearParam("sysint", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TelemetryEndpoint.");
tr181ErrorCode_t clearLocalParam(char *pcCallerID, const char* pcParameterName);

tr181ErrorCode_t getDefaultValue(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData);
#if defined(GTEST_ENABLE)
tr181ErrorCode_t setValue(const char* pcParameterName, const char* pcParamValue);
tr181ErrorCode_t getValue(const char* fileName, const char* pcParameterName, TR181_ParamData_t *pstParam);
TR181_PARAM_TYPE getType(DATA_TYPE type);
tr181ErrorCode_t getErrorCode(WDMP_STATUS status);
#endif

#ifdef __cplusplus
}
#endif

#endif

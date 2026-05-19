/**
 * @file rfcapi.h
 * @brief Public C API for reading and writing RFC parameters.
 *
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

#ifndef RFCAPI_H_
#define RFCAPI_H_

#include <string.h>
#include <stdlib.h>

#define RFCVAR_FILE "/opt/secure/RFC/rfcVariable.ini"   /**< RFC shell-variable store. */
#define TR181STORE_FILE "/opt/secure/RFC/tr181store.ini" /**< TR181 parameter override store. */

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(RDKB_SUPPORT) && !defined(RDKC)
#include <wdmp-c/wdmp-c.h>
#endif

#define MAX_PARAM_LEN     (2*1024)  /**< Maximum length of a parameter name or value. */

#if defined(RDKB_SUPPORT) || defined(RDKC)
/**
 * @brief Return-type / value-status codes (RDKB & RDKC path).
 */
typedef enum
{
  SUCCESS=0,   /**< Operation succeeded. */
  FAILURE,     /**< Operation failed. */
  NONE,        /**< No type information available. */
  EMPTY        /**< Parameter value is empty. */
}DATA_TYPE;
#endif

/**
 * @struct _RFC_Param_t
 * @brief Container for a single RFC parameter (name + value + type).
 */
#if defined(USE_IARMBUS)
typedef struct _RFC_Param_t {
   char name[MAX_PARAM_LEN];   /**< Parameter name. */
   char value[MAX_PARAM_LEN];  /**< Parameter value. */
   DATA_TYPE type;              /**< WDMP data type. */
} RFC_ParamData_t;
#else
typedef struct _RFC_Param_t {
   char name[MAX_PARAM_LEN];   /**< Parameter name. */
   char value[MAX_PARAM_LEN];  /**< Parameter value. */
   DATA_TYPE type;              /**< WDMP data type. */
} RFC_ParamData_t;
#endif

#if defined(RDKB_SUPPORT) || defined(RDKC)
/**
 * @brief Retrieve an RFC parameter value (RDKB / RDKC — int return path).
 * @param[in]  pcParameterName  TR181 parameter name.
 * @param[out] pstParamData     Filled with the retrieved name/value/type.
 * @return SUCCESS (0) or FAILURE (1).
 */
int getRFCParameter(const char* pcParameterName, RFC_ParamData_t *pstParamData);
#else
/**
 * @brief Retrieve an RFC parameter value (STB — WDMP_STATUS return path).
 * @param[in]  pcCallerID       Caller identifier string.
 * @param[in]  pcParameterName  TR181 parameter name.
 * @param[out] pstParamData     Filled with the retrieved name/value/type.
 * @return WDMP_STATUS code.
 */
WDMP_STATUS getRFCParameter(const char *pcCallerID, const char* pcParameterName, RFC_ParamData_t *pstParamData);

/**
 * @brief Set an RFC parameter value via hostif.
 * @param[in] pcCallerID        Caller identifier string.
 * @param[in] pcParameterName   TR181 parameter name.
 * @param[in] pcParameterValue  New value to set.
 * @param[in] eDataType         WDMP data type of the value.
 * @return WDMP_STATUS code.
 */
WDMP_STATUS setRFCParameter(const char *pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType);

/**
 * @brief Return a human-readable error string for a WDMP status code.
 * @param[in] code  WDMP status code.
 * @return Static error description string.
 */
const char* getRFCErrorString(WDMP_STATUS code);

/**
 * @brief Check whether a named RFC feature is enabled.
 * @param[in] feature  Feature name (without "RFC_" prefix).
 * @retval true   Feature ini marker file exists.
 * @retval false  Feature not enabled.
 */
bool isRFCEnabled(const char *);

/**
 * @brief Check whether a file exists in a given directory.
 * @param[in] dir       Directory path to search.
 * @param[in] filename  Filename to look for.
 * @retval true   File found.
 * @retval false  File not found.
 */
bool isFileInDirectory(const char *, const char *);

#if defined(GTEST_ENABLE)
/**
 * @brief Merge per-feature rfcdefaults ini files into a single file.
 * @retval true   Merge succeeded.
 * @retval false  /etc/rfcdefaults/ could not be opened.
 */
bool init_rfcdefaults();
#endif
#endif
#ifdef __cplusplus
}
#endif

#endif

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

#include <fstream>
#include <sstream>
#ifndef RDKC
#include <curl/curl.h>
#include "cJSON.h"
#endif
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "rfcapi.h"
#include "rdk_debug.h"
using namespace std;

#define LOG_RFCAPI  "LOG.RDK.RFCAPI"
#define TR181_RFC_PREFIX   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC"
#define BOOTSTRAP_FILE "/opt/secure/RFC/bootstrap.ini"
#define RFCDEFAULTS_FILE "/tmp/rfcdefaults.ini"
#define RFCDEFAULTS_ETC_DIR "/etc/rfcdefaults/"
#define RFC_FEATURE_DIR "/opt/secure/RFC/"


#define CONNECTION_TIMEOUT 5
#define TRANSFER_TIMEOUT 10

static const char *url = "http://127.0.0.1:11999";
static bool tr69hostif_http_server_ready = false;

#ifdef TEMP_LOGGING
static ofstream logofs;

static void openLogFile()
{
   if (!logofs.is_open())
      logofs.open("/opt/logs/rfcscript.log", ios_base::app); 
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

bool init_rfcdefaults()
{
   DIR *dir;
   struct dirent *ent;
   if ((dir = opendir ( RFCDEFAULTS_ETC_DIR )) != NULL)
   {
      std::ofstream combined_file( RFCDEFAULTS_FILE ) ;
      while ((ent = readdir (dir)) != NULL )
      {
         if (strstr(ent->d_name, ".ini"))
         {
            RDK_LOG (RDK_LOG_DEBUG, LOG_RFCAPI,"rfcdefaults file: %s\n", ent->d_name);
            string filepath = RFCDEFAULTS_ETC_DIR;
            std::ifstream file1( filepath.append(ent->d_name) ) ;
            combined_file << file1.rdbuf();
	    combined_file << "\n" ;
         }
      }
      closedir (dir);
   }
   else
   {
      RDK_LOG (RDK_LOG_ERROR, LOG_RFCAPI,"Could not open dir %s \n", RFCDEFAULTS_ETC_DIR) ;
      return false;
   }
   return true;
}

#ifndef RDKC
WDMP_STATUS getValue(const char* fileName, const char* pcParameterName, RFC_ParamData_t *pstParam)
{
    ifstream ifs_rfcVar(fileName);
    if (!ifs_rfcVar.is_open())
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_RFCAPI, "%s: Trying to open a non-existent file %s \n", __FUNCTION__, fileName);
        if ( strcmp(fileName, RFCDEFAULTS_FILE) == 0 && init_rfcdefaults() )
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI, "Trying to open %s after newly creating\n", RFCDEFAULTS_FILE);
            ifs_rfcVar.open(RFCDEFAULTS_FILE, ifstream::in);
            if (!ifs_rfcVar.is_open())
                return WDMP_FAILURE;
        }
        else
            return WDMP_FAILURE;
    }
    {
        string line;
        while (getline(ifs_rfcVar, line))
        {
            line=line.substr(line.find_first_of(" \t")+1);//Remove any export word that maybe before the key(for rfcVariable.ini)
            size_t splitterPos = line.find('=');
            if (splitterPos < line.length())
            {
                string key = line.substr(0, splitterPos);
                if ( !key.compare(pcParameterName) )
                {
                   ifs_rfcVar.close();
                   string value = line.substr(splitterPos+1, line.length());
                   RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI, "Found Key = %s : Value = %s\n", key.c_str(), value.c_str());
                   if(value.length() > 0)
                   {
                      strncpy(pstParam->name, pcParameterName, MAX_PARAM_LEN);
                      pstParam->name[MAX_PARAM_LEN - 1] = '\0';

                      pstParam->type = WDMP_NONE; //The caller must know what type they are expecting if they are requesting a param before the hostif is ready.

                      strncpy(pstParam->value, value.c_str(), MAX_PARAM_LEN);
                      pstParam->value[MAX_PARAM_LEN - 1] = '\0';
                      return WDMP_SUCCESS;
                   }
                   return WDMP_ERR_VALUE_IS_EMPTY;
                }
            }
        }
        ifs_rfcVar.close();
    }
    return WDMP_FAILURE;
}
#endif

static size_t writeCurlResponse(void *ptr, size_t size, size_t nmemb, string stream)
{
   size_t realsize = size * nmemb;
   string temp(static_cast<const char*>(ptr), realsize);
   stream.append(temp);
   return realsize;
}

#ifdef RDKC

int getValue(const char* fileName, const char* pcParameterName, RFC_ParamData_t *pstParam)
{
    ifstream ifs_rfcVar(fileName);
    if (!ifs_rfcVar.is_open())
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_RFCAPI, "%s: Trying to open a non-existent file %s \n", __FUNCTION__, fileName);
        if ( strcmp(fileName, RFCDEFAULTS_FILE) == 0 && init_rfcdefaults() )
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI, "Trying to open %s after newly creating\n", RFCDEFAULTS_FILE);
            ifs_rfcVar.open(RFCDEFAULTS_FILE, ifstream::in);
            if (!ifs_rfcVar.is_open())
                return FAILURE;
        }
        else
            return FAILURE;
    }
    {
        string line;
        while (getline(ifs_rfcVar, line))
        {
            line=line.substr(line.find_first_of(" \t")+1);//Remove any export word that maybe before the key(for rfcVariable.ini)
            size_t splitterPos = line.find('=');
            if (splitterPos < line.length())
            {
                string key = line.substr(0, splitterPos);
                if ( !key.compare(pcParameterName) )
                {
                   ifs_rfcVar.close();
                   string value = line.substr(splitterPos+1, line.length());
                   RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI, "Found Key = %s : Value = %s\n", key.c_str(), value.c_str());
                   if(value.length() > 0)
                   {
                      strncpy(pstParam->name, pcParameterName, MAX_PARAM_LEN);
                      pstParam->name[MAX_PARAM_LEN - 1] = '\0';
                      pstParam->type = NONE; //The caller must know what type they are expecting if they are requesting a param before the hostif is ready.

                      strncpy(pstParam->value, value.c_str(), MAX_PARAM_LEN);
                      pstParam->value[MAX_PARAM_LEN - 1] = '\0';
                      return SUCCESS;
                   }
                   return EMPTY;
                }
            }
        }
        ifs_rfcVar.close();
    }
    return FAILURE;
}


int getRFCParameter(const char* pcParameterName, RFC_ParamData_t *pstParam)
{
 int ret = FAILURE;
 if(!strcmp(pcParameterName+strlen(pcParameterName)-1,"."))
 {
   RDK_LOG (RDK_LOG_DEBUG, LOG_RFCAPI, "%s: RFC API doesn't support wildcard parameterName\n", __FUNCTION__);
 }

 if(strncmp(pcParameterName, "RFC_", 4) == 0 && strchr(pcParameterName, '.') == NULL) 
 {
  return getValue(RFCVAR_FILE, pcParameterName, pstParam);
 }

 else
 {
    ret = getValue(TR181STORE_FILE, pcParameterName, pstParam);
    if (SUCCESS == ret)
      return SUCCESS;

    // If the param is not found in override files, find it in rfcdefaults.
    return getValue(RFCDEFAULTS_FILE, pcParameterName, pstParam);

 }
}

#else

WDMP_STATUS getRFCParameter_ex(const char *pcCallerID, const char* pcParameterName, RFC_ParamData_t *pstParam, const char* file, int line)
//WDMP_STATUS getRFCParameter(const char *pcCallerID, const char* pcParameterName, RFC_ParamData_t *pstParam)
{
#ifdef TEMP_LOGGING
    openLogFile();
    logofs << prefix() << "getRFCParameter called from " << file << ":" << line << endl;
#endif
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "getRFCParameter called from %s:%d\n", file, line);

   WDMP_STATUS ret = WDMP_FAILURE;
   long http_code = 0;
   string response;
   CURL *curl_handle = NULL;
   CURLcode res = CURLE_FAILED_INIT;

   if(!strcmp(pcParameterName+strlen(pcParameterName)-1,"."))
   {
#ifdef TEMP_LOGGING
       logofs << prefix() << __FUNCTION__ << ": RFC API doesn't support wildcard parameterName " << endl;
#endif
       RDK_LOG (RDK_LOG_DEBUG, LOG_RFCAPI, "%s: RFC API doesn't support wildcard parameterName\n", __FUNCTION__);
       return ret;
   }

   if(!tr69hostif_http_server_ready)
   {
      ifstream ifs_rfc("/tmp/.tr69hostif_http_server_ready");
      if(!ifs_rfc.is_open())
      {
#ifdef TEMP_LOGGING
         logofs << prefix() << __FUNCTION__ << ": file /tmp/.tr69hostif_http_server_ready doesn't exist, http server isn't ready yet" << endl;
#endif
         RDK_LOG (RDK_LOG_ERROR, LOG_RFCAPI, "%s: file /tmp/.tr69hostif_http_server_ready doesn't exist, http server isn't ready yet\n", __FUNCTION__);

         if(strncmp(pcParameterName, "RFC_", 4) == 0 && strchr(pcParameterName, '.') == NULL)
         {
            return getValue(RFCVAR_FILE, pcParameterName, pstParam);
         }
         else
         {
            ret = getValue(TR181STORE_FILE, pcParameterName, pstParam);
            if (WDMP_SUCCESS == ret)
               return WDMP_SUCCESS;

            // If the param is not found in tr181store.ini, also search in bootstrap.ini. When the hostif is not ready we do not know whether the requested param is regular tr181 param or bootstrap param.
            ret = getValue(BOOTSTRAP_FILE, pcParameterName, pstParam);
            if (WDMP_SUCCESS == ret)
               return WDMP_SUCCESS;

            // If the param is not found in override files, find it in rfcdefaults.
            return getValue(RFCDEFAULTS_FILE, pcParameterName, pstParam);

         }
      }
      else
      {
         ifs_rfc.close();
#ifdef TEMP_LOGGING
         logofs << prefix() << __FUNCTION__ << ": http server is ready" << endl;
#endif
         RDK_LOG (RDK_LOG_DEBUG, LOG_RFCAPI, "%s: http server is ready\n", __FUNCTION__);
         tr69hostif_http_server_ready = true;
      }
   }
  
   curl_handle = curl_easy_init();
   string data = "{\"names\" : [\"";
   data.append(pcParameterName);
   data.append("\"]}");
#ifdef TEMP_LOGGING
   logofs << prefix() << "getRFCParam data = " << data << " dataLen = " << data.length() << endl;
#endif
   RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"getRFCParam data = %s, datalen = %d\n", data.c_str(), data.length());
   if (curl_handle) 
   {
       char pcCallerIDHeader[128];
       if(pcCallerID)
           snprintf(pcCallerIDHeader, sizeof(pcCallerIDHeader), "CallerID: %s", pcCallerID);
       else
           sprintf(pcCallerIDHeader, "CallerID: Unknown");
       struct curl_slist *customHeadersList = NULL;
       customHeadersList = curl_slist_append(customHeadersList, pcCallerIDHeader);
       if(curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, customHeadersList) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_HTTPHEADER\n", __FUNCTION__, __LINE__);            
       }	       

       if(curl_easy_setopt(curl_handle, CURLOPT_URL, url) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_URL\n", __FUNCTION__, __LINE__);
       } 	       
       if(curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET") != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_CUSTOMREQUEST\n", __FUNCTION__, __LINE__); 
       }	       
       if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long) data.length()) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_POSTFIELDSIZE\n", __FUNCTION__, __LINE__); 
       }
       if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.c_str()) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_POSTFIELDS\n", __FUNCTION__, __LINE__); 
       }
       if(curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_FOLLOWLOCATION\n", __FUNCTION__, __LINE__);  
       }
       if(curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeCurlResponse) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_WRITEFUNCTION\n", __FUNCTION__, __LINE__); 
       }		       
       if(curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_WRITEDATA\n", __FUNCTION__, __LINE__);
       }       
       if(curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_CONNECTTIMEOUT\n", __FUNCTION__, __LINE__);
       }
       if(curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, TRANSFER_TIMEOUT) != CURLE_OK){
           RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_TIMEOUT\n", __FUNCTION__, __LINE__);   
       }	       

       res = curl_easy_perform(curl_handle);
       curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
#ifdef TEMP_LOGGING
       logofs  << prefix() << "curl response = " << res << "http response code = " << http_code << endl;
#endif
       RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"curl response : %d http response code: %ld\n", res, http_code);
       curl_easy_cleanup(curl_handle);

       curl_slist_free_all(customHeadersList);
   }
   else
   {
#ifdef TEMP_LOGGING
      logofs << prefix() << "Could not perform curl" << endl;
#endif
      RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"Could not perform curl \n");
   }
   if (res == CURLE_OK)
   {
      cJSON *response_json = NULL;
#ifdef TEMP_LOGGING
      logofs << prefix() << "curl response: " << response << endl;
#endif
      RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"Curl response: %s\n", response.c_str());
      response_json = cJSON_Parse(response.c_str());

      if (response_json)
      {
         cJSON *items = cJSON_GetObjectItem(response_json, "parameters");

         for (int i = 0 ; i < cJSON_GetArraySize(items) ; i++)
         {
            cJSON* subitem  = cJSON_GetArrayItem(items, i);
            cJSON* name    = cJSON_GetObjectItem(subitem, "name");
            if(name)
            {
               strncpy(pstParam->name, name->valuestring, MAX_PARAM_LEN);
               pstParam->name[MAX_PARAM_LEN - 1] = '\0';
#ifdef TEMP_LOGGING
               logofs << prefix() << "name = " << pstParam->name << endl;
#endif
               RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"name = %s\n", pstParam->name);
            }

            cJSON* dataType = cJSON_GetObjectItem(subitem, "dataType");
            if (dataType)
            {
               pstParam->type = (DATA_TYPE)dataType->valueint;
#ifdef TEMP_LOGGING
               logofs << prefix() << "dataType = " << pstParam->type << endl;
#endif
               RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"type = %d\n", pstParam->type);
            }
            cJSON* value = cJSON_GetObjectItem(subitem, "value");
            if (value)
            {
               strncpy(pstParam->value, value->valuestring, MAX_PARAM_LEN);
               pstParam->value[MAX_PARAM_LEN - 1] = '\0';
#ifdef TEMP_LOGGING
               logofs << prefix() << "value = " << pstParam->value << endl;
#endif
               RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"value = %s\n", pstParam->value);
            }
            cJSON* message = cJSON_GetObjectItem(subitem, "message");
            if (message)
            {
#ifdef TEMP_LOGGING
               logofs << prefix() << "message = " << message->valuestring << endl;
#endif
               RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"message = %s\n", message->valuestring);
            }
         }
         cJSON* statusCode = cJSON_GetObjectItem(response_json, "statusCode");
         if(statusCode)
         {
            ret = (WDMP_STATUS)statusCode->valueint;
#ifdef TEMP_LOGGING
            logofs << prefix() << "statusCode = " << ret << endl;
#endif
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"statusCode = %d\n", ret);
         }
         cJSON_Delete(response_json);
      }
   }
   return ret;
}

WDMP_STATUS setRFCParameter(const char *pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType)
{
#ifdef TEMP_LOGGING 
   openLogFile();
#endif
   WDMP_STATUS ret = WDMP_FAILURE;
   long http_code = 0;
   string response;
   CURL *curl_handle = NULL;
   CURLcode res = CURLE_FAILED_INIT;

   if(!strcmp(pcParameterName+strlen(pcParameterName)-1,".") && pcParameterValue == NULL)
   {
#ifdef TEMP_LOGGING
   logofs << prefix() <<__FUNCTION__ << ": RFC API doesn't support wildcard parameterName or NULL parameterValue" << endl;
#endif
       RDK_LOG (RDK_LOG_DEBUG, LOG_RFCAPI, "%s: RFC API doesn't support wildcard parameterName or NULL parameterValue\n", __FUNCTION__);
       return ret;
   }

   curl_handle = curl_easy_init();

   ostringstream ss;
   ss << eDataType;
   string strDataType = ss.str();

   string data = "{\"parameters\" : [{\"name\":\"";
   data.append(pcParameterName);
   data.append("\",\"value\":\"");
   data.append(pcParameterValue);
   data.append("\",\"dataType\":");
   data.append(strDataType);
   data.append("}]}");
#ifdef TEMP_LOGGING
   logofs << prefix() << "setRFCParam data = " << data << " dataLen = " <<  data.length() << endl;
#endif
   RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"setRFCParam data = %s, datalen = %d\n", data.c_str(), data.length());

   if (curl_handle)
   {
       char pcCallerIDHeader[128];
       if(pcCallerID)
           snprintf(pcCallerIDHeader, sizeof(pcCallerIDHeader), "CallerID: %s", pcCallerID);
       else
           sprintf(pcCallerIDHeader, "CallerID: Unknown");

      struct curl_slist *customHeadersList = NULL;
      customHeadersList = curl_slist_append(customHeadersList, pcCallerIDHeader);
      if(curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, customHeadersList) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_HTTPHEADER\n", __FUNCTION__, __LINE__);
      }
      if(curl_easy_setopt(curl_handle, CURLOPT_URL, url) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_URL\n", __FUNCTION__, __LINE__); 
      }
      if(curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1L) != CURLE_OK){
	  RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_HTTPPOST\n", __FUNCTION__, __LINE__);    
      } 	      
      if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long) data.length()) != CURLE_OK){
	  RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_POSTFIELDSIZE\n", __FUNCTION__, __LINE__);    
      }  	      
      if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.c_str()) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_POSTFIELDS\n", __FUNCTION__, __LINE__);      	      
      }	      
      if(curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_FOLLOWLOCATION\n", __FUNCTION__, __LINE__);      	      
      }	      
      if(curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeCurlResponse) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_WRITEFUNCTION\n", __FUNCTION__, __LINE__);
      }	      
      if(curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response) != CURLE_OK){
          RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"%s:%d curl setup failed for CURLOPT_WRITEDATA\n", __FUNCTION__, __LINE__);
      } 	      
      res = curl_easy_perform(curl_handle);
      curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);

#ifdef TEMP_LOGGING
   logofs << prefix() << "curl response = " << res << "http response code = " << http_code << endl;
#endif
      RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"curl response : %d http response code: %ld\n", res, http_code);
      curl_easy_cleanup(curl_handle);

      curl_slist_free_all(customHeadersList);
   }
   else
   {
#ifdef TEMP_LOGGING
   logofs << prefix() << "Could not perform curl" << endl;
#endif
      RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI,"Could not perform curl \n");
   }
   if (res == CURLE_OK)
   {
      cJSON *response_json = NULL;
#ifdef TEMP_LOGGING
   logofs << prefix() << "curl response: " << response << endl;
#endif
      RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"Curl response: %s\n", response.c_str());
      response_json = cJSON_Parse(response.c_str());
      if (response_json)
      {
         cJSON* statusCode = cJSON_GetObjectItem(response_json, "statusCode");
         if(statusCode)
         {
            ret = (WDMP_STATUS)statusCode->valueint;
#ifdef TEMP_LOGGING
   logofs << prefix() << "statusCode = " << statusCode->valueint << endl;
#endif
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCAPI,"statusCode = %d\n", ret);
         }
         cJSON_Delete(response_json);
      }
   }
   return ret;
}

/**
* Return the textual description of error code from wdmp_status
*@param [in] wdmp_status
*@param [out] textual description of error code.
*/
const char * getRFCErrorString(WDMP_STATUS code)
{
   const char * err_string;
   switch(code)
   {
      case WDMP_SUCCESS:
         err_string = " Success";
            break;
      case WDMP_FAILURE:
         err_string = " Request Failed";
            break;
      case WDMP_ERR_TIMEOUT:
         err_string = " Request Timeout";
            break;
       case WDMP_ERR_INVALID_PARAMETER_NAME:
          err_string = " Invalid Parameter Name";
             break;
       case WDMP_ERR_INVALID_PARAMETER_TYPE:
          err_string = " Invalid Parameter Type";
             break;
       case WDMP_ERR_INVALID_PARAMETER_VALUE:
          err_string = " Invalid Parameter Value";
             break;
       case WDMP_ERR_NOT_WRITABLE:
          err_string = " Not writable";
             break;
       case WDMP_ERR_SETATTRIBUTE_REJECTED:
          err_string = " SetAttribute Rejected";
             break;
       case WDMP_ERR_NAMESPACE_OVERLAP:
          err_string = " Namespace Overlap";
             break;
       case WDMP_ERR_UNKNOWN_COMPONENT:
          err_string = " Unknown Component";
             break;
       case WDMP_ERR_NAMESPACE_MISMATCH:
          err_string = " Namespace Mismatch";
             break;
       case WDMP_ERR_UNSUPPORTED_NAMESPACE:
          err_string = " Unsupported Namespace";
             break;
       case WDMP_ERR_DP_COMPONENT_VERSION_MISMATCH:
          err_string = " Component Version Mismatch";
             break;
       case WDMP_ERR_INVALID_PARAM:
          err_string = " Invalid Param";
             break;
       case WDMP_ERR_UNSUPPORTED_DATATYPE:
          err_string = " Unsupported Datatype";
             break;
       case WDMP_STATUS_RESOURCES:
          err_string = " Resources";
             break;
       case WDMP_ERR_WIFI_BUSY:
          err_string = " Wifi Busy";
             break;
       case WDMP_ERR_INVALID_ATTRIBUTES:
          err_string = " Invalid Attributes";
             break;
       case WDMP_ERR_WILDCARD_NOT_SUPPORTED:
          err_string = " Wildcard Not Supported";
             break;
       case WDMP_ERR_SET_OF_CMC_OR_CID_NOT_SUPPORTED:
          err_string = " Set of CMC or CID Not Supported";
             break;
       case WDMP_ERR_VALUE_IS_EMPTY:
          err_string = " Value is Empty";
             break;
       case WDMP_ERR_VALUE_IS_NULL:
          err_string = " Value is Null";
             break;
       case WDMP_ERR_DATATYPE_IS_NULL:
          err_string = " Datatype is Null";
             break;
       case WDMP_ERR_CMC_TEST_FAILED:
          err_string = " CMC Test Failed";
             break;
       case WDMP_ERR_NEW_CID_IS_MISSING:
          err_string = " New CID is Missing";
             break;
       case WDMP_ERR_CID_TEST_FAILED:
          err_string = " CID Test Failed";
             break;
       case WDMP_ERR_SETTING_CMC_OR_CID:
          err_string = " Setting CMC or CID";
             break;
       case WDMP_ERR_INVALID_INPUT_PARAMETER:
          err_string = " Invalid Input Parameter";
             break;
       case WDMP_ERR_ATTRIBUTES_IS_NULL:
          err_string = " Attributes is Null";
             break;
       case WDMP_ERR_NOTIFY_IS_NULL:
          err_string = " Notify is Null";
             break;
       case WDMP_ERR_INVALID_WIFI_INDEX:
          err_string = " Invalid Wifi Index";
             break;
       case WDMP_ERR_INVALID_RADIO_INDEX:
          err_string = " Invalid Radio Index";
             break;
       case WDMP_ERR_ATOMIC_GET_SET_FAILED:
          err_string = " Atomic Get Set Failed";
             break;
       case WDMP_ERR_METHOD_NOT_SUPPORTED:
          err_string = " Method Not Supported";
             break;
       case WDMP_ERR_INTERNAL_ERROR:
          err_string = " Internal Error";
             break;
       case WDMP_ERR_DEFAULT_VALUE:
          err_string = " Default Value";
             break;
       default:
          err_string = " Unknown error code";
   }
   return err_string;
}

bool isRFCEnabled(const char *feature)
{
   struct stat buffer;
   string fileName = RFC_FEATURE_DIR + string(".RFC_") + feature + ".ini";

   return (stat(fileName.c_str(), &buffer) == 0);
}

#endif

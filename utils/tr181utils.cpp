/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
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


/**
* @defgroup rfc
* @{
* @defgroup utils
* @{
**/

#include <unistd.h>
#include <error.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include "rfcapi.h"
#include "tr181api.h"
#include "trsetutils.h"
using namespace std;

static char value_type = 'u';
static char * value = NULL;
static char * key = NULL;
static char * id = NULL;
static REQ_TYPE mode = GET;
static bool silent = true;

inline bool legacyRfcEnabled() {
    ifstream f("/opt/RFC/.RFC_LegacyRFCEnabled.ini");
    return f.good();
}

/**
* Returns the parameter type
*@param [in] paramName Name of the parameter for which type is requested
*@param [out] paramType Holds the value of paramtype if call is successful
*@returns true if the call succeded, false otherwise.
*/
static bool getParamType(char * const paramName, DATA_TYPE * paramType)
{
   RFC_ParamData_t param = {0};
   param.type = WDMP_NONE;
   WDMP_STATUS status = getRFCParameter(NULL, paramName, &param);

   if(param.type != WDMP_NONE)
   {
      cout << __FUNCTION__ << " >> Using the type provided by hostif, type= " << param.type << endl;
      *paramType = param.type;
      return true;
   }
   else
      cout << __FUNCTION__ << " >> Failed to retrieve : Reason " << getRFCErrorString(status) << endl;

   return false;
}

/**
* Convert the user input to enumeration
* @param [in] type character value, can be (s)tring, (i)integer or (b) boolean
*/
static DATA_TYPE convertType(char type)
{
   DATA_TYPE t;
   switch(type)
   {
      case 's':
         t = WDMP_STRING;
         break;
      case 'i':
         t = WDMP_INT;
         break;
      case 'b':
         t = WDMP_BOOLEAN;
         break;
      default:
         cout << __FUNCTION__ << " >>Invalid type entered, default to integer." << endl;
         t = WDMP_INT;
         break;
   }
   return t;
}

/**
* Retrieves the details about a property
* @param [in] paramName the parameter whose properties are retrieved
* @return 0 if succesfully retrieve value, 1 otherwise
*/
static int getAttribute(char * const paramName)
{
   if (id && !strncmp(id, "localOnly", 9)) {
       TR181_ParamData_t param;
       tr181ErrorCode_t status = getLocalParam(id, paramName, &param);
       if(status == tr181Success)
       {
           cout << __FUNCTION__ << " >> Param Value :: " << param.value << endl;
           cerr << param.value << endl;
       }
       else
       {
          cout << __FUNCTION__ << " >> Failed to retrieve : Reason " << getTR181ErrorString(status) << endl;
       }
       return status;
    }

   RFC_ParamData_t param;
   WDMP_STATUS status = getRFCParameter(id, paramName, &param);

   if(status == WDMP_SUCCESS || status == WDMP_ERR_DEFAULT_VALUE)
   {
       cout << __FUNCTION__ << " >> Param Value :: " << param.value << endl;
       cerr << param.value << endl;
   }
   else
   {
      cout << __FUNCTION__ << " >> Failed to retrieve : Reason " << getRFCErrorString(status) << endl;
   }

   return status;
}
/**
* Set the value for a property
* @param [in] paramName the name of the property
* @param [in] type type of property
* @param [in] value  value of the property
* @return 0 if success, 1 otherwise
*/
static int setAttribute(char * const paramName  ,char type, char * value)
{
   if (id && !strncmp(id, "localOnly", 9)) {
      int status = setLocalParam(id, paramName, value);
      if(status == 0)
      {
         cout << __FUNCTION__ << " >> Set Local Param success! " << endl;
      }
      else
      {
         cout << __FUNCTION__ << " >> Failed to Set Local Param." << endl;
      }
      return status;
   }

   DATA_TYPE paramType;
   if (!getParamType (paramName, &paramType))
   {
      cout << __FUNCTION__ << " >>Failed to retrive parameter type from agent. Using provided values " <<endl;
      paramType = convertType(type);
   }

   WDMP_STATUS status = setRFCParameter(id, paramName, value, paramType);
   if (status != WDMP_SUCCESS)
      cout << __FUNCTION__ << " >> Set operation failed : " << getRFCErrorString(status) << endl;
   else
      cout << __FUNCTION__ << " >> Set operation success " << endl;

   return status;
}

/**
* Clears a local setting of a property
* @param [in] paramName the parameter whose properties are retrieved
* @return 0 if succesfully clears value, 1 otherwise
*/
static int clearAttribute(char * const paramName)
{
   int status = clearParam(id, paramName);

   if(status == 0)
   {
       cout << __FUNCTION__ << " >> Clear success! " << endl;
   }
   else
   {
      cout << __FUNCTION__ << " >> Failed to clear." << endl;
   }

   return status;
}

/**
* Prints the usage of this app
*/
static void showusage(const char *exename)
{
   cout << "Usage : " << exename << "[-d] [-g] [-s] [-v value] ParamName\n" <<
      "-d debug enable\n-g get operation\n-s set operation\n-v value of parameter\n" <<
      "If -s option is set -v is mandatory, otherwise -g option is default\n" <<
      "eg:\n" << exename << " Device.DeviceInfo.X_RDKCENTRAL-COM_xBlueTooth.Enabled\n" <<
      exename << " -s -v XG1 Device.DeviceInfo.X_RDKCENTRAL-COM_PreferredGatewayType\n" <<
      endl;
}

static int parseargs(int argc, char * argv[])
{
    int i = 1;
    while ( i < argc )
    {
        if(strncasecmp(argv[i], "-s", 2) == 0)
        {
            mode = SET;
            i ++;
        }
        else if(strncasecmp(argv[i], "-c", 2) == 0)
        {
            mode = DELETE_ROW;
            i ++;
        }
        else if(strncasecmp(argv[i], "-t", 2) == 0)
        {
            if (i + 1 < argc) {
                value_type = argv[i+1][0];
                i += 2;
            }
        }
        else if(strncasecmp(argv[i], "-v", 2) == 0)
        {
            if (i + 1 < argc) {
                value = argv[i+1];
                i += 2;
            }
        }
        else if(argv[i][0] != '-')
        {
            key = argv[i];
            i++;
        }
        else if(strncasecmp(argv[i], "-d", 2) == 0)
        {
            silent = false;
            i ++;
        }
        else if(strncasecmp(argv[i], "-n", 2) == 0)
        {
            id = argv[i+1];
            i += 2;
        }
        else
        {
            if(!silent)
                cout << __FUNCTION__ << " >>Ignoring input "<<argv[i]<<endl;
            i++;
        }
    }
    return  0;
}

#ifndef GTEST_ENABLE
int main(int argc, char *argv [])
{
   if(legacyRfcEnabled() == true)
   {
      return trsetutil(argc,argv);
   }

   streambuf* stdout_handle = nullptr;
   ofstream void_file;
   int retcode = 1;

   parseargs(argc,argv);

   if(NULL == key || (mode == SET && NULL == value))
   {
      showusage(argv[0]);
      exit(1);
   }

   if(silent)
   {
      void_file.open("/dev/null");
      stdout_handle = cout.rdbuf();

      cout.rdbuf(void_file.rdbuf());
   }
   try {
      if(mode == GET)
      {
         retcode = getAttribute(key);
      }
      else if(mode == DELETE_ROW){
        retcode = clearAttribute(key);
      }
      else if( mode == SET && NULL != value){
        retcode = setAttribute(key,value_type, value);
      }

   } catch (const std::bad_cast& e) {
       cout << "Caught bad_cast exception: " << e.what() << endl;
   } catch (const std::exception& e) {
       cout << "Exception caught in main: " << e.what() << endl;
   }
   //Redirecting again to avoid rfcapi prints
   if(silent)
   {
      cout.rdbuf(stdout_handle);
      void_file.close();
   }
   return retcode;
}
#endif
/** @} */
/** @} */

// Define your write callback function
#ifdef GTEST_ENABLE
bool (*getGetParamTypeFunc())(char * const, DATA_TYPE *)
{
    return &getParamType;
}

DATA_TYPE (*getConvertTypeFunc())(char)
{
    return &convertType;
}

int (*getClearAttributeFunc())(char * const)
{
    return &clearAttribute;
}

int (*getSetAttributeFunc())(char * const, char, char *)
{
    return &setAttribute;
}

int (*getGetAttributeFunc())(char * const)
{
    return &getAttribute;
}

int (*getparseargsFunc())(int argc, char * argv[])
{
    return &parseargs;
}

#endif

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
using namespace std;

#if defined(USE_IARMBUS)
#include "libIBus.h"
#endif

#if defined(USE_TR69HOSTIF)
#include "hostIf_tr69ReqHandler.h"
#endif

#define IARM_APP_NAME "tr181Set"
#define ONE_SEC_MILLIS 1000

#if defined(USE_TR69HOSTIF)
HostIf_ReqType_t mode = HOSTIF_GET;
#endif

char value_type = 'u';
char * value = NULL;
char * key = NULL;
bool silent = true;
/**
* Initializes the application. This method will not return until IPC connectivity is established.
*
*/
#if defined(USE_TR69HOSTIF)
static void  initilize()
{
#if defined(USE_IARMBUS)
        IARM_Result_t result = IARM_RESULT_SUCCESS;
        int retry = 0;

        //We are going to make sure we get hooked in to the bus
        while(true)
        {
                result = IARM_Bus_Init(IARM_APP_NAME);
                if(IARM_RESULT_SUCCESS != result)
                {
			cout << __FUNCTION__ << " >> Failed to initialize IARM bus." << endl;
                        retry += 1;
                        continue;
                }

                //Let us connect
                result = IARM_Bus_Connect();
                if(IARM_RESULT_SUCCESS == result)
                {
                        break;
                }
                else
                {
			cout << __FUNCTION__ << " >> Failed to connect to IARM bus. " << result;

                        retry += 1;
                }
                IARM_Bus_Term();//Since we already initialized.
                sleep(ONE_SEC_MILLIS); //Let us pause for a second.
        }
	cout << __FUNCTION__ << " >>IARM Initialized successfully" << endl;
#endif
}
#endif
/**
* Return the textual description of error code from host interface
*@param [in] code the code returned from IARM_Bus_Call
*@param [out] textual description of error code.
*/
#if defined(USE_IARMBUS)
static string getErrorDetails(faultCode_t code)
{
	string err_string;
	switch(code)
	{
		case fcMethodNotSupported:
			err_string = " Method not supported";
			break;
		case fcRequestDenied:
			err_string = " Request denied";
			break;
		case fcInternalError:
			err_string = " Internal error ";
			break;
		case fcInvalidArguments:
			err_string = " Method not supported";
			break;
		case fcResourcesExceeded:
			err_string = " Resorces exhausted";
			break;
		case fcInvalidParameterName:
			err_string = " Invalid parameter name";
			break;
		case fcInvalidParameterType:
			err_string = " Invalid parameter type";
			break;
		case fcInvalidParameterValue:
			err_string = " Invalid parameter value";
			break;
		case fcAttemptToSetaNonWritableParameter:
			err_string = " Read only parameter";
			break;
		default:
			err_string = " Unknown error code";
	}
	return err_string;
}
#else
string getErrorDetails() {
	string err_string = " Unknown error code";
	return err_string;
}
#endif
/**
* Returns the parameter type
*@param [in] paramName Name of the parameter for which type is requested
*@param [out] paramType Holds the value of paramtype if call is successful
*@returns true if the call succeded, false otherwise.
*/
#if defined(USE_TR69HOSTIF) && defined(USE_IARMBUS)
static bool getParamType(char * const paramName, HostIf_ParamType_t * paramType)
{
        IARM_Result_t result;
	bool status = false;

        HOSTIF_MsgData_t param;
	memset(&param,0,sizeof(param));
        snprintf(param.paramName,TR69HOSTIFMGR_MAX_PARAM_LEN,"%s",paramName);
        param.reqType = HOSTIF_GET;
        cout << __FUNCTION__ << " >>Retrieving values for :: " << paramName << endl;

        result = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,IARM_BUS_TR69HOSTIFMGR_API_GetParams,
                (void *)&param, sizeof(param));

        if(result  == IARM_RESULT_SUCCESS)
        {
		if(param.faultCode == fcNoFault)
		{
			*paramType = param.paramtype;
			status = true;
		}
		else
		{
			cout << __FUNCTION__ << " >> Failed to retrieve : Reason " << getErrorDetails(param.faultCode) << endl;
		}
	}
	return status;
}
#else
bool getParamType()
{
	return true;
}
#endif
/**
* Converts the parameter values based on the paramter types
* @param [in] param the object holding parameter details
* @param [in] value in string format
*/
#if defined(USE_TR69HOSTIF)
static void convertParamValues (HOSTIF_MsgData_t *param, char *value)
{
	switch (param->paramtype)
	{
		case hostIf_StringType:
		case hostIf_DateTimeType:
			snprintf(param->paramValue,TR69HOSTIFMGR_MAX_PARAM_LEN, "%s", value);
			cout << __FUNCTION__ << " >>Considered as string type " << endl;
			break;
		case hostIf_IntegerType:
		case hostIf_UnsignedIntType:
			{
				int ivalue = atoi(value);
				int *iptr = (int *)param->paramValue;
				*iptr = ivalue;
				cout << __FUNCTION__ << " >>Type :integer, value : " << ivalue << endl;
			}
			break;
		case hostIf_UnsignedLongType:
			{
				long lvalue = atol(value);
				long *lptr = (long *)param->paramValue;
				*lptr = lvalue;
				cout << __FUNCTION__ << " >>Type long, value : " << lvalue << endl;
			}
			break;
		case hostIf_BooleanType:
			{
				bool *bptr = (bool *)param->paramValue;
				*bptr = (0 == strncasecmp(value, "TRUE", 4)|| (isdigit(value[0]) && value[0] != '0' ));
				cout << __FUNCTION__ << " >>Type boolean, value : " << (*bptr) << endl;
			}
			break;
		default:
			cout << __FUNCTION__ << " >>This path should never be reached. param type is %d " << param->paramtype << endl;

	}
}
#else
void convertParamValues()
{

}
#endif
/**
* Convert the user input to enumeration
* @param [in] type character value, can be (s)tring, (i)integer or (b) boolean
*/
#if defined(USE_TR69HOSTIF)
static HostIf_ParamType_t convertType(char type)
{
	HostIf_ParamType_t t;
	switch(type)
	{
		case 's':
			t = hostIf_StringType;
			break;
		case 'i':
			t = hostIf_IntegerType;
			break;
		case 'b':
			t = hostIf_BooleanType;
			break;
		default:
			cout << __FUNCTION__ << " >>Invalid type entered, default to integer." << endl;
			t = hostIf_IntegerType;
			break;
	}
	return t;
}
#else
int convertType(char type)
{
	return 0;
}
#endif
/**
* @param [in] param the object holding the details
*/
#if defined(USE_TR69HOSTIF)
static void printParameterDetails(HOSTIF_MsgData_t* param )
{
	cout << __FUNCTION__ << " >>Get Operation successfull " <<endl;
	switch (param->paramtype)
	{
		case hostIf_StringType :
			cout << __FUNCTION__ << " >>Parameter Type ::String" << endl;
			cout << __FUNCTION__ << " >>Param value ::" << param->paramValue << endl;
			cerr << param->paramValue << endl;
			break;
		case hostIf_IntegerType :
			{
				cout << __FUNCTION__ << " >>Parameter Type ::Integer" << endl;
				int * iptr = (int *)param->paramValue;
				cout << __FUNCTION__ << " >>Param value ::" << (*iptr) << endl;
				cerr << (*iptr) << endl;
			}
			break;
		case hostIf_UnsignedIntType :
			{
				cout << __FUNCTION__ << " >>Parameter Type ::Unsigned Integer" << endl;
				int * iptr = (int *)param->paramValue;
				cout << __FUNCTION__ << " >>Param value ::" << (*iptr) << endl;
				cerr << (*iptr) << endl;
			}
			break;
		case hostIf_BooleanType:
			{
				cout << __FUNCTION__ << " >>Parameter Type ::Boolean" << endl;
				bool * bptr = (bool *)param->paramValue;
				cout << __FUNCTION__ << " >>Param value ::" << (*bptr) << endl;
				cerr << (*bptr) << endl;
			}
			break;
		case hostIf_DateTimeType:
			cout << __FUNCTION__ << " >>Parameter Type ::DateTime" << endl;
			cout << __FUNCTION__ << " >>Param value ::" << param->paramValue << endl;
			cerr << param->paramValue << endl;
			break;
		case hostIf_UnsignedLongType:
			{
				cout << __FUNCTION__ << " >>Parameter Type ::Unsigned Long" << endl;
				long * lptr = (long *)param->paramValue;
				cout << __FUNCTION__ << " >>Param value ::" << (*lptr) << endl;
				cerr << (*lptr) << endl;
			}
			break;
	}
}
#else
void printParameterDetails(){

}
#endif
/**
* Retrieves the details about a property
* @param [in] paramName the parameter whose properties are retrieved
* @return 0 if succesfully retrieve value, 1 otherwise
*/
#if defined(USE_TR69HOSTIF)
static int getAttribute(char * const paramName)
{
	int status = 1;
#if defined(USE_TR69HOSTIF) && defined(USE_IARMBUS)
        IARM_Result_t result;
	HOSTIF_MsgData_t param;
	memset(&param,0,sizeof(param));
	snprintf(param.paramName,TR69HOSTIFMGR_MAX_PARAM_LEN,"%s",paramName);
	param.reqType = HOSTIF_GET;
	cout << __FUNCTION__ << " >>Retrieving values for :: " << paramName << endl;

	result = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,IARM_BUS_TR69HOSTIFMGR_API_GetParams,
		(void *)&param,	sizeof(param));

	if(result  == IARM_RESULT_SUCCESS)
	{
		if(fcNoFault == param.faultCode)
		{
			status = 0;
			printParameterDetails(param);
		}
	}
	else
	{
		cout << __FUNCTION__ << " >>Failed to retrieve value  " << result << endl;
	}
#else
    status = 0;
#endif
	return status;
}
#endif
/**
* Set the value for a property
* @param [in] paramName the name of the property
* @param [in] type type of property
* @param [in] value  value of the property
* @return 0 if success, 1 otherwise
*/
#if defined(USE_TR69HOSTIF)
static int setAttribute(char * const paramName  ,char type, char * value)
{
	int status = 1;
#if defined(USE_TR69HOSTIF)
	IARM_Result_t result;
	HOSTIF_MsgData_t param = {0};
	HostIf_ParamType_t paramType;

	snprintf(param.paramName,TR69HOSTIFMGR_MAX_PARAM_LEN,"%s", paramName);
	if (getParamType (paramName, &paramType))
	{
		param.paramtype = paramType;
	}
	else
	{
		cout << __FUNCTION__ << " >>Failed to retrive parameter type from agent. Using provided values " <<endl;
		param.paramtype = convertType(type);
	}
	convertParamValues(&param,value);

	param.reqType = HOSTIF_SET;

	result = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_SetParams,
	(void *)&param, sizeof(param));

	if(result  == IARM_RESULT_SUCCESS)
	{
		if(param.faultCode == fcNoFault)
		{
			cout << __FUNCTION__ << " >> Set operation passed" << endl;
			status = 0;
			getAttribute(paramName);
		}
		else
		{
			cout << __FUNCTION__ << " >> Set operation failed : " << getErrorDetails(param.faultCode) << endl;
		}
	}
	else
	{
		cout << __FUNCTION__ << " >>Failed to set value for " << paramName << " , reason " << result << endl;
	}
#else
    status = 0;
#endif
	return status;
}
#endif

/**
* Cleanup for IPC setup
*/
#if defined(USE_TR69HOSTIF)
static void term()
{
#if defined(USE_IARMBUS)
	IARM_Bus_Disconnect();
	IARM_Bus_Term();
#endif
}
#endif

/**
* Prints the usage of this app
*/
#if defined(USE_TR69HOSTIF)
static void showusage(const char *exename)
{
	cout << "Usage : " << exename << "[-d] [-g] [-s] [-v value] ParamName\n" <<
		"-d debug enable\n-g get operation\n-s set operation\n-v value of parameter\n" <<
		"If -s option is set -v is mandatory, otherwise -g option is default\n" <<
		"eg:\n" << exename << " Device.DeviceInfo.X_RDKCENTRAL-COM_xBlueTooth.Enabled\n" <<
		exename << " -s -v XG1 Device.DeviceInfo.X_RDKCENTRAL-COM_PreferredGatewayType\n" <<
		endl;
}
#endif

int parseargs(int argc, char * argv[])
{
#if defined(USE_TR69HOSTIF)
    int i = 1;
    while ( i < argc )
    {
        if(strncasecmp(argv[i], "-s", 2) == 0)
        {
            mode = HOSTIF_SET; //Set
            i ++;
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
        else
        {
	    if(!silent)
	            cout << __FUNCTION__ << " >>Ignoring input "<<argv[i]<<endl;
            i++;
        }
    }
#endif
    return  0;
}

int trsetutil(int argc, char *argv [])
{
	int retcode = 1;
#if defined(USE_TR69HOSTIF)
	streambuf* stdout_handle = nullptr;
	ofstream void_file;

	parseargs(argc,argv);

	if(NULL == key || (mode == HOSTIF_SET && NULL == value))
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
	initilize();

	if(mode == HOSTIF_GET)
	{
		retcode = getAttribute(key);
	}
	else if( NULL != value){
		retcode = setAttribute(key,value_type, value);
	}

	//Redirecting again to avoid IARM prints
	term();
	if(silent)
	{
		cout.rdbuf(stdout_handle);
		void_file.close();
	}
#else
    retcode = 0;
#endif
	return retcode;
}
/** @} */
/** @} */


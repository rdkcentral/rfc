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
#include <unistd.h>
#include <sstream>
#include "rdk_debug.h"
#include "rfcapi.h"
#include "tr181api.h"
#include "trsetutils.h"
#include <vector>
#include <pwd.h>

using namespace std;
#define LOG_RFCAPI "LOG.RDK.RFCAPI"

static char value_type = 'u';
static char * value = NULL;
static char * key = NULL;
static char * id = NULL;
static REQ_TYPE mode = GET;
static bool silent = false;

// Helper: Split a null-separated cmdline into arguments
std::vector<std::string> splitCmdline(const std::string& cmdline) {
    std::vector<std::string> args;
    size_t start = 0;
    while (start < cmdline.size()) {
        size_t end = cmdline.find('\0', start);
        if (end == std::string::npos) end = cmdline.size();
        if (end > start) args.push_back(cmdline.substr(start, end - start));
        start = end + 1;
    }
    return args;
}

// Helper: Read cmdline of a process as a string
static std::string getCmdline(pid_t pid) {
    std::stringstream path;
    path << "/proc/" << pid << "/cmdline";
    std::ifstream file(path.str(), std::ios::in | std::ios::binary);
    if (!file) return "";
    std::string buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return buf;
}

// Helper: Get parent PID of a process
static pid_t getParentPid(pid_t pid) {
    std::stringstream path;
    path << "/proc/" << pid << "/status";
    std::ifstream file(path.str());
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("PPid:", 0) == 0) {
            return std::stoi(line.substr(5));
        }
    }
    return -1;
}

// Helper: Build ancestry chain up to root or max depth
static std::vector<std::pair<pid_t, std::string>> getProcessAncestry(pid_t start_pid, int max_depth = 5) {
    std::vector<std::pair<pid_t, std::string>> ancestry;
    pid_t current = start_pid;
    for (int i = 0; i < max_depth && current > 1; ++i) {
        std::string cmd = getCmdline(current);
        ancestry.push_back(std::make_pair(current, cmd));
        current = getParentPid(current);
    }
    return ancestry;
}

// -- Improved Script Name Detection --
static std::string detectScriptName() {
    // 1. Try BASH_SOURCE environment variable (if exported in the shell script)
    const char* bash_source = getenv("BASH_SOURCE");
    if (bash_source && bash_source[0] != '\0') {
        return std::string(bash_source);
    }

    // 2. Try scanning parent process arguments for a script
    pid_t ppid = getppid();
    std::string parent_cmdline = getCmdline(ppid);
    std::vector<std::string> parent_args = splitCmdline(parent_cmdline);

    for (size_t i = 1; i < parent_args.size(); ++i) {
        const std::string& arg = parent_args[i];
        // Heuristic: ends with script extension
        if ((arg.size() > 3 && (arg.substr(arg.size()-3) == ".sh" ||
                                arg.substr(arg.size()-3) == ".py" ||
                                arg.substr(arg.size()-3) == ".pl")) ||
            (arg.size() > 2 && arg.substr(arg.size()-2) == ".ksh")) {
            return arg;
        }
        // Or, exists as a file
        std::ifstream f(arg);
        if (f.good()) return arg;
    }

    // 3. Fallback: use our own process's argv[0]
    std::string self_cmdline = getCmdline(getpid());
    std::vector<std::string> self_args = splitCmdline(self_cmdline);
    if (!self_args.empty()) {
        return self_args[0];
    }

    // 4. Unknown
    return "<unknown>";
}

static void logCallerInfo(const char* operation, const char* paramName) {
    extern bool silent;
    if (silent) return;

    pid_t ppid = getppid();
    std::vector<std::pair<pid_t, std::string>> ancestry = getProcessAncestry(ppid);

    std::string parent_cmdline = getCmdline(ppid);
    std::vector<std::string> parent_args = splitCmdline(parent_cmdline);

    std::string script_name = detectScriptName();

    std::cout << "Parent process arguments:" << std::endl;
    for (size_t i = 0; i < parent_args.size(); ++i) {
        std::cout << "  [" << i << "]: " << parent_args[i] << std::endl;
    }

    bool is_terminal = isatty(STDIN_FILENO);

    const char* bash_source = getenv("BASH_SOURCE");
    const char* bash_lineno = getenv("BASH_LINENO");

    const char* user = getlogin();
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) user = pw->pw_name;
    }

    std::cout << "================== CALLER TRACE ==================" << std::endl;
    std::cout << "Operation: " << operation << ", Param: " << paramName << std::endl;
    std::cout << "User: " << (user ? user : "<unknown>") << std::endl;
    std::cout << "Script Name: " << script_name << std::endl;

    // -- Call Type Detection --
    std::string parent_cmd = ancestry.empty() ? "<unknown>" : ancestry[0].second;
    if (bash_source && bash_lineno && std::strlen(bash_source) > 0) {
        std::cout << "Caller Type: Shell script (exported BASH_SOURCE)" << std::endl;
        std::cout << "Script: " << bash_source << ":" << bash_lineno << std::endl;
    }
    else if (parent_cmd.find("bash") != std::string::npos ||
             parent_cmd.find("sh") != std::string::npos ||
             parent_cmd.find("dash") != std::string::npos) {
        if (is_terminal) {
            std::cout << "Caller Type: Manual shell (interactive)" << std::endl;
        } else if (parent_cmd.find(".sh") != std::string::npos) {
            std::cout << "Caller Type: Shell script (inferred from .sh in command)" << std::endl;
        } else {
            std::cout << "Caller Type: Likely shell script (non-interactive shell)" << std::endl;
        }
    }
    else {
        std::cout << "Caller Type: Other binary or system process" << std::endl;
    }

    // -- Print Process Ancestry --
    std::cout << "Process Ancestry (up to 5 levels):" << std::endl;
    for (std::vector<std::pair<pid_t, std::string>>::const_iterator it = ancestry.begin(); it != ancestry.end(); ++it) {
        pid_t pid = it->first;
        const std::string& cmd = it->second;
        std::cout << "  PID " << pid << ": " << cmd << std::endl;
    }

    std::cout << "==================================================" << std::endl;
}

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
   logCallerInfo("getAttribute", paramName);
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
    // Extract just the script file name
   std::string script_path = detectScriptName();
   std::string script_file;
   size_t last_slash = script_path.find_last_of("/\\");
   if (last_slash != std::string::npos)
       script_file = script_path.substr(last_slash + 1);
   else
       script_file = script_path;
   RFC_ParamData_t param;
   WDMP_STATUS status = getRFCParameter_ex(id, paramName, &param,script_file.c_str(), 0);

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
/** @} */
/** @} */


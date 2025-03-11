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

#include "xconf_handler.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <json_parse.h>
#include <rdk_fwdl_utils.h>
#include <downloadUtil.h>
#include <common_device_api.h>

namespace xconf {	

int XconfHandler::ExecuteRequest(FileDwnl_t *file_dwnl, MtlsAuth_t *security, int *httpCode)
{
	void *curl = nullptr;
	int curl_ret_code = -1;
	
	curl = doCurlInit();
    
	if(curl)
    {
	    curl_ret_code = doHttpFileDownload(curl, file_dwnl, security, 0, NULL, httpCode);
		doStopDownload(curl);
	}
	
	return curl_ret_code;
}

std::string getErouterMac() {
    std::string ifname;
    std::string erouterMac;

    FILE* pipe = popen("sysevent get current_wan_ifname", "r");
    if (pipe) {
        char buffer[128] = {0};
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            ifname = buffer;
            if (!ifname.empty() && ifname.back() == '\n') {
                ifname.pop_back();
            }
        }
        pclose(pipe);
    }

    if (!ifname.empty()) {
        std::string cmd = "ifconfig " + ifname + " | grep HWaddr | cut -d \" \" -f7";
        FILE* macPipe = popen(cmd.c_str(), "r");
        if (macPipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), macPipe) != nullptr) {
                erouterMac = buffer;
                // Trim trailing newline
                if (!erouterMac.empty() && erouterMac.back() == '\n') {
                    erouterMac.pop_back();
                }
            }
            pclose(macPipe);
        }
    }

    return erouterMac;
}

int XconfHandler:: initializeXconfHandler()
{
	char tmpbuf[200] = {0};
	int len = 0;

#if defined(RDKB_SUPPORT)
        std::string mac = getErouterMac();
        if (!mac.empty()) {
            strncpy(tmpbuf, mac.c_str(), sizeof(tmpbuf) - 1);
            tmpbuf[sizeof(tmpbuf) - 1] = '\0'; // Ensure null termination
            len = strlen(tmpbuf);
        }
#else
        len = GetEstbMac(tmpbuf, sizeof(tmpbuf));
#endif
        if( len )
	{
	    _estb_mac_address = tmpbuf;
	}
	memset(tmpbuf, '\0', sizeof(tmpbuf));
	
	len = GetFirmwareVersion( tmpbuf, sizeof(tmpbuf) );
        if( len )
        {
	    _firmware_version = tmpbuf;
	}
	
	memset(tmpbuf, '\0', sizeof(tmpbuf));
	
	len = GetBuildType( tmpbuf, sizeof(tmpbuf), &_ebuild_type );
	
	if( len )
	{
             _build_type_str = tmpbuf;
	}
	
	len = GetModelNum( tmpbuf, sizeof(tmpbuf) );
        if( len )
        {
	     _model_number = tmpbuf;
	}
	
	len = GetPartnerId( tmpbuf, sizeof(tmpbuf) );
	if( len )
        {
	     _partner_id = tmpbuf;
	}
	else
	{
	    _partner_id="Unkown";
	}
	
    return 0;
}

}
#ifdef __cplusplus
}

#endif

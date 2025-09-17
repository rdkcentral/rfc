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
#include "mtlsUtils.h"

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

int XconfHandler:: initializeXconfHandler()
{
	char tmpbuf[200] = {0};
	int len = 0;

#if defined(RDKB_SUPPORT)
        std::string mac = getErouterMac();
        if (!mac.empty()) {
            strncpy(tmpbuf, mac.c_str(), sizeof(tmpbuf) - 1);
            tmpbuf[sizeof(tmpbuf) - 1] = '\0';
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
	
	memset(tmpbuf, '\0', sizeof(tmpbuf));
	len = GetModelNum( tmpbuf, sizeof(tmpbuf) );
        if( len )
        {
	     _model_number = tmpbuf;
	}

#if defined(RDKB_SUPPORT)
        memset(tmpbuf, '\0', sizeof(tmpbuf));
        std::string cmmac = geteCMMac();
        if (!cmmac.empty()) {
            strncpy(tmpbuf, cmmac.c_str(), sizeof(tmpbuf) - 1);
            tmpbuf[sizeof(tmpbuf) - 1] = '\0';
            len = strlen(tmpbuf);
        }
        if( len )
        {
             _ecm_mac_address = tmpbuf;
        }
#else
	memset(tmpbuf, '\0', sizeof(tmpbuf));
	len = GetMFRName( tmpbuf, sizeof(tmpbuf) );
        if( len )
        {
             _manufacturer = tmpbuf;
        }
#endif
	memset(tmpbuf, '\0', sizeof(tmpbuf));
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



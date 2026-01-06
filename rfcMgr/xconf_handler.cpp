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
#include <errno.h>

namespace xconf {	

int XconfHandler::ExecuteRequest(FileDwnl_t *file_dwnl, MtlsAuth_t *security, int *httpCode)
{
	// ---- Stage 1: Basic parameter checks (run before doCurlInit) ----
	if (!file_dwnl || !httpCode) {
		return -EINVAL;       // required parameters missing
	}
	if (!security) {
		return -EKEYREQUIRED; // credential struct is mandatory for mTLS path
	}

	// ---- Stage 2: Credential-content validation (still before doCurlInit) ----
	// Adjust these to your actual MtlsAuth_t fields.
	const bool hasPkcs12 = security->pkcs12Path && security->pkcs12Path[0] != '\0';
	const bool hasPkcs11 = security->pkcs11Uri   && security->pkcs11Uri[0]   != '\0';

	// Enforce exactly one mode (prevents the mixed PKCS12+PKCS11 path seen in the crash callstack)
	if (hasPkcs12 == hasPkcs11) {
		return -EBADMSG;      // ambiguous or missing credential configuration
	}

	if (hasPkcs11) {
		if (!security->pkcs11ModulePath || security->pkcs11ModulePath[0] == '\0') {
			return -ENOENT;   // module path required for PKCS11 engine
		}
		if (!security->pkcs11Pin || security->pkcs11Pin[0] == '\0') {
			return -EACCES;   // PIN required for private key access
		}
		// Optional: lightweight URI sanity checks (prefix, required attributes)
		// if (strncmp(security->pkcs11Uri, "pkcs11:", 7) != 0) return -EBADMSG;
	}

	if (hasPkcs12) {
		// Optional: pre-check file existence if your platform/API allows here
		// if (!fileExists(security->pkcs12Path)) return -ENOENT;
	}

	// ---- Stage 3: Proceed with curl only if inputs are valid ----
	void *curl = nullptr;
	int curl_ret_code = -1;
	
	curl = doCurlInit();
    
	if(curl)
	{
		// NOTE: At this point we know we won't hand bad credential state to the TLS stack.
		curl_ret_code = doHttpFileDownload(curl, file_dwnl, security, 0, NULL, httpCode);
		doStopDownload(curl);
	}
	else
	{
		return -ECONNREFUSED; // curl init failure
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



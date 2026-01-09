/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2023 RDK Management
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
**/

#ifndef XCONF_HANDLER_H
#define XCONF_HANDLER_H

#include <string>
#include "mtlsUtils.h"

#if defined(GTEST_ENABLE)
#include <gtest/gtest.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <common_device_api.h>
#include <rdk_fwdl_utils.h>

namespace xconf {
    class XconfHandler {
        public :
        XconfHandler() : _ebuild_type(RDKB_DEV) { }
        int initializeXconfHandler(void);
        
        // We do not allow this class to be copied !!
        XconfHandler(const XconfHandler&) = delete;
        XconfHandler& operator=(const XconfHandler&) = delete;
        
#if defined(GTEST_ENABLE)
        public:
#else
	protected :
#endif
        std::string _estb_mac_address; /* Device Mac Address*/
        std::string _firmware_version; /* Device Frimware version */
        BUILDTYPE   _ebuild_type; /* Device Build Type */
	std::string _build_type_str;
        std::string _model_number; /* Device Model Number */
        std::string _manufacturer; /* Device Manufacturer */
        std::string _ecm_mac_address; /* Cable Modem Mac Address*/
        std::string _partner_id; /* Device Partner ID */
	int ExecuteRequest(FileDwnl_t *file_dwnl, MtlsAuth_t *security, int *httpCode);
    };
}

#ifdef __cplusplus
}
#endif
#endif

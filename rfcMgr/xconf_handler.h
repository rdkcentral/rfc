/**
 * @file xconf_handler.h
 * @brief Base Xconf HTTP handler — device identity and download primitives.
 *
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
 */

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

    /**
     * @class XconfHandler
     * @brief Collects device identity (MAC, FW version, model, etc.) and
     *        provides an HTTP download helper for Xconf communication.
     *
     * Non-copyable.  Derived classes (RuntimeFeatureControlProcessor and its
     * platform overrides) build on these primitives.
     */
    class XconfHandler {
        public :
        /** @brief Default constructor. */
        XconfHandler(){ }

        /**
         * @brief Populate all device-identity fields.
         * @return 0 on success.
         */
        int initializeXconfHandler(void);

        XconfHandler(const XconfHandler&) = delete;            /**< Copy disabled. */
        XconfHandler& operator=(const XconfHandler&) = delete; /**< Assignment disabled. */
        
#if defined(GTEST_ENABLE)
        public:
#else
	protected :
#endif
        std::string _estb_mac_address;  /**< Device eSTB MAC address. */
        std::string _firmware_version;  /**< Current firmware version. */
        BUILDTYPE   _ebuild_type;       /**< Build type enum (DEV/VBN/PROD). */
	std::string _build_type_str;    /**< Build type as a string. */
        std::string _model_number;      /**< Device model number. */
        std::string _manufacturer;      /**< Device manufacturer name. */
        std::string _ecm_mac_address;   /**< Cable-modem MAC address. */
        std::string _partner_id;        /**< Syndication partner ID. */

	/**
	 * @brief Execute an HTTP file download via cURL.
	 * @param[in,out] file_dwnl   Download descriptor (URL, output buffer).
	 * @param[in]     security    mTLS certificate bundle.
	 * @param[out]    httpCode    Resulting HTTP status code.
	 * @return cURL return code (0 = OK).
	 */
	int ExecuteRequest(FileDwnl_t *file_dwnl, MtlsAuth_t *security, int *httpCode);
    };
}

#ifdef __cplusplus
}
#endif
#endif

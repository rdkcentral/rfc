/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
 *
 * rdkc_rfc_xconf_handler.h
 *
 * RDKC-specific (RDKC / XHC1) RFC handler.
 *
 * Inherits all common RFC request/response logic from
 * RuntimeFeatureControlProcessor and overrides only the three methods
 * whose behaviour differs on camera platforms:
 *
 *   CreateXconfHTTPUrl()
 *       RDKC query string adds "accountHash" and omits
 *       manufacturer / ecmMacAddress / osClass.
 *
 *   RetrieveHashAndTimeFromPreviousDataSet()
 *       RDKC devices store configSetHash / configSetTime in RAM
 *       files only (/tmp/RFC/.hashValue, /tmp/RFC/.timeValue).
 *       No TR181 DB lookup is performed.
 *
 *   StoreXconfEndpointMetadata()
 *       XHC1 must NOT write XconfSelector / XconfUrl back to the
 *       data store after a successful sync — this is a no-op.
 **/

#pragma once
#ifdef RDKC

#include "rfc_xconf_handler.h"
#include <string>
#include <sstream>

/**
 * @class RdkcRuntimeFeatureControlProcessor
 * @brief RDKC camera-specific RFC processor subclass.
 *
 * Inherits all common RFC logic from RuntimeFeatureControlProcessor
 * and overrides only platform-specific methods.
 */
class RdkcRuntimeFeatureControlProcessor : public RuntimeFeatureControlProcessor
{
public:
    /** @brief Default constructor. */
    RdkcRuntimeFeatureControlProcessor() = default;

    RdkcRuntimeFeatureControlProcessor(const RdkcRuntimeFeatureControlProcessor&) = delete;            /**< Copy disabled. */
    RdkcRuntimeFeatureControlProcessor& operator=(const RdkcRuntimeFeatureControlProcessor&) = delete; /**< Assignment disabled. */

protected:
    /** @brief Build RDKC-specific Xconf URL (adds accountHash, omits manufacturer). */
    std::stringstream CreateXconfHTTPUrl() override;

    /**
     * @brief Read configSetHash/Time from RAM files (/tmp/RFC/).
     * @param[out] valueHash  Retrieved hash string.
     * @param[out] valueTime  Retrieved time string.
     */
    void RetrieveHashAndTimeFromPreviousDataSet(std::string &valueHash,
                                                std::string &valueTime) override;

    /** @brief No-op — RDKC must not persist XconfSelector/XconfUrl. */
    void StoreXconfEndpointMetadata() override;

private:
    std::string _accountHash; /**< MD5 account hash, XHC1-specific query param. */

    /** @brief Lazy-fetch the MD5 account hash from the RFC parameter store. */
    void GetAccountHash();
};

#endif /* RDKC */

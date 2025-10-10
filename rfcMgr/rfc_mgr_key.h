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

#ifndef RFC_KEY_H
#define RFC_KEY_H

#define XCONF_SELECTOR_KEY_STR "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfSelector"
#define XCONF_URL_KEY_STR      "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfUrl"
#if defined(RDKB_SUPPORT)
#define BOOTSTRAP_XCONF_URL_KEY_STR "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.XconfURL"
#else
#define BOOTSTRAP_XCONF_URL_KEY_STR "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.XconfUrl"
#endif
#define RFC_ACCOUNT_ID_KEY_STR "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
#define RFC_PARTNER_ID_KEY_STR  "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"
#define RFC_OSCLASS_KEY_STR     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass"
#define RFC_PARTNERNAME_KEY_STR "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName"
#define RFC_CONFIG_SET_HASH    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetHash"
#define RFC_CONFIG_SET_TIME    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime"
#define RFC_MTLS_KEY_STR       "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MTLS.mTlsXConfDownload.Enable"

#define XCONF_SELECTOR_NAME      "XconfSelector"
#define XCONF_URL_TR181_NAME     "XconfUrl"
#define TELEMETRY_CONFIG_URL     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.ConfigURL"
#endif

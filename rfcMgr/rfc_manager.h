/**
 * @file rfc_manager.h
 * @brief RFC Manager — orchestrates the RFC feature-control lifecycle.
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

#ifndef RFC_MGR_H
#define RFC_MGR_H

/** @addtogroup rfcMgr
 *  @{ */
#include "rfc_mgr_iarm.h"
#include "rfc_common.h"
#include <stdint.h>

#ifdef EN_MAINTENANCE_MANAGER
#include "maintenanceMGR.h"
#endif

/** @name Macros
 *  @{ */
#define DEBUG_INI_FILE "/etc/debug.ini"
#if !defined(RDKB_SUPPORT)
#define OVERIDE_DEBUG_INI_FILE "/opt/debug.ini"
#else
#define OVERIDE_DEBUG_INI_FILE "/nvram/debug.ini"
#endif
#define DNS_RESOLV_FILE "/etc/resolv.dnsmasq"
#define IP_ROUTE_FLAG "/tmp/route_available"
#define GATEWAYIP_FILE "/tmp/.GatewayIP_dfltroute"
#define ROUTE_FLAG_MAX_CHECK 5
#define RFC_MGR_INTERNET_CHECK_TIMEOUT 2000

#if defined(GTEST_ENABLE)
#include <gtest/gtest.h>
#endif

/** @} */ /* end Macros */

namespace rfc {

/**
 * @brief Device network connectivity status.
 */
enum DeviceStatus {
    RFCMGR_DEVICE_ONLINE,
    RFCMGR_DEVICE_OFFLINE
};

/** @name Maintenance Manager Event Codes
 *  @{ */
#define MAINT_RFC_ERROR         3   /**< RFC processing encountered an error. */
#define MAINT_RFC_INPROGRESS    14  /**< RFC processing is in progress. */
#define MAINT_RFC_COMPLETE      2   /**< RFC processing completed successfully. */
#define MAINT_CRITICAL_UPDATE   11  /**< A critical firmware update is available. */
#define MAINT_REBOOT_REQUIRED   12  /**< A device reboot is required. */
/** @} */

#if !defined(RDKB_SUPPORT)
#define RFC_MGR_IPTBLE_INIT_SCRIPT      "/lib/rdk/iptables_init"
#else
#define RFC_MGR_IPTBLE_INIT_SCRIPT      "/lib/rdk/RFCpostprocess.sh"
#define RFC_LOG_FILE                    "/rdklogs/logs/dcmrfc.log.0"
#endif

#define RFC_MGR_SERVICE_LOCK_FILE       "/tmp/.rfcServiceLock"

#if defined(GTEST_ENABLE)
bool isDnsResolve(const char *);
#endif

/**
 * @class RFCManager
 * @brief Manages the Remote Feature Control (RFC) lifecycle.
 *
 * Coordinates device-online checks, Xconf feature queries, post-processing,
 * maintenance-manager event notifications, and periodic cron scheduling.
 * This class is non-copyable.
 */
class RFCManager {
    public:
        /** @brief Construct and initialise RDK Logger + IARM bus. */
        RFCManager();

        RFCManager(const RFCManager &) = delete;            /**< Copy construction disabled. */
        RFCManager &operator=(const RFCManager &) = delete;  /**< Copy assignment disabled. */

        /**
         * @brief Entry point — run the full RFC Xconf request cycle.
         * @return SUCCESS (0) on success, FAILURE (-1) on error.
         */
        int RFCManagerProcessXconfRequest();

        /**
         * @brief Check whether the device has IP connectivity.
         * @return RFCMGR_DEVICE_ONLINE or RFCMGR_DEVICE_OFFLINE.
         */
        rfc::DeviceStatus CheckDeviceIsOnline(void);

#if !defined(RDKB_SUPPORT)	
        /**
         * @brief Broadcast an IARM event to the Maintenance Manager.
         * @param[in] cur_event_name  IARM event owner name.
         * @param[in] main_mgr_event  Maintenance manager event code.
         */
        void SendEventToMaintenanceManager(const char *, unsigned int);
#endif

        /**
         * @brief Configure a periodic cron job for rfcMgr execution.
         * @param[in] cron  Five-field cron schedule string.
         */
        void manageCronJob(const std::string& cron);	

#if defined(GTEST_ENABLE)
    public:
#else
    private:
#endif
        void InitializeIARM(void);                       /**< Register on the IARM bus. */
        bool isConnectedToInternet();                     /**< Quick internet-reachability probe. */
        bool CheckIProuteConnectivity(const char *);      /**< Verify IP route via gateway file. */
	std::string getErouterIPAddress();                /**< Retrieve the eRouter WAN address. */
	bool CheckIPConnectivity(void);                   /**< Check eRouter IP availability. */
        bool IsIarmBusConnected();                        /**< Query IARM connection state. */
        int RFCManagerProcess();                          /**< Core RFC fetch-and-apply logic. */
        int RFCManagerPostProcess();                      /**< Post-processing scripts (iptables, etc.). */
    }; // end of RFCManager Class

/** @} */ /* end rfcMgr group */
} // end of namespace RFC

#endif

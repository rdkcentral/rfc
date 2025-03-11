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

#ifndef RFC_MGR_H
#define RFC_MGR_H

/*----------------------------------------------------------------------------*/
/*                                   Header File                              */
/*----------------------------------------------------------------------------*/
#include "rfc_mgr_iarm.h"
#include "rfc_common.h"
#include <stdint.h>

#ifdef EN_MAINTENANCE_MANAGER
#include "maintenanceMGR.h"
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEBUG_INI_FILE "/etc/debug.ini"
#define DNS_RESOLV_FILE "/etc/resolv.dnsmasq"
#define IP_ROUTE_FLAG "/tmp/route_available"
#define GATEWAYIP_FILE "/tmp/.GatewayIP_dfltroute"
#define ROUTE_FLAG_MAX_CHECK 5
#define RFC_MGR_INTERNET_CHECK_TIMEOUT 2000

/*----------------------------------------------------------------------------*/
/*                                   Namespace                                */
/*----------------------------------------------------------------------------*/
namespace rfc {
/*----------------------------------------------------------------------------*/
/*                                   Enum                                     */
/*----------------------------------------------------------------------------*/
enum DeviceStatus {
    RFCMGR_DEVICE_ONLINE,
    RFCMGR_DEVICE_OFFLINE
};

/* Maintenance Manager Events */
#define MAINT_RFC_ERROR         3
#define MAINT_RFC_INPROGRESS    14
#define MAINT_RFC_COMPLETE      2
#define MAINT_CRITICAL_UPDATE   11
#define MAINT_REBOOT_REQUIRED   12

#if !defined(RDKB_SUPPORT)
#define RFC_MGR_IPTBLE_INIT_SCRIPT      "/lib/rdk/iptables_init"
#else
#define RFC_MGR_IPTBLE_INIT_SCRIPT      "/lib/rdk/RFCpostprocess.sh"
#endif

#define RFC_MGR_SERVICE_LOCK_FILE       "/tmp/.rfcServiceLock"

/*----------------------------------------------------------------------------*/
/*                                   Class                                    */
/*----------------------------------------------------------------------------*/
class RFCManager {
    public:
        RFCManager();
        // We do not allow this class to be copied !!
        RFCManager(const RFCManager &) = delete;
        RFCManager &operator=(const RFCManager &) = delete;
        int RFCManagerProcessXconfRequest();
        rfc::DeviceStatus CheckDeviceIsOnline(void);
        void SendEventToMaintenanceManager(const char *, unsigned int);
	void manageCronJob(const std::string& cron);

    private:
        void InitializeIARM(void);
        bool isConnectedToInternet();
        bool CheckIProuteConnectivity(const char *);
	std::string getErouterIPAddress();
        bool CheckIPConnectivity(void);
        bool IsIarmBusConnected();
        int RFCManagerProcess();
        int RFCManagerPostProcess();
    }; // end of RFCManager Class
} // end of namespace RFC

#endif

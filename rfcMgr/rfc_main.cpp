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

#include "rfc_common.h"
#include "rfc_manager.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "common_device_api.h"
#include <unistd.h>
#include <signal.h>

static char* g_lockFile = RFC_MGR_SERVICE_LOCK_FILE;

static void cleanup_handler(int signum) {
    // Remove lock file on termination
    unlink(g_lockFile);
    exit(0);
}

int main()
{
    pid_t pid;
    pid = fork(); // Fork off the parent process
    
    if (pid < 0)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC:Fork Failed.\n", __FUNCTION__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    signal(SIGTERM, cleanup_handler);
    signal(SIGINT, cleanup_handler);
    signal(SIGABRT, cleanup_handler);

    rfc::RFCManager* rfcMgr = new rfc::RFCManager();
    if (!rfcMgr->CreateLockFile()) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "Failed to create lock file, exiting");
        delete rfcMgr;
        return 1;
    }

    /* Abort if another instance of rfcMgr is already running */
    if (rfcMgr->IsInstanceAlreadyRunning())
    {
#if !defined(RDKB_SUPPORT)
        rfcMgr->SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_INPROGRESS);
#endif
        delete rfcMgr;
        unlink(g_lockFile);
        return 1;
    }

    rfc::DeviceStatus isDeviceOnline = rfcMgr->CheckDeviceIsOnline();
    
    if (isDeviceOnline == rfc::RFCMGR_DEVICE_ONLINE) 
    {
        int status = FAILURE;
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC:Device is Online\n", __FUNCTION__, __LINE__);
        status = rfcMgr->RFCManagerProcessXconfRequest();
        if(status == SUCCESS)
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC:Xconf Request Processed successfully\n", __FUNCTION__, __LINE__);  
	    // Schedule reboot if needed
            if (rfcMgr->IsRebootRequired()) {
                rfcMgr->ScheduleReboot();
            }
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFC:Device is Offline\n", __FUNCTION__, __LINE__);
    }

    delete rfcMgr;

    return 0;
}
#ifdef __cplusplus
}
#endif

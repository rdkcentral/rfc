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

#include "rfc_manager.h"
#include "rfc_common.h"
#include "rfc_mgr_iarm.h"
#include "rfc_xconf_handler.h"
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace rfc {
#if defined(USE_IARMBUS)
    void rfcMgrEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len) 
    {
        /* This is place holder for future */
        owner = owner;
        eventId = eventId;
        data = data;
        len = len;
        return;
    }
#endif
    RFCManager ::RFCManager() {
        /* Initialize RDK Logger */
        rdk_logger_init(0 == access("/opt/debug.ini", R_OK) ? "/opt/debug.ini" : "/etc/debug.ini");

        /* Initialize IARM Bus */
        InitializeIARM();
    }

    /** Description: Check if Module is connected to IARM
     *
     *  @param cur_event_name: event name.
     *  @param event_status: Status Of the event.
     *  @return void.
     */
    bool RFCManager ::IsIarmBusConnected() {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        int isRegistered = 0;
        res = IARM_Bus_IsConnected(IARM_RFCMANAGER_EVENT, &isRegistered);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_IsConnected: %d (%d)", __FUNCTION__, __LINE__, res, isRegistered);

        if (isRegistered == 1) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
#else
	return true;
#endif
    }

    void RFCManager ::InitializeIARM(void) {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        if (IsIarmBusConnected()) 
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM already connected\n",__FUNCTION__, __LINE__);
        } 
        else 
        {
            res = IARM_Bus_Init(IARM_RFCMANAGER_EVENT);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Init: %d\n", __FUNCTION__, __LINE__, res);
            if (res == IARM_RESULT_SUCCESS || res == IARM_RESULT_INVALID_STATE) {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] SUCCESS: IARM_Bus_Init done!\n", __FUNCTION__, __LINE__);

                res = IARM_Bus_Connect();
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Connect: %d\n", __FUNCTION__, __LINE__, res);
                if (res == IARM_RESULT_SUCCESS || res == IARM_RESULT_INVALID_STATE) 
                {
                    bool result = IsIarmBusConnected();
                    if(result)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM connected\n", __FUNCTION__,__LINE__);
                    }
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] SUCCESS: IARM_Bus_Connect done!\n", __FUNCTION__,__LINE__);
                } 
                else 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Connect failure: %d\n", __FUNCTION__,__LINE__, res);
                }
            } else {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Init failure: %d\n", __FUNCTION__,__LINE__, res);
            }

            res = IARM_Bus_RegisterEventHandler(IARM_BUS_RDKVRFC_MGR_NAME, IARM_BUS_RDK_RFCMGR_DWNLD_CONFIGURATION, rfcMgrEventHandler);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_RegisterEventHandler ret=%d\n", __FUNCTION__,__LINE__, res);

        }
#endif
    }

    /** Description: This API UnRegister IARM event handlers in order to release
     * bus-facing resources.
     *
     *  @param void
     *  @return 0.
     */
    int term_event_handler(void) {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        res = IARM_Bus_UnRegisterEventHandler(IARM_BUS_RDKVRFC_MGR_NAME, IARM_BUS_RDK_RFCMGR_DWNLD_CONFIGURATION);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully terminated all event handlers:%d\n", __FUNCTION__,__LINE__, res);
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
#endif
        return 0;
    }

    std::string RFCManager::getErouterIPAddress() {
        std::string address;

        // Try to get IPv6 address first
        FILE* pipe = popen("dmcli eRT retv Device.DeviceInfo.X_COMCAST-COM_WAN_IPv6", "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                address = buffer;
                // Trim trailing newline
                if (!address.empty() && address.back() == '\n') {
                    address.pop_back();
                }
            }
            pclose(pipe);
        }

        // If IPv6 not available, get IPv4
        if (address.empty()) {
            FILE* pipe = popen("dmcli eRT retv Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "r");
            if (pipe) {
                char buffer[128] = {0};
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    address = buffer;
                    // Trim trailing newline
                    if (!address.empty() && address.back() == '\n') {
                        address.pop_back();
                    }
                }
                pclose(pipe);
            }
        }

        return address;
    }

    bool RFCManager::CheckIPConnectivity(void)
    {
        bool ip_status = false;

        // Call the getErouterIPAddress function directly instead of using the shell script
        std::string ip_address = getErouterIPAddress();

        // If we got an IP address (non-empty string), consider it connected
        if (!ip_address.empty()) {
            ip_status = true;
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully got eRouter IP address\n", __FUNCTION__, __LINE__);
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get eRouter IP address\n", __FUNCTION__, __LINE__);
        }

        // Log the IP address and connection status
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] eRouter IP Address: '%s'\n", __FUNCTION__, __LINE__, ip_address.c_str());
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] IP Connectivity Status: %s\n", __FUNCTION__, __LINE__, ip_status ? "Connected" : "Disconnected");

        return ip_status;
    }

    /* Description: Checking IP route address and device is online or not.
     *              Use IARM event provided by net service manager to check either
     *              device is online or not.
     * @param: file_name : pointer to gateway iproute config file name
     * return :true = success
     * return :false = failure */
    bool RFCManager ::CheckIProuteConnectivity(const char *file_name) {
        bool ip_status = false;
        bool string_check = false;
        FILE *fp = NULL;
        char tbuff[80] = {0};
        char *tmp;
        int IpRouteCnt = 5;

        if (file_name == NULL) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Incorrect File Name", __FUNCTION__,__LINE__);
            return ip_status;
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] CheckIPRoute Waiting for Route Config %s file\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
        while (IpRouteCnt--) 
        {
            if (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG))) {
                break;
            }
            sleep(15);
        }
        if (IpRouteCnt == 0 && (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG)))) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] route flag=%s not present\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
            return ip_status;
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] CheckIPRoute Received Route Config file\n", __FUNCTION__,__LINE__);
        fp = fopen(file_name, "r");
        if (fp != NULL) 
        {
            while (NULL != (fgets(tbuff, sizeof(tbuff), fp))) 
            {
                if (NULL != (strstr(tbuff, "IPV"))) {
                    string_check = true;
                    break;
                }
            }
            if (string_check == true) {
                tmp = strstr(tbuff, "IPV");
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ip address=%s\n", __FUNCTION__,__LINE__, tbuff);
                if (tmp != NULL && (NULL != strstr(tmp, "IPV4"))) 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] default router Link Local IPV4 address present=%s\n", __FUNCTION__,__LINE__, tmp);
                } 
                else if (tmp != NULL && (NULL != strstr(tmp, "IPV6"))) 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] default router Link Local IPV6 address present=%s\n", __FUNCTION__,__LINE__ ,tmp);
                } 
                else 
                {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] IP address type does not found\n", __FUNCTION__,__LINE__);
                }
            } 
            else 
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] File %s does not have IP address in proper format\n", __FUNCTION__,__LINE__, file_name);
            }
            fclose(fp);
        } 
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] ip route file:%s not present\n", __FUNCTION__,__LINE__, file_name);
        }
        ip_status = true;

        /*if (true == checkDeviceInternetConnection(RFC_MGR_INTERNET_CHECK_TIMEOUT))
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Device is online\n", __FUNCTION__,__LINE__);
            ip_status = true;
        } 
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Device is not online\n", __FUNCTION__,__LINE__);
            ip_status = false;
        }*/
        return ip_status;
    }

    /* Description: Checking dns nameserver ip is present or not.
     * @param: dns_file_name : pointer to dns config file name
     * return :true = success
     * return :false = failure */
    bool isDnsResolve(const char *dns_file_name)
    {
        bool dns_status = false;
        bool string_check = false;
        FILE *fp = NULL;
        char tbuff[80] = {0};
        char *tmp;
        if (dns_file_name == NULL) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] isDnsResolve(): parameter is NULL\n", __FUNCTION__,__LINE__);
            return dns_status;
        }
        fp = fopen(dns_file_name , "r");
        if (fp != NULL) 
        {
            while (NULL != (fgets(tbuff, sizeof(tbuff), fp))) {
                if (NULL != (strstr(tbuff, "nameserver"))) {
                    string_check = true;
                    break;
                }
            }
            if (string_check == true) 
            {
                tmp = strstr(tbuff, "nameserver");
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns resolve data=%s\n", __FUNCTION__,__LINE__, tbuff);
                if (tmp != NULL) 
                {
                    tmp = tmp + 10;
                    if (*tmp != '\0' && *tmp != '\n') 
                    {
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns nameserver present.\n", __FUNCTION__,__LINE__);
                        dns_status = true;
                    }
                }
            }
            fclose(fp);
        }
        else 
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns resolve file:%s not present\n", __FUNCTION__,__LINE__, dns_file_name);
        }
        return dns_status;
    }

    DeviceStatus RFCManager ::CheckDeviceIsOnline() 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration\n", __FUNCTION__,__LINE__);
        rfc::DeviceStatus result = RFCMGR_DEVICE_OFFLINE;
#if !defined(RDKB_SUPPORT)	
        if (true == CheckIProuteConnectivity(GATEWAYIP_FILE)) 
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration found\n", __FUNCTION__,__LINE__);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking DNS Nameserver configuration\n", __FUNCTION__,__LINE__);
            if (true == (isDnsResolve(DNS_RESOLV_FILE))) 
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] DNS Nameservers are available\n",__FUNCTION__,__LINE__);
                result = RFCMGR_DEVICE_ONLINE;
            } 
            else 
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] DNS Nameservers missing..!!\n",__FUNCTION__,__LINE__);
            }
        } 
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] IP and Route configuration not found...!!\n", __FUNCTION__,__LINE__);
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_ERROR);
        }
#else
        if (true == CheckIPConnectivity()){
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] IP configuration found...\n", __FUNCTION__,__LINE__);
            result = RFCMGR_DEVICE_ONLINE;
        }
        else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] IP configuration not found!!\n", __FUNCTION__,__LINE__);
	}
#endif	
        return result;
    }

    /** Description: Send event to iarm event manager
     *
     *  @param cur_event_name: event name.
     *  @param main_mgr_event: Status Of the event.
     *  @return void.
     */
    void RFCManager::SendEventToMaintenanceManager(const char *cur_event_name, unsigned int main_mgr_event)
    {
        cur_event_name = cur_event_name;
        main_mgr_event = main_mgr_event;
#ifdef EN_MAINTENANCE_MANAGER
        if ( !(strncmp(cur_event_name,"MaintenanceMGR", 14)) ) 
	{
            IARM_Bus_MaintMGR_EventData_t infoStatus;
	    IARM_Result_t ret_code = IARM_RESULT_SUCCESS;

            memset( &infoStatus, 0, sizeof(IARM_Bus_MaintMGR_EventData_t) );
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] >>>>> Identified MaintenanceMGR with event value=%u\n", __FUNCTION__,__LINE__, main_mgr_event);
            infoStatus.data.maintenance_module_status.status = (IARM_Maint_module_status_t)main_mgr_event;
            ret_code=IARM_Bus_BroadcastEvent(IARM_BUS_MAINTENANCE_MGR_NAME,(IARM_EventId_t)IARM_BUS_MAINTENANCEMGR_EVENT_UPDATE, (void *)&infoStatus, sizeof(infoStatus));
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] >>>>> IARM %s  Event  = %d\n",__FUNCTION__,__LINE__ , (ret_code == IARM_RESULT_SUCCESS) ? "SUCCESS" : "FAILURE",infoStatus.data.maintenance_module_status.status);
        }
#endif
    }

    int RFCManager::RFCManagerPostProcess()
    {
        // Check if the script exists before executing it
        if (access(RFC_MGR_IPTBLE_INIT_SCRIPT, F_OK) == 0) {
            if(-1 == v_secure_system("sh %s", RFC_MGR_IPTBLE_INIT_SCRIPT))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Script[%s] Execution Failed ...!!\n", __FUNCTION__, __LINE__, RFC_MGR_IPTBLE_INIT_SCRIPT);
                return FAILURE;
            }
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Script[%s] Executed Successfully\n", __FUNCTION__, __LINE__, RFC_MGR_IPTBLE_INIT_SCRIPT);
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Script[%s] does not exist, skipping execution\n", __FUNCTION__, __LINE__, RFC_MGR_IPTBLE_INIT_SCRIPT);
        }
    
        return SUCCESS;
    }    

    int RFCManager::RFCManagerProcess() 
    {
        /* Create Object for Xconf Handler */
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();

        int reqStatus = FAILURE;
        int RfcRebootCronNeeded = 0;

        /* Initialize xconf Hanlder */
        int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
        if(result == FAILURE)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
	    delete rfcObj;
            return reqStatus;
        }

        result = rfcObj->ProcessRuntimeFeatureControlReq();

#if !defined(RDKB_SUPPORT)	
        if(result == SUCCESS)
        {
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_COMPLETE); 

            bool isRebootRequired = rfcObj->getRebootRequirement();
            if(isRebootRequired == true)
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] RFC: Posting Reboot Required Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_CRITICAL_UPDATE);
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_REBOOT_REQUIRED);
            }
            reqStatus = SUCCESS;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] RFC: Posting RFC Error Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_ERROR);
        }
#endif        
	int post_process_result = RFCManagerPostProcess();
	if(post_process_result == SUCCESS)
	{
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC:Post Processing Successfully Completed\n", __FUNCTION__,__LINE__);
	    checkAndScheduleReboot(RfcRebootCronNeeded);
	}
	else
	{
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC:Post Processing Failed\n", __FUNCTION__,__LINE__);
	}

        delete rfcObj;
        return reqStatus;
    }

    int RFCManager ::RFCManagerProcessXconfRequest() 
    {
        int ret_status = FAILURE;

        ret_status = RFCManagerProcess();

        return ret_status;
    }

    void RFCManager::checkAndScheduleReboot(int rfcRebootCronNeeded) 
    {
        if (rfcRebootCronNeeded == 1) {
            // Effective Reboot is required for the New RFC config
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC: RfcRebootCronNeeded=%d. Calling script to schedule reboot in maintenance window\n", 
                    __FUNCTION__, __LINE__, rfcRebootCronNeeded);
        
            // Check if the script exists
            if (access("/etc/RfcRebootCronschedule.sh", X_OK) == 0) {
                // Use v_secure_system to run the script in background
                int result = v_secure_system("sh /etc/RfcRebootCronschedule.sh &");
            
                if (result != 0) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to execute reboot script, return code: %d\n", 
                        __FUNCTION__, __LINE__, result);
                }
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Reboot script not found or not executable: /etc/RfcRebootCronschedule.sh\n", 
                        __FUNCTION__, __LINE__);
            }
        }
    }    

    void rfc::RFCManager::manageCronJob(const std::string& cron) {	    
        if (!cron.empty()) {
            int cron_update = 1;

            // Parse cron components
            std::istringstream iss(cron);
            std::vector<std::string> cronParts;
            std::string part;
            while (iss >> part) {
                cronParts.push_back(part);
            }

            if (cronParts.size() >= 5) {
                // Get cron parts as integers for calculations
                int vc1 = std::stoi(cronParts[0]);
                int vc2 = std::stoi(cronParts[1]);
                int vc3 = std::stoi(cronParts[2]);
                int vc4 = std::stoi(cronParts[3]);
                int vc5 = std::stoi(cronParts[4]);

                // Adjust time
                if (vc1 > 2) {
                    vc1 -= 3;
                } else {
                    vc1 += 57;
                    if (vc2 == 0) {
                        vc2 = 23;
                    } else {
                        vc2 -= 1;
                    }
                }

                // Reconstruct cron string
                std::ostringstream cronStream;
                cronStream << vc1 << " " << vc2 << " " << vc3 << " " << vc4 << " " << vc5;
                std::string adjustedCron = cronStream.str();

                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC_TM_Track : Configuring cron job for rfcMgr, cron = %s\n",
                       __FUNCTION__, __LINE__, adjustedCron.c_str());

                // Get environment variables
                std::string RFC_LOG_FILE = std::getenv("RFC_LOG_FILE") ? std::getenv("RFC_LOG_FILE") : "";

                // Define the current cron file path
                std::string current_cron_file = "/tmp/cron_tab_tmp_file";

                // Check if cronjobs_update.sh exists
                bool cronjobsUpdateExists = (access("/lib/rdk/cronjobs_update.sh", F_OK) == 0);

    #if defined(RDKB_SUPPORT)
                std::string crontabPath = "/var/spool/cron/crontabs/";
    #else
                std::string crontabPath = "/var/spool/cron/";
    #endif

                if (!cronjobsUpdateExists) {
                    // Export existing crontab
                    std::string cmd = "crontab -l -c " + crontabPath + " > " + current_cron_file;
                    int result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to export existing crontab\n", __FUNCTION__, __LINE__);
                    }

                    // Remove existing rfcMgr entries (both rfcMgr and RFCbase.sh for backward compatibility)
                    cmd = "sed -i '/[A-Za-z0-9]*rfcMgr[A-Za-z0-9]*/d' " + current_cron_file;
                    result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to remove existing rfcMgr entries\n", __FUNCTION__, __LINE__);
                    }

                    cmd = "sed -i '/[A-Za-z0-9]*RFCbase.sh[A-Za-z0-9]*/d' " + current_cron_file;
                    result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to remove existing RFCbase.sh entries\n", __FUNCTION__, __LINE__);
                    }
                }

                // Add new cron entry
                if (!cronjobsUpdateExists) {
                    std::ofstream outFile(current_cron_file, std::ios::app);
                    if (outFile) {
                        // Get timestamp
                        std::string timestamp_cmd = "/bin/timestamp";
                        FILE* pipe = popen(timestamp_cmd.c_str(), "r");
                        std::string timestamp;
                        if (pipe) {
                            char buffer[128];
                            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                                timestamp = buffer;
                                if (!timestamp.empty() && timestamp.back() == '\n') {
                                    timestamp.pop_back();
                                }
                            }
                            pclose(pipe);
                        }

                        outFile << timestamp << " " << adjustedCron << " /usr/bin/rfcMgr >> "
                               << RFC_LOG_FILE << " 2>&1" << std::endl;
                        outFile.close();

                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Added new cron entry for rfcMgr to temporary file\n", __FUNCTION__, __LINE__);
                    } else {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open temporary cron file for writing\n", __FUNCTION__, __LINE__);
                    }
                } else {
                    std::string cmd = "sh /lib/rdk/cronjobs_update.sh \"update\" \"rfcMgr\" \""
                                    + adjustedCron + " /usr/bin/rfcMgr >> "
                                    + RFC_LOG_FILE + " 2>&1\"";
                    int result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to update cron job using cronjobs_update.sh\n", __FUNCTION__, __LINE__);
                    } else {
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully updated cron job using cronjobs_update.sh\n", __FUNCTION__, __LINE__);
                    }
                }

                // Apply the new crontab if needed
                if (!cronjobsUpdateExists && cron_update == 1) {
                    std::string cmd = "crontab " + current_cron_file + " -c " + crontabPath;
                    int result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to apply updated crontab\n", __FUNCTION__, __LINE__);
                    } else {
                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Successfully applied updated crontab\n", __FUNCTION__, __LINE__);
                    }
                }

                // Log cron configuration
                std::string logCmd = "/bin/timestamp > " + RFC_LOG_FILE + " && echo ' " + crontabPath + "root:' >> " + RFC_LOG_FILE;
                v_secure_system("%s", logCmd.c_str());

                if (!cronjobsUpdateExists) {
                    std::string cmd = "/bin/timestamp > " + RFC_LOG_FILE + " && cat " + current_cron_file + " >> " + RFC_LOG_FILE;
                    int result = v_secure_system("%s", cmd.c_str());
                    if (result != 0) {
                        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to log crontab contents\n", __FUNCTION__, __LINE__);
                    }
                }

                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Completed cron job configuration for rfcMgr\n", __FUNCTION__, __LINE__);
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid cron format: not enough components\n", __FUNCTION__, __LINE__);
            }
        } else {
            RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] Empty cron string provided, no action taken\n", __FUNCTION__, __LINE__);
        }
    }
} // namespace RFC


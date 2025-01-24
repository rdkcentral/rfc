/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
 
/**
* @file libIBusDaemon.h
*
* @brief IARM-Bus Daemon API
*
* This API defines the public operations for IARM bus Daemon.
*
* @par Document
* Document reference.
*
* @par Open Issues (in no particular order)
* -# None
*
* @par Assumptions
* -# None
*
* @par Abbreviations
* - BE:       ig-Endian.
* - cb:       allback function (suffix).
* - DS:      Device Settings.
* - FPD:     Front-Panel Display.
* - HAL:     Hardware Abstraction Layer.
* - LE:      Little-Endian.
* - LS:      Least Significant.
* - MBZ:     Must be zero.
* - MS:      Most Significant.
* - RDK:     Reference Design Kit.
* - _t:      Type (suffix).
*
* @par Implementation Notes
* -# None
*
*/

/** @defgroup IARM_BUS IARM-Bus Daemon
*   @ingroup IARM_RDK
*
* 
*/

/** @addtogroup IARM_BUS_DAEMON_API IARM-BUS Daemon API.
*  @ingroup IARM_BUS
*
* Described herein are the IARM Bus daemon types and functions.
* This includes 
* <br> 1) Calls and data structures for requesting and releasing resources through IARM
* <br> 2) PreChange calls for power and resolution changes, which needs 
*         to be implemented by members if it requires some action to be done
*         before these changes.
*   
*
*  @{
*/

#ifndef _IARM_BUS_DAEMON_H
#define _IARM_BUS_DAEMON_H


#ifdef __cplusplus
extern "C" 
{
#endif
#define IARM_BUS_DAEMON_NAME 						"Daemon"  /*!< IARM bus name for Daemon */

/*! Daemon events enumeration */
typedef enum _IARM_Bus_Daemon_EventId_t {
    IARM_BUS_EVENT_RESOURCEAVAILABLE, /*!< Resource available event */
    IARM_BUS_EVENT_RESOLUTIONCHANGE,  /*!< Resolution change event */
    IARM_BUS_SIGNAL_MAX,              /*!< Maximum value for IARM daemon event */
} IARM_Bus_EventId_t;

/*! Type of resources acknowledge through on Resource available event */
typedef enum _IARM_Bus_Daemon_ResrcType_t {
    IARM_BUS_RESOURCE_FOCUS = 0,
    IARM_BUS_RESOURCE_DECODER_0,
    IARM_BUS_RESOURCE_DECODER_1,
    IARM_BUS_RESOURCE_PLANE_0,
    IARM_BUS_RESOURCE_PLANE_1,
    IARM_BUS_RESOURCE_POWER,
    IARM_BUS_RESOURCE_RESOLUTION,

    IARM_BUS_RESOURCE_MAX
} IARM_Bus_ResrcType_t;

/*! Data assosiated with Resolution change */
typedef struct _IARM_Bus_ResolutionChange_EventData_t{
    
    int width;                     /*!< new width after resoution change */
    int height;                    /*!< new height after resoution change */    
	
} IARM_Bus_ResolutionChange_EventData_t;

/*! Union of IARM Bus event, data could be resource type 
 * or new resolution based on the event type*/
typedef union _IARM_Bus_EventData_t {
	IARM_Bus_ResrcType_t resrcType;                /*!< Resource type, which will be valid if event is resource available*/
    IARM_Bus_ResolutionChange_EventData_t resolution;  /*!< New resolution, which will be valid if event is resolution change*/
     
} IARM_Bus_EventData_t;

/*! Data associated with Resolution Post-Change call*/
typedef struct _IARM_Bus_CommonAPI_ResChange_Param_t{
	int width;                                    /*!< Current resolution width */
	int height;                                   /*!< Current resolution height */ 
}IARM_Bus_CommonAPI_ResChange_Param_t;

/*! Possible power states */
typedef enum _IARM_Bus_Daemon_PowerState_t {
    IARM_BUS_PWRMGR_POWERSTATE_OFF, /*!< Used for both IARM Bus Daemon Power pre change and Power manager Power state OFF */
    IARM_BUS_PWRMGR_POWERSTATE_STANDBY, /*!< Used for both IARM Bus Daemon Power pre change and Power manager Power state STANDBY */
    IARM_BUS_PWRMGR_POWERSTATE_ON,  /*!< Used for both IARM Bus Daemon Power pre change and Power manager Power state ON */
    IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP, /*!< Used for both IARM Bus Daemon Power pre change and Power manager Power state set/get */
    IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP   /*!< Used for both IARM Bus Daemon Power pre change and Power manager Power state set/get */
} IARM_Bus_PowerState_t;

typedef IARM_Bus_PowerState_t IARM_Bus_PWRMgr_PowerState_t;

/*! Data associated with Power Pre-Change call*/
typedef struct _IARM_Bus_CommonAPI_PowerPreChange_Param_t{
    IARM_Bus_PWRMgr_PowerState_t  newState;  /*!< New power state*/  
    IARM_Bus_PWRMgr_PowerState_t  curState;  /*!< Current power state*/
} IARM_Bus_CommonAPI_PowerPreChange_Param_t;


typedef struct _IARM_Bus_PowerPreChange_Param_t{
    IARM_Bus_PWRMgr_PowerState_t  newState;  /*!< New power state*/
    IARM_Bus_PWRMgr_PowerState_t  curState;  /*!< Current power state*/
    char owner[IARM_MAX_NAME_LEN];
} IARM_Bus_PowerPreChange_Param_t;

/*! Data associated with Release ownership call*/
typedef struct _IARM_Bus_CommonAPI_ReleaseOwnership_Param_t {
    IARM_Bus_ResrcType_t resrcType;        /*!< Type of resource to be freed*/     
} IARM_Bus_CommonAPI_ReleaseOwnership_Param_t;


/*! Type of Sys modes available*/
typedef enum _IARM_Bus_Daemon_SysMode_t{
    IARM_BUS_SYS_MODE_NORMAL,
    IARM_BUS_SYS_MODE_EAS,
    IARM_BUS_SYS_MODE_WAREHOUSE
} IARM_Bus_Daemon_SysMode_t;

/*! data associated with Sys mod-change call*/
typedef struct _IARM_Bus_CommonAPI_SysModeChange_Param_t{
    IARM_Bus_Daemon_SysMode_t oldMode;                    
    IARM_Bus_Daemon_SysMode_t newMode;                   
}IARM_Bus_CommonAPI_SysModeChange_Param_t;


/**
 * @brief Request to grab resource
 *
 * Ask IARM Daemon to grant resoruce. Upon the success return, the resource is
 * available to use.
 *
 * @param [in] resrcType: Resource type.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_RequestOwnership(IARM_Bus_ResrcType_t resrcType);

/**
 * @brief Notify IARM Daemon that the resource is released.
 *
 * Upon success return, this client is no longer the owner of the resource.
 * A resource free event will be broadcasted and some other client intereseted in
 * freed resource can request the resource now.
 *
 * @param [in] resrcType: Resource type.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_ReleaseOwnership(IARM_Bus_ResrcType_t resrcType);

/**
 * @brief Send power pre change command
 *
 * Command is broadcasted before power change and all those modules implementing 
 * power prechange function will be called. 
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_PowerPrechange(IARM_Bus_CommonAPI_PowerPreChange_Param_t preChangeParam);

/**
 * @brief Send resolution pre change command
 *
 * Command is broadcasted before resolution change and all those modules implementing 
 * resolution prechange function will be called. 
 *
 * @param [in] preChangeParam: height, width etc.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_ResolutionPrechange(IARM_Bus_CommonAPI_ResChange_Param_t preChangeParam);

/**
 * @brief Send resolution post change command
 *
 * Command is broadcasted after resolution change and all those modules implementing 
 * resolution postchange handler will be called. 
 *
 * @param [in] postChangeParam: height, width etc.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_ResolutionPostchange(IARM_Bus_CommonAPI_ResChange_Param_t postChangeParam);


/**
 * @brief Send Deep Sleep Wakeup command
 *
 * Command is broadcasted to Deep sleep Manager for wakeup from deep sleep.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_BusDaemon_DeepSleepWakeup(IARM_Bus_CommonAPI_PowerPreChange_Param_t preChangeParam);



#define IARM_BUS_COMMON_API_ReleaseOwnership 		"ReleaseOwnership" /*!< This method shall be implemented by all member requests for resources*/


#define IARM_BUS_COMMON_API_PowerPreChange 		"PowerPreChange"     /*!< This method if implemented by a member, will be called before a power change event*/

#define IARM_BUS_COMMON_API_DeepSleepWakeup          "DeepSleepWakeup"     /*!< This method if implemented by a member, will be called to wakeup from Deep Sleep */


#define IARM_BUS_COMMON_API_ResolutionPreChange		"ResolutionPreChange" /*!< This method if implemented by a member, will be called before a resolution change*/

#define IARM_BUS_COMMON_API_ResolutionPostChange     "ResolutionPostChange" /*!< This method if implemented by a member, will be called after  a resolution change*/


#define IARM_BUS_COMMON_API_SysModeChange               "SysModeChange" /*!< This method if implemented by a member, will be called on a Sys mode change*/

#ifdef __cplusplus
}
#endif
#endif

/* End of IARM_BUS_DAEMON_API doxygen group */
/**
 * @}
 */

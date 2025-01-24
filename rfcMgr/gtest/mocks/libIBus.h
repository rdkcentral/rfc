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
* @file libIBus.h
*
* @brief RDK IARM-Bus API Declarations.
*
* @defgroup IARMBUS_API IARM Bus API
* @ingroup IARMBUS
* Application should use the APIs declared in this file to access
* services provided by IARM-Bus. Basically services provided by
* these APIs include:
* <br> 1) Library Initialization and termination.
* <br> 2) Connection to IARM-Bus.
* <br> 3) Send and Receive Events.
* <br> 4) Declared and Invoke RPC Methods.
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

/** @defgroup IARMBUS IARM Bus
*
*  IARM-Bus is a platform agnostic Inter-process communication (IPC) interface. It allows
*  applications to communicate with each other by sending Events or invoking Remote
*  Procedure Calls. The common programming APIs offered by the RDK IARM-Bus interface is
*  independent of the operating system or the underlying IPC mechanism.
*
*  Two applications connected to the same instance of IARM-Bus are able to exchange events
*  or RPC calls. On a typical system, only one instance of IARM-Bus instance is needed. If
*  desired, it is possible to have multiple IARM-Bus instances. However, applications
*  connected to different buses will not be able to communicate with each other.
*
* @par Capabilities
* <ol>
* <li> Invoke methods in other processes via Remote Procedure Call (RPC).
* <li> Send interprocess messages.
* <li> Manage shared memory and exclusive access to resources.
* <li> Register for event notification.
* <li> Publish event notification to registered listeners.
* </ol>
*/

#ifndef _LIB_IARM_BUS_H
#define _LIB_IARM_BUS_H



#ifdef __cplusplus
extern "C" 
{
#endif

#include "libIARM.h"

/**
 * @brief Function signature for RPC Methods.
 *
 * All IARM RPC Methods must use <tt>IARM_BusCall_t</tt> as their function signature.
 * Important Note: The argument structure cannot have pointers. The sizeof() operator applied
 * on the @p arg must equal to the actual memory allocated.  Internally an equivalent of
 * memcpy() is used to dispatch parameters its target. If a pointer is used in the parameters,
 * the pointer, not the content it points to, is sent to the destination.
 *
 *  @param arg is the composite carrying all input and output parameters of the RPC Method.
 *
 */
typedef IARM_Result_t (*IARM_BusCall_t) (void *arg);
/**
 * @brief Function signature for event handlers.
 *
 * All IARM Event handlers must use <tt>IARM_EventHandler_t</tt> as their function signature.
 * Important Note: The event data structure cannot have pointers. The sizeof() operator applied
 * on the @p data must equal to the actual memory allocated.  Internally an equivalent of
 * memcpy() is used to dispatch event data to its target. If a pointer is used in the event data,
 * the pointer, not the content it points to, is sent to the destination.
 *
 * @param owner is well-known name of the application sending the event.
 * @param eventID is the integer uniquely identifying the event within the sending application.
 * @param data is the composite carrying all input and output parameters of event.
 * @param len is the result of sizeof() applied on the event data data structure.
 * 
 * @return None
 */
typedef void (*IARM_EventHandler_t)(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

/**
  * @addtogroup IARMBUS_API
  * @{
  */

/**
 * @brief This API is used to initialize the IARM-Bus library.
 *
 * The registered IARM client is uniquely identified by the given name in the IARM_Bus_Init() function.
 * The application is not yet connected to the bus until IARM_Bus_Connect() is called.
 *
 * After the library is initialized, the application is ready to access events and RPC methods
 * using the bus.
 *
 * @param[in] name A well-known name of the IARM client. The registered IARM client
 * should be uniquely identified by (groupName, memberName)
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates the call was unsuccessful because the bus is
 * already initialised and connected.
 */
IARM_Result_t IARM_Bus_Init(const char *name);

/**
 * @brief This API is used to terminate the IARM-Bus library.
 *
 * This function releases resources allocated by the IARM Bus Library. After it is called,
 * the library returns to the state prior to IARM_Bus_Init function is called.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates the call was unsuccessful because the bus is
 * not initialised.
 */
IARM_Result_t IARM_Bus_Term(void);

/**
 * @brief This API is used to connect application to the IARM bus daemon.
 * After connected, the application can send/receive IARM events and invoke IARM RPC calls.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_STATE Indicates the call was unsuccessful.
 */
IARM_Result_t IARM_Bus_Connect(void);

/**
 * @brief This API disconnect Application from IARM Bus so the application will not receive
 * any IARM event or RPC calls.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_STATE Indicates the call was unsuccessful.
 */
IARM_Result_t IARM_Bus_Disconnect(void);

/**
 * @brief Returns group context of the calling member
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_GetContext(void **context);

/**
 * @brief This API is used to publish an Asynchronous event to all IARM client registered for this
 * perticular event. Upon returns of this function, all the listeners are notified of the event.
 *
 * @param[in] ownerName The IARM client that publishes/owns the broadcast event.
 * @param[in] eventId The event id to publish.
 * @param[in] data Data carried by this event.
 * @param[in] len Length of the data parameter.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_STATE Indicates the call was unsuccessful because the bus is either
 * not initialised nor connected.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid parameter.
 */
IARM_Result_t IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len);

/**
 * @brief This API is used to check if the current process is registered with IARM.
 *
 * @param[in] memberName IARMBUS member whose registration status has to be checked.
 * @param[out] isRegistered True if the specified process is still registered.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid input parameter.
 * @retval IARM_RESULT_IPCCORE_FAIL Indicates failure of the underlying IPC.
 */
IARM_Result_t IARM_Bus_IsConnected(const char *memberName, int *isRegistered);

/**
 * @brief This API register to listen to event and provide the callback function for event notification.
 * Execution of the handler will not block the process sending the event.
 *
 *  The API checks for duplicate handlers so a same handler for same event and owner name will not be registered twice 
 *	NULL handler is not allowed.
 *
 * @param[in] ownerName The well-known name of the IARM client.
 * @param[in] eventId The event to listen for.
 * @param[in] handler The hander function to be called for  event notification. 
 *
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid input parameters.
 * @retval IARM_RESULT_INVALID_STATE Indicates the IARM_Bus is either not initialised nor connected.
 * @retval IARM_RESULT_IPCCORE_FAIL Indicates failure of the underlying IPC.
 * @retval IARM_RESULT_OOM Indicates memory allocation failure.
 * @see IARM_Bus_BroadcastEvent()
 * @see IARM_EventHandler_t
 */
IARM_Result_t IARM_Bus_RegisterEventHandler(const char *ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler);

/**
 * @brief This API is used to Remove ALL handlers registered for the given event.
 * This API remove the all the event handlers. This API is not used to unregister a specific handler..

 * @param[in] eventId The event whose listener to be removed.
 * @param[in] ownerName The well-known name of the application.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid input parameter was passed.
 * @retval IARM_RESULT_INVALID_STATE Indicates the IARM_Bus is either not initialised nor connected.
 * @retval IARM_RESULT_IPCCORE_FAIL Indicates underlying IPC failure.
 * @retval IARM_RESULT_OOM Indicates memory allocation failure.
 */
IARM_Result_t IARM_Bus_UnRegisterEventHandler(const char *ownerName, IARM_EventId_t eventId);

/**
 * @brief Remove specific handler  registered for the given event.
 *
 * This API remove the specific handlers.   
 * @param[in] ownerName The well-known name of the application.
 * @param [in] eventId The event whose listener to remove.
 * @param [in] handler The event handler to remove.
 *
 * @return IARM_Result_t Error Code.
 */
IARM_Result_t IARM_Bus_RemoveEventHandler(const char *ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler);



/**
 * @brief This API is used to register an RPC method that can be invoked by other applications.
 *
 * The parameter methodName is the string name used to invoke the RPC method and the parameter handler
 * is the implementation of the RPC method. When other application invokes the method via its string name,
 * the function pointed to by the handler is executed.
 *
 * @param[in] methodName The name used to invoke the RPC method.
 * @param[in] handler A pointer to RPC method implementation.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid parameter.
 * @retval IARM_RESULT_INVALID_STATE Indicates IARM_Bus is either not initialised nor connected.
 * @retval IARM_RESULT_OOM Indicates memory allocation failure.
 */
IARM_Result_t IARM_Bus_RegisterCall(const char *methodName, IARM_BusCall_t handler);

/**
 * @brief This API is used to Invoke RPC method by its application name and method name.
 *
 * @param[in] ownerName well-known name of the application that publish the RPC call.
 * @param[in] methodName well-known name of the RPC method.
 * @param[in] arg It is the data structure holding input & output parameters of the invocation.
 * @param[in] argLen The size of the data pointed by arg parameter.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid input parameter.
 * @retval IARM_RESULT_INVALID_STATE Indicates the IARM_Bus was either not initialised nor connected.
 * @retval IARM_RESULT_IPCCORE_FAIL Indicates failure of the underlying IPC.
 * @retval IARM_RESULT_OOM Indicates failure to allocate memory.
 */
IARM_Result_t IARM_Bus_Call(const char *ownerName,  const char *methodName, void *arg, size_t argLen);

/**
 * @brief This API is used to Invoke RPC method by its application name and method
 * name with specified timeout to wait for response.
 *
 * @param[in] ownerName well-known name of the application that publish the RPC call.
 * @param[in] methodName well-known name of the RPC method.
 * @param[in] arg It is the data structure holding input & output parameters of the invocation.
 * @param[in] argLen The size of the data pointed by arg parameter.
 * @param[in] timeout in millisecond for the RPC method.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_PARAM Indicates invalid input parameter.
 * @retval IARM_RESULT_INVALID_STATE Indicates the IARM_Bus was either not initialised nor connected.
 * @retval IARM_RESULT_IPCCORE_FAIL Indicates failure of the underlying IPC.
 * @retval IARM_RESULT_OOM Indicates failure to allocate memory.
 */
IARM_Result_t IARM_Bus_Call_with_IPCTimeout(const char *ownerName,  const char *methodName, void *arg, size_t argLen, int timeout);

/**
 * @brief This API is used to register all the events that are published by the application.
 *
 * An application can publish multiple events and these events must have an enumeration value
 * defined in the public header file of the HAL.
 * It registers all events whose enum value is less than maxEventId.
 *
 * @param[in] maxEventId The maximum number of events that can be registered.
 *
 * @return Error Code.
 * @retval IARM_RESULT_SUCCESS Indicates the call was successful.
 * @retval IARM_RESULT_INVALID_STATE Indicates the IARM Bus is either not initialised nor connected.
 */
IARM_Result_t IARM_Bus_RegisterEvent(IARM_EventId_t maxEventId);

/**
 * @brief Write PID file
 *
 * This API allows Daemon to write PID file
 *
 * @param full pathname to pidfile to write
 */
void IARM_Bus_WritePIDFile(const char *path);


/* End of IARM_BUS_IARM_CORE_API doxygen group */
/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif


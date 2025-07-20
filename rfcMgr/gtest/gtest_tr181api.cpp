/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
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

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <string>

#include "tr181api.h"
#include "tr181_store_writer.h"

using namespace std;

#define TR181_LOCAL_STORE_FILE "/opt/secure/RFC/tr181localstore.ini"


TEST(tr181apiTest, getTR181ErrorString) {
   EXPECT_STREQ(getTR181ErrorString(tr181Success) , " Success");
   EXPECT_STREQ(getTR181ErrorString(tr181InternalError) , " Internal Error");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidParameterName), " Invalid Parameter Name");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidParameterValue), " Invalid Parameter Value");
   EXPECT_STREQ(getTR181ErrorString(tr181Failure), " Failure");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidType), " Invalid type");
   EXPECT_STREQ(getTR181ErrorString(tr181NotWritable), " Not writable");
   EXPECT_STREQ(getTR181ErrorString(tr181ValueIsEmpty), " Value is empty");
   EXPECT_STREQ(getTR181ErrorString(tr181ValueIsNull), " Value is Null");
   EXPECT_STREQ(getTR181ErrorString(tr181DefaultValue), " Default Value");
}

TEST(tr181apiTest, getType) {
    EXPECT_EQ(getType(WDMP_STRING), TR181_STRING);
    EXPECT_EQ(getType(WDMP_INT), TR181_INT);
    EXPECT_EQ(getType(WDMP_UINT), TR181_UINT);
    EXPECT_EQ(getType(WDMP_BOOLEAN), TR181_BOOLEAN);
    EXPECT_EQ(getType(WDMP_DATETIME), TR181_DATETIME);
    EXPECT_EQ(getType(WDMP_BASE64), TR181_BASE64);
    EXPECT_EQ(getType(WDMP_LONG), TR181_LONG);
    EXPECT_EQ(getType(WDMP_ULONG), TR181_ULONG);
    EXPECT_EQ(getType(WDMP_FLOAT), TR181_FLOAT);
    EXPECT_EQ(getType(WDMP_DOUBLE), TR181_DOUBLE);
    EXPECT_EQ(getType(WDMP_BYTE), TR181_BYTE);
    EXPECT_EQ(getType(WDMP_NONE), TR181_NONE);
}


TEST(tr181apiTest, getErrorCode) {
   tr181ErrorCode_t errorCode = getErrorCode(WDMP_ERR_DEFAULT_VALUE);
   EXPECT_EQ(getErrorCode(WDMP_SUCCESS), tr181Success);
   EXPECT_EQ(getErrorCode(WDMP_FAILURE), tr181Failure);
   EXPECT_EQ(getErrorCode(WDMP_ERR_TIMEOUT), tr181Timeout);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_NAME), tr181InvalidParameterName);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_VALUE), tr181InvalidParameterValue);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_TYPE), tr181InvalidType);
   EXPECT_EQ(getErrorCode(WDMP_ERR_NOT_WRITABLE), tr181NotWritable);
   EXPECT_EQ(getErrorCode(WDMP_ERR_VALUE_IS_EMPTY), tr181ValueIsEmpty);
   EXPECT_EQ(getErrorCode(WDMP_ERR_VALUE_IS_NULL), tr181ValueIsNull);
   EXPECT_EQ(getErrorCode(WDMP_ERR_DEFAULT_VALUE), tr181DefaultValue);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INTERNAL_ERROR), tr181InternalError);
}

TEST(tr181apiTest, getDefaultValue) {
  writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable", "false", "/etc/rfcdefaults/rfcdefaults.ini", Plain);
  const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable";
  char *pcCallerID ="rfcdefaults";
  TR181_ParamData_t pstParamData;

  tr181ErrorCode_t status =  getDefaultValue(pcCallerID,pcParameterName,&pstParamData);
  EXPECT_EQ(status, tr181Success);
  EXPECT_EQ(pstParamData.value, false) 

}

TEST(tr181apiTest, getDefaultValue_callerIDNULL) {
  const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable";
  char *pcCallerID = NULL;
  TR181_ParamData_t pstParamData;

  tr181ErrorCode_t status =  getDefaultValue(pcCallerID,pcParameterName,&pstParamData);
  EXPECT_EQ(status, tr181Failure);

}

TEST(tr181apiTest, getValue) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable", "true", TR181_LOCAL_STORE_FILE, Plain);
   const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable";
   TR181_ParamData_t pstParam;
   tr181ErrorCode_t status = getValue(TR181_LOCAL_STORE_FILE, pcParameterName, &pstParam);
   EXPECT_EQ(status, tr181Success);
   EXPECT_STREQ(pstParam.value, "true");
}

TEST(tr181apiTest, getEmptyValue) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable", "", TR181_LOCAL_STORE_FILE, Plain);
   const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable";
   TR181_ParamData_t pstParam;
   tr181ErrorCode_t status = getValue(TR181_LOCAL_STORE_FILE, pcParameterName, &pstParam);
   EXPECT_EQ(status, tr181ValueIsEmpty);
}


TEST(tr181apiTest, setValue) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   const char* pcParamValue ="comcast";
   tr181ErrorCode_t status = setValue(pcParameterName, pcParamValue);
   EXPECT_EQ(status, tr181Success);
}

TEST(tr181apiTest, setLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName";
   char *pcCallerID ="rfcdefaults";
   const char* pcParamValue ="Xfinity";
   tr181ErrorCode_t status = setLocalParam(pcCallerID, pcParameterName, pcParamValue);
   EXPECT_EQ(status, tr181Success);
}


TEST(tr181apiTest, clearLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName";
   char *pcCallerID ="rfcdefaults";
   const char* pcParamValue ="Xfinity";
   tr181ErrorCode_t status = clearLocalParam(pcCallerID, pcParameterName);
   EXPECT_EQ(status, tr181Success);
}


TEST(tr181apiTest, getLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   tr181ErrorCode_t status = getLocalParam(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_STREQ(pstParamData.value, "comcast");
   EXPECT_EQ(status, tr181Success);
}


TEST(tr181apiTest, getParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   tr181ErrorCode_t status = getParam(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_EQ(status, tr181Success);
}
/*
TEST(tr181apiTest, getParam_failure) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_FirmwareDownloadProtocol";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   tr181ErrorCode_t status = getParam(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_EQ(status, tr181Failure);
} */

GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}

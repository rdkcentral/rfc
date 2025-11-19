####################################################################################
# If not stated otherwise in this file or this component's Licenses file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################

from rfc_test_helper import *

def test_WebPA_Set_ACC_Id():
    print("Starting parodus mock process")
    payload = '{"command":"SET","parameters":[{"name":"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID","dataType":0,"value":"412370664406228514"}]}'
    command = ["/usr/local/bin/parodus", payload]

    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, f"Command failed with error: {result.stderr}"

    STATUS_CODE_MSG = '"statusCode":200'
    assert STATUS_CODE_MSG in grep_paroduslogs(STATUS_CODE_MSG)

    SUCCESS_STATUS_MSG = '"message":"Success"'
    assert SUCCESS_STATUS_MSG in grep_paroduslogs(SUCCESS_STATUS_MSG)

def test_WebPA_Get_ACC_Id():
    print("Starting parodus mock process")
    payload ='{"command":"GET","names":["Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"]}'
    command = ["/usr/local/bin/parodus", payload]

    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, f"Command failed with error: {result.stderr}"

    STATUS_CODE_MSG = '"statusCode":200'
    assert STATUS_CODE_MSG in grep_paroduslogs(STATUS_CODE_MSG)

    SUCCESS_STATUS_MSG = '"message":"Success"'
    assert SUCCESS_STATUS_MSG in grep_paroduslogs(SUCCESS_STATUS_MSG)

    ACC_ID_MSG = '"value":"412370664406228514"'
    assert ACC_ID_MSG in grep_paroduslogs(ACC_ID_MSG)


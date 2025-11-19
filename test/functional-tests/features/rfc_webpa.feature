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

Feature: WebPA Set Get using mock parodus

  Background:
    Given the mock parodus executable is avaliable at "/usr/local/bin/parodus"
    And it accepts payloads as input


  Scenario: Set a WebPA parameter using mock parodus
    Given a valid WebPA set request payload:
      """
      {
        "command": "SET",
        "parameters": [
          {
            "name": "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID",
            "value": "local",
            "dataType": 0
          }
        ]
      }
      """
    When I execute the mock parodus with the payload
    Then the response should indicate success
    And the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.FWUpdate.AutoExcluded.Enable" should be "local"

  Scenario: Get a WebPA parameter using mock parodus
    Given a valid WebPA get request payload:
      """
      {
        "command": "GET",
        "names": [
          "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
        ]
      }
      """
    When I execute the mock parodus with the payload
    Then the response should indicate success
    And the returned value should be "local"

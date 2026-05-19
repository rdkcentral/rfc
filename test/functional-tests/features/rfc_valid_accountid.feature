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

Feature: Verify RFC valid Account ID handling

  Scenario: RFC XConf request starts with Unknown AccountID and transitions to valid ID
    Given the mockxconf server is running
    When the RFC manager binary is run and queries XConf
    Then a message "&accountId=Unknown&" should be logged in the RFC log
    And a message "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID" should be logged in the RFC log
    And a message "NEW valid Account ID: 3064488088886635972" should be logged in the RFC log
    And a message "&accountId=3064488088886635972&" should be logged in the RFC log

  Scenario: Verify Account ID is persisted in TR181 store after XConf update
    When I query the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID" via tr181 CLI
    Then the returned value should contain "3064488088886635972"


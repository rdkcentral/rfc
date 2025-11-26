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

Feature: Validate RFC API parameter handling
  The RFC API should correctly store and retrieve parameters
  using the setRFCParameter() and getRFCParameter() functions. 

  Background:
    Given the RFC API service is running

  Scenario: Set a parameter successfully
    When I set the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfSelector" to "local"
    Then the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfSelector" should be "local"

  Scenario: Get a parameter successfully
    Given the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfSelector" is set to "local"
    When I get the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfSelector"
    Then I should receive "local"


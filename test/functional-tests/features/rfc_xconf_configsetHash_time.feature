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

Feature: RFC Manager ConfigSetHash and ConfigSetTime Tracking

  Background:
    Given all the properties to run RFC manager is available and running

  Scenario: Set ConfigSetTime value via TR181 CLI
    When I set the parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime" to "1763118860" as uint32
    Then the parameter should be set successfully with "Set operation success"

  Scenario: Baseline XConf communication for configSetHash test
    When the RFC binary is run
    Then the XConf request should complete successfully

  Scenario: Validate configSetHash and configSetTime from XConf response
    When the RFC binary is run
    Then a message "ConfigSetHash: Hash = 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp" should be logged
    And a message "ConfigSetTime: Set Time = " should be logged
    And a message "configSetHash value: 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp" should be logged
    And a message "Config Set Hash = 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp" should be logged
    And a message "Timestamp as string: = " should be logged
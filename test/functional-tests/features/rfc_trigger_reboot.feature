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

Feature: RFC Trigger Reboot - Account ID Transition

  Scenario: RFC processes XConf response with valid Account ID transition
    Given the mockxconf server is running
    And the device starts with an Unknown account ID
    When the RFC manager binary is run and queries XConf
    Then a message "Checking Config Value changed for tr181 param" should be logged
    And a message "RFC: Checking AccountId received from Xconf is Unknown" should be logged
    And a message "RFC: Comparing Xconfvalue='3064488088886635972' with Unknown" should be logged
    And a message "AccountId is Valid 3064488088886635972, Updating the device Database" should be logged

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

Feature: RFC Manager direct-failure blocking

  Scenario: Direct block is active within 24 hours of a failure
    Given the direct-failure marker is less than 24 hours old
    When the RFC manager binary is run
    Then the log should contain "Last direct failed blocking is still valid"
    And no XConf request should be made during that run

  Scenario: Direct block expires after 24 hours
    Given the direct-failure marker is more than 24 hours old
    When the RFC manager binary is run
    Then the direct-failure marker should be removed
    And the log should contain "Last direct failed blocking has expired"
    And an XConf request should be made during that run

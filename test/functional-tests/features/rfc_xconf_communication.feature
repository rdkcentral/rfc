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

Feature: RFC Manager XCONF Communication and Error Handling

  Scenario: Unresolved XCONF URL
    Given the RFC properties file is modified to use an unresolved XCONF URL
    When the RFC manager binary is run
    Then an error message "Couldn't resolve host name" should be logged
    And an error message "cURL Return : 6 HTTP Code : 0" should be logged

  Scenario: 404 XCONF URL
    Given the RFC properties file is modified to use a 404 XCONF URL
    When the RFC manager binary is run
    Then an error message "cURL Return : 0 HTTP Code : 404" should be logged

  Scenario: URL Encoding
    Given all the properties to run RFC manager is available and running
    When RFC manager binary is communicating with XCONF server
    Then the URL should be percentage encoded 
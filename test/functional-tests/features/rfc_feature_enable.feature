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

Feature: RFC feature enable status

  Scenario: A 304 response for an XCONF URL indicates the server checked and determined the resource has not changed since what the client already has.So the server sends no body, only headers
    Given the RFC properties file is modified to use a 304 XCONF URL
    When the RFC manager binary is run
    Then a message "cURL Return : 0 HTTP Code : 304" should be logged
    And a message "HTTP request success. Response unchanged (304). No processing" should be logged
    And a RFC feature message "[Features Enabled]-[ACTIVE]:" should be logged

  Scenario: A 404 response for an XCONF URL indicates that the requested resource could not be found on the server
    Given the RFC properties file is modified to use a 404 XCONF URL
    When the RFC manager binary is run
    Then a message "cURL Return : 0 HTTP Code : 404" should be logged
    And a RFC feature message "[Features Enabled]-[NONE]:" should be logged 

  Scenario: Valid XCONF URL
    Given the RFC properties file is modified to use a Valid XCONF URL
    When the RFC manager binary is run
    Then an error message "cURL Return : 0 HTTP Code : 200" should be logged
    And a RFC feature message "[Features Enabled]-[STAGING]:" should be logged


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

Feature: RFC Manager Configuration Management 

  Scenario: Validate RFC manager rejects empty parameter values from Xconf
    Given the mockxconf server is running
    When a RFC curl request is made to "https://mockxconf:50053/featureControl/getSettings?" with:
      | method | GET |
      | query  | estbMacAddress=01%3A23%3A45%3A67%3A89%3Aab&firmwareVersion=T2_Container_0.0.0&env=prod&model=L2CNTR&manufacturer=&controllerId=2504&channelMapId=2345&VodId=15660&partnerId=global&osClass=&accountId=Unknown&Experience=X1&version=2 |
    Then the mockxconf server should have received a request to ""https://mockxconf:50053/featureControl/getSettings?"
    And the request method should be "GET"
    And the response from the xconf server and the PartnerProductName value should be empty
      | PartnerProductName       | "" |
    And the RFC manager should reject the configuration
    And a message "EMPTY value for Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName is rejected" should be logged 

  Scenario: Successfully retrieve PartnerName value and verify via tr181
    Given the mockxconf server is running
    When a RFC curl request is made to "https://mockxconf:50053/featureControl/getSettings?" with:
      | method | GET |
      | query  | estbMacAddress=01%3A23%3A45%3A67%3A89%3Aab&firmwareVersion=T2_Container_0.0.0&env=prod&model=L2CNTR&manufacturer=&controllerId=2504&channelMapId=2345&VodId=15660&partnerId=global&osClass=&accountId=Unknown&Experience=X1&version=2 |
    Then the mockxconf server should have received a request to ""https://mockxconf:50053/featureControl/getSettings?"
    And the request method should be "GET"
    And the response from the xconf server and the PartnerName value should be TestPartner
      | PartnerName       | TestPartner |
    When I run "tr181 -d -g Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName"
    Then the output should be "TestPartner" 


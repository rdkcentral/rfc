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

Feature: Verify RFC request parameters

  Scenario: RFC GET request is sent with correct parameters
    Given the mockxconf server is running
    When a RFC curl request is made to "https://mockxconf:50053/featureControl/getSettings?" with:
      | method | GET |
      | query  | estbMacAddress=01%3A23%3A45%3A67%3A89%3Aab&firmwareVersion=T2_Container_0.0.0&env=prod&model=L2CNTR&manufacturer=&controllerId=2504&channelMapId=2345&VodId=15660&partnerId=global&osClass=&accountId=Unknown&Experience=X1&version=2 |
    Then the mockxconf server should have received a request to ""https://mockxconf:50053/featureControl/getSettings?"
    And the request method should be "GET"
    And the request query parameters should contain:
	| accountId       | Unknown            |
    And the response from the xconf server and the account id should be Unknown
	| accountId       | Unknown|
    And a message "Feature Name [Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID] Value[Unknown]" should be logged
    And a message "Checking Config Value changed for tr181 param" should be logged
    And a message "RFC: AccountId received from Xconf is Unknown" should be logged
    And a message "RFC: Comparing Xconfvalue='Unknown' with Unknown" should be logged
    And a message "RFC: AccountId Unknown is replaced with Authservice 412370664406228514" should be logged
    And a message "RFC: AccountId Updated Value is 412370664406228514 and Xconf value is 412370664406228514" should be logged 

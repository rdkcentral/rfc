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

Feature: Invalid Account ID Validation
  As a system administrator
  I want to ensure that invalid account IDs are properly rejected
  So that the system maintains data integrity and security

  Background:
    Given the RFC system is initialized
    And the telemetry system is running

  Scenario: Set invalid account ID with special characters
    Given I have an account ID with invalid characters "306045!@#06186635988"
    When I set the account ID using TR181 parameter "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
    Then the set operation should succeed
    And the invalid characters should be logged

  Scenario: XCONF request validates invalid account ID
    Given the TR181 INI file does not exist
    And the RFC old firmware file is backed up
    When the RFC binary is executed
    Then the TR181 INI file should be created
    And the RFC log file should contain "Invalid characters in newly received accountId"

  Scenario Outline: Validate various invalid account ID formats
    Given I have an account ID "<account_id>"
    When I attempt to set it via TR181
    Then the system should log "Invalid characters in newly received accountId"
    And the operation should be handled appropriately

    Examples:
      | account_id            |
      | 306045!@#06186635988 |
      | test@#$%account      |
      | 123<>456             |
      | acc&*()id            |
      | id;DROP TABLE;       |


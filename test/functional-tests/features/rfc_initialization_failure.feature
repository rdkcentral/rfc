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

Feature: RFC Initialization Failure

  Scenario: Initialization failure due to missing server URL in properties file
    Given the properties file exists
    And the properties file is modified to have an empty server URL
    When the RFC binary is run
    Then an error message "URL not found in the file." should be logged
    And an error message "Xconf Initialization Failed...!!" should be logged

  Scenario: Initialization failure due to missing properties file
    Given the properties file does not exist
    When the RFC binary is run
    Then an error message "Failed to open file." should be logged
    And an error message "Xconf Initialization Failed...!!" should be logged


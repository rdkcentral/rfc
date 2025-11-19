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

Feature: Verify RFC data population
  The system should correctly populate RFC data files
  under the /opt/secure/RFC directory. 

  Background:
     Given the RFC API service is running 

  Scenario: Validate RFC data is populated under /opt/secure/RFC
    When the RFC binary is run
    And the directory "/opt/secure/RFC" should not be empty
    Then the directory "/opt/secure/RFC" should contain the file "tr181store.ini"
    And the directory "/opt/secure/RFC" should contain the file "tr181localstore.ini"
    And the directory "/opt/secure/RFC" should contain the file "tr181.list"
    And the directory "/opt/secure/RFC" should contain the file "rfcVariable.ini"
    And the directory "/opt/secure/RFC" should contain the file "rfcFeature.list"
    And each RFC data file should have a size greater than 0 bytes



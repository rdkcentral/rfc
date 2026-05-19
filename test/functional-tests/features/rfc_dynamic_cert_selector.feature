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

Feature: Certificate Selector - Dynamic Certificate Method
  I want to validate that Cert Selector uses the Dynamic Certificate method
  So that secure communication is established correctly

  Background:
    Given the Cert Selector component is initialized

  Scenario: Validate dynamic certificate is successfully loaded and mTLS connection succeeds
    Given the dynamic certificate "/opt/certs/client.p12" exists in the certificate directory
    When the Cert Selector loads the certificate
    Then a message "Initializing cert selector" should be logged
    And a message "Cert selector initialization successful" should be logged
    And a message "MTLS dynamic/static cert success. cert=/opt/certs/client.p12, type=P12" should be logged
    And a message "MTLS is enable" should be logged
    And a message "RFC Xconf Connection Response cURL Return : 0 HTTP Code : 200" should be logged

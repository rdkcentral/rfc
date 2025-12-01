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

Feature: Certificate Selector Fallback to Static Certificate
  I want to use a static certificate when dynamic certificate retrieval fails
  So that secure communication can still be established

  Background:
    Given the Cert Selector component is initialized

  Scenario: Use static certificate when dynamic certificate is unavailable
    Given the Cert Selector is configured with method "Dynamic"
    And the dynamic certificate source is unavailable
    When the Cert Selector attempts to retrieve a certificate
    Then it should fallback to the static certificate "static_cert.pem"
    And the handshake should use the static certificate

  Scenario: Use static certificate when dynamic certificate retrieval fails
    Given the Cert Selector is configured with method "Dynamic"
    And the dynamic certificate retrieval fails with error "Timeout"
    When the Cert Selector attempts to retrieve a certificate
    Then it should fallback to the static certificate "static_cert.pem"
    And the handshake should use the static certificate


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

Feature: Certificate Selector - Static Certificate Method
  I want to validate that Cert Selector uses the Static Certificate method
  So that secure communication is established correctly

  Background:
    Given the Cert Selector component is initialized

  Scenario: Validate static certificate is successfully loaded
  Given the static certificate "static_cert.pem" exists in the certificate directory
  When the Cert Selector loads the static certificate
  Then the load operation should complete successfully
  And no certificate parsing errors should occur

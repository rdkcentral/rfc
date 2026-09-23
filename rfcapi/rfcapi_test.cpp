/**
 * @file rfcapi_test.cpp
 * @brief RFC API CLI test application - allows independent testing of RFC APIs.
 *
 * Usage:
 *   rfc_feature_test get <parameter>
 *   rfc_feature_test set <parameter> <value>
 *   rfc_feature_test getrfc <feature> <key>
 *   rfc_feature_test enabled <feature>
 *
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfcapi.h"

#define RFC_CALLER_ID "RFC_TEST_APP"
#define VALUE_BUF_SIZE 2048

/**
 * @brief Print usage help.
 */
static void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s get <parameter>                - Get RFC parameter value\n", prog_name);
    printf("  %s set <parameter> <value>        - Set RFC parameter value\n", prog_name);
    printf("  %s getrfc <feature> <key>         - Get specific RFC value from feature marker\n", prog_name);
    printf("  %s enabled <feature>              - Check if feature is enabled\n", prog_name);
    printf("\n");
    printf("Examples:\n");
    printf("  %s get Device.DeviceInfo.SoftwareVersion\n", prog_name);
    printf("  %s set Device.DeviceInfo.SoftwareVersion TEST_VALUE\n", prog_name);
    printf("  %s getrfc HDR RFC_ENABLE_HDR\n", prog_name);
    printf("  %s enabled HDR\n", prog_name);
}

/**
 * @brief Test getRFCParameter() API.
 * @param[in] param_name  Parameter name to retrieve.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
static int test_get_parameter(const char *param_name)
{
    if (!param_name || strlen(param_name) == 0) {
        printf("ERROR: Parameter name cannot be empty\n");
        return EXIT_FAILURE;
    }

    RFC_ParamData_t param;
    memset(&param, 0, sizeof(param));

    printf("Getting RFC parameter: %s\n", param_name);
    WDMP_STATUS status = getRFCParameter(RFC_CALLER_ID, param_name, &param);

    if (status != WDMP_SUCCESS) {
        printf("FAILED: getRFCParameter() returned status=%d (%s)\n", (int)status, getRFCErrorString(status));
        return EXIT_FAILURE;
    }

    if (param.value[0] == '\0') {
        printf("FAILED: Parameter value is empty\n");
        return EXIT_FAILURE;
    }

    printf("SUCCESS: %s = %s\n", param_name, param.value);
    return EXIT_SUCCESS;
}

/**
 * @brief Test setRFCParameter() API.
 * @param[in] param_name   Parameter name to set.
 * @param[in] param_value  Value to set.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
static int test_set_parameter(const char *param_name, const char *param_value)
{
    if (!param_name || strlen(param_name) == 0) {
        printf("ERROR: Parameter name cannot be empty\n");
        return EXIT_FAILURE;
    }

    if (!param_value || strlen(param_value) == 0) {
        printf("ERROR: Parameter value cannot be empty\n");
        return EXIT_FAILURE;
    }

    printf("Setting RFC parameter: %s = %s\n", param_name, param_value);
    WDMP_STATUS status = setRFCParameter(RFC_CALLER_ID, param_name, param_value, WDMP_STRING);

    if (status != WDMP_SUCCESS) {
        printf("FAILED: setRFCParameter() returned status=%d (%s)\n", (int)status, getRFCErrorString(status));
        return EXIT_FAILURE;
    }

    printf("SUCCESS: Parameter set successfully\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Test getRFCFeatureValue() API.
 * @param[in] feature_name  Feature name (without .RFC_ prefix).
 * @param[in] key           RFC key to extract (e.g., RFC_ENABLE_HDR).
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
static int test_get_rfc_value(const char *feature_name, const char *key)
{
    if (!feature_name || strlen(feature_name) == 0) {
        printf("ERROR: Feature name cannot be empty\n");
        return EXIT_FAILURE;
    }

    if (!key || strlen(key) == 0) {
        printf("ERROR: RFC key cannot be empty\n");
        return EXIT_FAILURE;
    }

    char value_buf[VALUE_BUF_SIZE];
    memset(value_buf, 0, sizeof(value_buf));

    printf("Getting RFC value from feature: %s, key: %s\n", feature_name, key);
    WDMP_STATUS status = getRFCFeatureValue(feature_name, key, value_buf, sizeof(value_buf));

    if (status != WDMP_SUCCESS) {
        printf("FAILED: getRFCFeatureValue() returned status=%d (%s)\n", (int)status, getRFCErrorString(status));
        return EXIT_FAILURE;
    }

    printf("SUCCESS: %s (from .RFC_%s.ini) = %s\n", key, feature_name, value_buf);
    return EXIT_SUCCESS;
}

/**
 * @brief Test isFeatureEnabled() API.
 * @param[in] feature_name  Feature name (without .RFC_ prefix).
 * @return EXIT_SUCCESS if enabled, EXIT_FAILURE if disabled or error.
 */
static int test_is_feature_enabled(const char *feature_name)
{
    if (!feature_name || strlen(feature_name) == 0) {
        printf("ERROR: Feature name cannot be empty\n");
        return EXIT_FAILURE;
    }

    printf("Checking if feature is enabled: %s\n", feature_name);
    bool enabled = isFeatureEnabled(feature_name);

    if (enabled) {
        printf("SUCCESS: Feature %s is ENABLED\n", feature_name);
        return EXIT_SUCCESS;
    } else {
        printf("RESULT: Feature %s is DISABLED or marker not found\n", feature_name);
        return EXIT_FAILURE;
    }
}

/**
 * @brief Main entry point - route to appropriate API based on command line arguments.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *command = argv[1];

    /* Route to appropriate API based on command */
    if (strcmp(command, "get") == 0) {
        if (argc < 3) {
            printf("ERROR: get requires parameter name\n");
            printf("Usage: %s get <parameter>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return test_get_parameter(argv[2]);
    }
    else if (strcmp(command, "set") == 0) {
        if (argc < 4) {
            printf("ERROR: set requires parameter name and value\n");
            printf("Usage: %s set <parameter> <value>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return test_set_parameter(argv[2], argv[3]);
    }
    else if (strcmp(command, "getrfc") == 0) {
        if (argc < 4) {
            printf("ERROR: getrfc requires feature name and key\n");
            printf("Usage: %s getrfc <feature> <key>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return test_get_rfc_value(argv[2], argv[3]);
    }
    else if (strcmp(command, "enabled") == 0) {
        if (argc < 3) {
            printf("ERROR: enabled requires feature name\n");
            printf("Usage: %s enabled <feature>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return test_is_feature_enabled(argv[2]);
    }
    else {
        printf("ERROR: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


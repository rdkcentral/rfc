/**
 * @file rfcapi_test.cpp
 * @brief Standalone test utility for RFC feature check APIs.
 *
 * Usage: rfc_feature_test <feature_name>
 *
 * Tests the following APIs for a given RFC feature:
 * - getRFCFeature()     : Checks if marker file exists
 * - isRFCEnabled()      : Backward-compatible marker file check
 * - isFeatureEnabled()  : Checks marker + RFC_ENABLE_<Feature>=true
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

#define RFC_FEATURE_DIR "/opt/secure/RFC/"

/**
 * @brief Print usage message and exit
 */
static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s <feature_name>\n\n", prog_name);
    fprintf(stderr, "Test RFC feature check APIs for a given feature.\n\n");
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "  <feature_name>  RFC feature name (without RFC_ prefix)\n\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s FEATURE_EXAMPLE\n\n", prog_name);
    fprintf(stderr, "APIs tested:\n");
    fprintf(stderr, "  - getRFCFeature()     : Checks if .RFC_<feature>.ini exists\n");
    fprintf(stderr, "  - isRFCEnabled()      : Same as getRFCFeature (backward compat)\n");
    fprintf(stderr, "  - isFeatureEnabled()  : Checks marker + RFC_ENABLE_<feature>=true\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Main entry point
 */
int main(int argc, char *argv[])
{
    const char *feature_name = NULL;
    char marker_file_path[512];
    bool get_rfc_result = false;
    bool is_rfc_enabled_result = false;
    bool is_feature_enabled_result = false;

    /* Validate arguments */
    if (argc < 2)
    {
        fprintf(stderr, "Error: Feature name is required.\n");
        print_usage(argv[0]);
    }

    feature_name = argv[1];

    /* Validate feature name */
    if ((feature_name == NULL) || (feature_name[0] == '\0'))
    {
        fprintf(stderr, "Error: Feature name cannot be empty.\n");
        print_usage(argv[0]);
    }

    if (strlen(feature_name) > 256)
    {
        fprintf(stderr, "Error: Feature name too long (max 256 characters).\n");
        exit(EXIT_FAILURE);
    }

    /* Construct marker file path */
    snprintf(marker_file_path, sizeof(marker_file_path), "%s.RFC_%s.ini", 
             RFC_FEATURE_DIR, feature_name);
    marker_file_path[sizeof(marker_file_path) - 1] = '\0';

    /* Print header */
    printf("=======================================================\n");
    printf("RFC Feature Check Test Utility\n");
    printf("=======================================================\n");
    printf("Feature Name     : %s\n", feature_name);
    printf("Marker File Path : %s\n", marker_file_path);
    printf("=======================================================\n\n");

    /* Test getRFCFeature() */
    printf("Testing getRFCFeature():\n");
    printf("  Description: Check if marker file exists\n");
    get_rfc_result = getRFCFeature(feature_name);
    printf("  Result: %s\n", get_rfc_result ? "true (marker file exists)" : "false (marker file not found)");
    printf("\n");

    /* Test isRFCEnabled() */
    printf("Testing isRFCEnabled():\n");
    printf("  Description: Check if marker file exists (backward compatible)\n");
    is_rfc_enabled_result = isRFCEnabled(feature_name);
    printf("  Result: %s\n", is_rfc_enabled_result ? "true (marker file exists)" : "false (marker file not found)");
    printf("\n");

    /* Test isFeatureEnabled() */
    printf("Testing isFeatureEnabled():\n");
    printf("  Description: Check if marker exists AND RFC_ENABLE_<feature>=true\n");
    is_feature_enabled_result = isFeatureEnabled(feature_name);
    printf("  Result: %s\n", is_feature_enabled_result ? "true (marker + enable flag=true)" : "false (marker missing or flag!=true)");
    printf("\n");

    /* Print summary */
    printf("=======================================================\n");
    printf("Summary:\n");
    printf("=======================================================\n");
    printf("Marker File Exists      : %s\n", get_rfc_result ? "YES" : "NO");
    printf("isRFCEnabled()          : %s\n", is_rfc_enabled_result ? "YES" : "NO");
    printf("isFeatureEnabled()      : %s\n", is_feature_enabled_result ? "YES" : "NO");
    printf("=======================================================\n");

    /* Return success if any API succeeded, else failure */
    if (get_rfc_result || is_rfc_enabled_result || is_feature_enabled_result)
    {
        printf("\nFeature appears to be configured on this device.\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf("\nFeature is not configured or not enabled on this device.\n");
        return EXIT_FAILURE;
    }
}


/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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
 **/

#ifndef RDK_DEBUG_H_
#define RDK_DEBUG_H_

#include <stdio.h>


// Try to include the real RDK logger if available
#ifdef RDK_LOGGER
    // First try to include the real RDK logger header
    #if __has_include(<rdk_logger.h>)
        #include <rdk_logger.h>
    #elif __has_include("rdk_logger.h")  
        #include "rdk_logger.h"
    #else
        // If real RDK logger is not available, provide fallback definitions
        #define RDK_LOG_TRACE1 1
        #define RDK_LOG_DEBUG 2
        #define RDK_LOG_INFO 3
        #define RDK_LOG_WARN 4
        #define RDK_LOG_ERROR 5
        #define RDK_LOG_FATAL 6

        // RDK Logger success/failure codes
        #define RDK_SUCCESS 0
        #define RDK_FAILURE -1

        // RDK Logger Extended API structures and enums
        typedef enum {
            RDKLOG_OUTPUT_CONSOLE = 1,
            RDKLOG_OUTPUT_FILE = 2
        } rdklog_output_t;

        typedef enum {
            RDKLOG_FORMAT_SIMPLE = 1,
            RDKLOG_FORMAT_WITH_TS = 2
        } rdklog_format_t;

        typedef struct {
            char *pModuleName;
            int loglevel;
            rdklog_output_t output;
            rdklog_format_t format;
            void *pFilePolicy;
        } rdk_logger_ext_config_t;

        // Fallback implementations for different RDK logger APIs
        #ifdef RDKB_SUPPORT
            // For RDKB builds, use the old API
            #define RDK_LOGGER_INIT() printf("RDK Logger initialized (fallback)\n")
        #else
            // For non-RDKB builds, use the new extended API
            static inline int rdk_logger_ext_init(rdk_logger_ext_config_t *config) {
                if (config && config->pModuleName) {
                    printf("RDK Extended Logger initialized for module: %s (fallback)\n", config->pModuleName);
                }
                return RDK_SUCCESS;
            }
        #endif

        // Fallback RDK_LOG macro that uses printf
        #define RDK_LOG(level, module, ...) \
            do { \
                if (level == RDK_LOG_DEBUG) { \
                    printf("DEBUG: %s: ", module); \
                } \
                else if (level == RDK_LOG_INFO) { \
                    printf("INFO: %s: ", module); \
                } \
                else if (level == RDK_LOG_ERROR) { \
                    printf("ERROR: %s: ", module); \
                } \
                else if (level == RDK_LOG_FATAL) { \
                    printf("FATAL: %s: ", module); \
                } \
                printf(__VA_ARGS__); \
            } while (0)
    #endif
#else
    // When RDK_LOGGER is not defined, provide minimal definitions
    #define RDK_LOG_TRACE1 1
    #define RDK_LOG_DEBUG 2
    #define RDK_LOG_INFO 3
    #define RDK_LOG_WARN 4
    #define RDK_LOG_ERROR 5
    #define RDK_LOG_FATAL 6

    #define RDK_SUCCESS 0
    #define RDK_FAILURE -1

    #define RDK_LOGGER_INIT() ;
    
    typedef enum {
        RDKLOG_OUTPUT_CONSOLE = 1,
        RDKLOG_OUTPUT_FILE = 2
    } rdklog_output_t;

    typedef enum {
        RDKLOG_FORMAT_SIMPLE = 1,
        RDKLOG_FORMAT_WITH_TS = 2
    } rdklog_format_t;
    
    typedef struct {
        char *pModuleName;
        int loglevel;
        rdklog_output_t output;
        rdklog_format_t format;
        void *pFilePolicy;
    } rdk_logger_ext_config_t;

    static inline int rdk_logger_ext_init(rdk_logger_ext_config_t *config) {
        (void)config; // Suppress unused parameter warning
        return RDK_SUCCESS;
    }

    // Simple printf-based logging when RDK_LOGGER is not enabled
    #define RDK_LOG(level, module, ...) \
        do { \
            printf("[%s] ", module); \
            printf(__VA_ARGS__); \
        } while (0)
#endif

#endif // RDK_DEBUG_H_

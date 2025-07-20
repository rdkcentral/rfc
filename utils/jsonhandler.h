/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
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


/**
* @defgroup rfc
* @{
* @defgroup utils
* @{
**/

#ifndef JSONHANDLER_H
#define JSONHANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include <string.h>
#include <errno.h>

#if defined(GTEST_ENABLE)
char * readFromFile(char * absolutePath);
cJSON * getArrayNode(cJSON *node);
int saveToFile(cJSON * arrayNode, const char * format,const char * name);
int saveIfNodeContainsLists(cJSON *node,const char * asbolutepath);
int iterateAndSaveArrayNodes(const char * absolutePath,const char * json_data);
char * getFilePath();
#endif

#endif // JSONHANDLER_H

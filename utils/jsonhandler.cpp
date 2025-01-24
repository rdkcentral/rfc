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

#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include <string.h>
#include <errno.h>

#define JSON_ARRAY_IDENTIFIER "listType"
#define JSON_NAME_IDENTIFIER "name"
#define JSON_FEATURE_CONTROL_IDENTIFIER "featureControl"
#define JSON_FEATURES_IDENTIFIER "features"

#define FILE_PREFIX "RFC_LIST_FILE_NAME_PREFIX"
#define FILE_SUFFIX "RFC_LIST_FILE_NAME_SUFFIX"
#define ROOT_DIR "RFC_PATH"


/**
 * @brief returns the size of the file
 * @param the file descriptor, expected to be non-null
 * @return the size of the file in bytes
 * */
long fsize(FILE *fp)
{
	fseek(fp,0,SEEK_END);
	long size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	return size;
}
/**
 * @brief returns whether the node type is an array or not
 * @param the node to check 
 * @return  non-zero  if an array type, zero otherwise
 * */


int isNodeArrayType(cJSON * const item)
{
    if (item == NULL)
    {
        return 0;
    }

    return (item->type & 0xFF) == cJSON_Array;
}


/**
 * @brief returns the contents of the file in an array.
 * if the input file exists and is not empty and is small enough, will
 * return content of the file in a buffer;
 * @param filename  the absolute path of the file to read
 * @return the char buffer containing the contents, or NULL in case of error
 * 
 * @warning Current implementation does not have any limitation on the size
 * of the file. This can cause potential issues if someone try to open a 
 * huge file.
 * */

char * readFromFile(char * absolutePath)
{
	FILE *fp = fopen( absolutePath, "r");
	char *fcontent = NULL;
	if(NULL != fp)
	{
          //CID 56034 FIX - Argument cannot be negative (NEGATIVE_RETURNS)
		long size = fsize(fp);
                if( size < 0 ){
                        printf("%s:%d Warning. failed to obtain the length of the file, size : %ld \n", 
                                __FUNCTION__, __LINE__, size);
                        fclose(fp);
                        return fcontent;
                }
		fcontent = new char[size];
		if(NULL != fcontent)
		{
			long readSize = fread(fcontent,1,size,fp);
			if(readSize != size)
			{
				printf("%s:%d Warning. Total characters read is not matching filesize,actual : %ld,  expected: %ld \n", 
					__FUNCTION__, __LINE__, readSize, size);
			}
		
		}
		fclose(fp);
	} 
	return fcontent;
}

/**
 * @brief returns the array node in the given json object
 * if the node contains a node that is of array type, will return the 
 * array node; otherwise, will return NULL.
 * @param node the node to be checked
 * @return the first node that is array type, NULL if not found
 * 
 * */
 
cJSON * getArrayNode(cJSON *node)
{
	cJSON * arrayNode = NULL;
	while(NULL != node)
	{
		
		if(isNodeArrayType(node))
		{
			arrayNode = node;
			break;
		}
		else if (NULL != node->child)
		{
			cJSON * childNode = getArrayNode(node->child);
			if( NULL != childNode)
			{
				arrayNode = childNode;
				break;
			}
		} 
		node = node->next;
	}
	return arrayNode;
}

/**
 * @brief Saves the contents of array node to a file.
 * Saves the contents of array to a file, each array item per line
  * @param arrayNode the json array type node
 * @return 1 if success zero otherwise.
 * 
 * */
 
int saveToFile(cJSON * arrayNode, const char * format,const char * name)
{
	int status = 0;
	char fileName [255]={'0'}; 		
	FILE * fp;
	int i = 0;
	status = snprintf(fileName,255,format,name);	
	if(status <= 0)
	{
		printf("%s:%d Failed to create proper filename %s \n", __FUNCTION__, __LINE__, name);
		status = 0 ;				
	}
	else
	{
		fp = fopen(fileName,"w");
		if(NULL != fp)
		{
			int arraySize = cJSON_GetArraySize(arrayNode);
			printf("Saving to %d entries to %s \n",arraySize,fileName);
			for(i =0;i<arraySize;i++)
			{
				cJSON * aItem = cJSON_GetArrayItem(arrayNode,i);
				status = fprintf(fp,"%s\n",aItem->valuestring);
				if(status <= 0)
					printf("%s:%d Warning failed to write to file %s \n",__FUNCTION__, __LINE__,strerror(errno));
			}
			fclose(fp);		
			status = 1;
		}
		else
			printf("%s:%d Failed to open %s : %s\n",__FUNCTION__, __LINE__,fileName,strerror(errno));
	}
	return status;
}

/**
 * @brief Traverse through node and save array types if present.
 *  Given a json node, this function will check whether there is a subnode
 *  with given idenfier. If present, it will extract the node and save to
 *  a file
 * @param node to search for array items
 * @param absolutepath Path identifier for the file to be saved.
 * @return 1 if an array is found and saved, 0 otherwise
 * 
 * */

int saveIfNodeContainsLists(cJSON *node,const char * asbolutepath)
{
	int status = 0;
	cJSON * childNode =  cJSON_GetObjectItem(node, JSON_ARRAY_IDENTIFIER);

	if(NULL != childNode)
	{		
		//We got a match. 
		cJSON * name = cJSON_GetObjectItem(node,JSON_NAME_IDENTIFIER);
		if(NULL != name)
		{
			childNode = getArrayNode(node);
			//Now get to the array type 
			if(NULL != childNode)
			{
				status = saveToFile(childNode,asbolutepath,name->valuestring);		
			}			
		}
	}
	return status;
}
/**
 * @brief Iterate through nodes and look for array nodes to save 
 * 
 * @param absolutePath the path to which array lists needs to be saved
 * @param json_data the json structure to parse
 * @return the total number of arrays saved.
 * */
int iterateAndSaveArrayNodes(const char * absolutePath,const char * json_data){
	int count = 0;
	cJSON* head = NULL;
	cJSON * node = NULL;
	
	cJSON* root = cJSON_Parse(json_data);
	if(NULL != root)
	{
		head = cJSON_GetObjectItem(root, JSON_FEATURE_CONTROL_IDENTIFIER);
		if (NULL != head)
		{
			head = cJSON_GetObjectItem(head, JSON_FEATURES_IDENTIFIER);
		}
		if (NULL != head)
		{
			//Expecting head as an array of features.
			cJSON_ArrayForEach(node,head)
			{
				int result = saveIfNodeContainsLists(node,absolutePath);
				if(result == 1)
				{
					count += 1;
				}
			}
		}
		cJSON_Delete(root);	
	}	
	return count;
}
/**
 * @brief Retuns the path and format of files to be saved .
 *  This function will retrieve the values of environment variables and 
 *  and provide a path for retrieved lists.  
 * @return the path specifier where lists are stored.
**/

char * getFilePath(){
	//CID 69211,69400,74148 FIX - PARSE WARNING
	const char* root_path = getenv(ROOT_DIR);
	const char* filename_prefix = getenv(FILE_PREFIX);
	const char* filename_suffix = getenv(FILE_SUFFIX);

	char * path = NULL;
	int path_size = 0;

	if(NULL == root_path)
	{
		printf("[%s: %d] Warning %s is not set, using /opt/secure/RFC \n",__FUNCTION__,__LINE__,ROOT_DIR);
		root_path ="/opt/secure/RFC";
	}

	if(NULL == filename_prefix)
	{
		printf("[%s: %d] Warning %s is not set, using .RFC_LIST_\n",__FUNCTION__,__LINE__,FILE_PREFIX);
		filename_prefix = ".RFC_LIST_";
	}
	if(NULL == filename_suffix)
	{
		printf("[%s: %d] Warning %s is not set, using .ini\n",__FUNCTION__,__LINE__,FILE_SUFFIX);
		filename_suffix = ".ini";
	}

	printf("[%s: %d] Using  %s %s %s \n",__FUNCTION__,__LINE__, root_path,filename_prefix,filename_suffix);
	path_size = strlen(root_path) + strlen(filename_prefix) + strlen(filename_suffix) + 4 ; //to include path seperator %s and trailing NULL character;
	path = new char[path_size];
	snprintf(path,path_size,"%s/%s%%s%s",root_path,filename_prefix,filename_suffix);
	return path;
}

int main (int argc , char *argv [])
{
	char * json_string = NULL;
	char * root_path = NULL;
	int count = 0;
	if(argc != 2)
	{
		printf("Usage : %s <abosulte path to json file>\n", argv[0]);
		return (1);
		
	}
	json_string = readFromFile(argv[1]);
	if(NULL != json_string)
	{	root_path = getFilePath();
		printf("RFC PATH format  is %s\n",root_path);
		count = iterateAndSaveArrayNodes(root_path,json_string);
		printf("Total saved entries %d \n", count);
		delete(json_string);
		delete(root_path);
	}
	else
	{
		printf("Failed to read from input file %s \n",argv[1]);
	}
	return 0;
}

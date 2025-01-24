#!/bin/sh
#
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
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
##########################################################################
#
##################################################################
## Script to check if LSA is enabled
## LSA is considered enabled if:
##    FEATURE.RBI.ENABLED in rmfconfig.ini is set to true OR
##    RFC_ENABLE_LSA in rfcVariable.ini is set to true OR
##    /opt/persistent/lsaEnable exists
## If none of these conditions are true LSA is considered disabled
## Author: Alex Anastas
##################################################################

. /etc/include.properties

if [ -z $LOG_PATH ]; then
    LOG_PATH="/opt/logs"
fi

if [ -z $RDK_PATH ]; then
    RDK_PATH="/lib/rdk"
fi

doesRFCFeatureExist=-1
#####################################################################

#####################################################################
ifeLogging ()
{
    echo "`/bin/timestamp` [IFE]:: $1" >> $LOG_PATH/rfcscript.log
}
#####################################################################

#####################################################################
isFeatureEnabled()
{
    retF=0
    result_getRFC=77
    if [ -f /lib/rdk/getRFC.sh ]; then
        . $RDK_PATH/getRFC.sh  $1

        variable1=RFC_ENABLE_$1
        if [ "$result_getRFC" = "1" ]; then
            varValue=${!variable1}
            ifeLogging "Requesting $variable1 = $varValue"
            if [  $varValue = "true" ]; then
                retF=1
            elif [  $varValue = "false" ]; then
                retF=0
            fi
            doesRFCFeatureExist=1
        else
            ifeLogging "Requesting $1 ($variable1) NOT IN RFC!"
        fi
    else
        ifeLogging "ERROR: missing $RDK_PATH/getRFC.sh"
    fi
    return $retF
}
	
if [ $# -eq 1 ]; then
    isFeatureEnabled $1
    returniF=$?
else
    returniF=-1
    ifeLogging "WRONG Parameters: $#"
    ifeLogging " Usage: isFeatureEnabled.sh FEATURE_NAME"
fi

if [ $doesRFCFeatureExist -eq 1 ]; then
   doesRFCFeatureExist=$returniF
fi
ifeLogging "isFeatureEnabled $1 Returns $returniF"
echo $returniF
return $returniF

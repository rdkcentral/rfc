#!/bin/sh
#
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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
## Script to perform Remote Feature Control
## Updates the following information in the settop box
##    list of features that are neabled or disabled
##    if feature configuration is effective immediately
##    updates startup parameters for each feature
##    updates the list of variables in a single file
## Author: Milorad
##################################################################

. /etc/include.properties

if [ "$BUILD_TYPE" != "prod" ] && [ -f /opt/rfc.properties ]; then
      . /opt/rfc.properties
else
      . /etc/rfc.properties
fi

if [ -z $LOG_PATH ]; then
    LOG_PATH="/opt/logs/"
fi

#check lock first, retry for up to 10 seconds
RFCG_RETRY_COUNT=3
RFCG_RETRY_DELAY=1
loopgR=1
countgR=0

bRetgR=0

while [ $loopgR -eq 1 ]
do
	if [ -f $RFC_WRITE_LOCK ]; then
		countgR=$((countgR + 1))

		if [ $countgR -ge $RFCG_RETRY_COUNT ]
		then
			echo " `/bin/timestamp` getRFC $1 $RFCG_RETRY_COUNT tries failed. Lock file $RFC_WRITE_LOCK is locked" >> $LOG_PATH/rfcscript.log
			echo "Exiting script." >> $LOG_PATH/rfcscript.log
			return 0
		fi
		echo "`/bin/timestamp` READ count = $countgR. Sleeping $RFCG_RETRY_DELAY seconds ..." >> $LOG_PATH/rfcscript.log

		sleep $RFCG_RETRY_DELAY
	else
		# got lock
		loopgR=0
	fi

done

if [ $# -eq 0 ]; then

	if [ -f "$RFC_PATH/rfcVariable.ini" ]; then
		echo "`/bin/timestamp` [RFC] Sourced $RFC_PATH/rfcVariable.ini" >> $LOG_PATH/rfcscript.log

		source "$RFC_PATH/rfcVariable.ini"
		bRetgR=1
	else
		echo "`/bin/timestamp` [RFC] File $RFC_PATH/rfcVariable.ini does not exist" >> $LOG_PATH/rfcscript.log
	fi
else
	RFC_FEATURE="$RFC_PATH/.RFC_$1.ini"
	echo "`/bin/timestamp` [RFC] Requesting $RFC_FEATURE" >> $LOG_PATH/rfcscript.log
	if [ -f $RFC_FEATURE ]; then
		# Only source file if it exists
		source $RFC_FEATURE
		echo "`/bin/timestamp` [RFC] Sourced $RFC_FEATURE" >> $LOG_PATH/rfcscript.log
		bRetgR=1
	fi
fi

result_getRFC=$bRetgR

return $bRetgR




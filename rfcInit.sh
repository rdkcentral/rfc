#!/bin/sh
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
##    list of features that are enabled or disabled
##    if feature configuration is effective immediately
##    updates startup parameters for each feature
##    updates the list of variables in a single file
## Author: Milorad
##################################################################

. /etc/include.properties
. /etc/device.properties
. /etc/rfc.properties


RFC_LOG_FILE="/opt/logs/rfcscript.log"


#####################################################################
##  RFC Logging helper function
#####################################################################
rfcLogging ()
{
    echo "`/bin/timestamp` [RFC]:: $1" >> $RFC_LOG_FILE
}
#####################################################################



## Get update times

oldTime=0
newTime=0
RFC_INIT_LOCK="/opt/.rfcInitInProgress"

rfcMoveToNewLoc()
{
	## check if RFC transition has to take place from old to new secure location
	if [ ! -d $RFC_BASE ]; then
		rfcLogging "ERROR configuring RFC location NewLoc=$RFC_BASE, no parent folder" >> $RFC_LOG_FILE
	else
	    # mark transition in progress
		touch $RFC_INIT_LOCK
		if [ -d $RFC_BASE/RFC ]; then
			rm -r $RFC_BASE/RFC
		fi
		rfcLogging "Changing RFC location CurrentLoc=$OLD_RFC_BASE/RFC, NewLoc=$RFC_BASE/RFC"
		ls -l $OLD_RFC_BASE/RFC >> $RFC_LOG_FILE

		cp -r $OLD_RFC_BASE/RFC $RFC_BASE
		
		# update list files
		cp $RFC_PATH/$RFC_LIST_FILE_NAME_PREFIX*$RFC_LIST_FILE_NAME_SUFFIX $RFC_RAM_PATH/.

		# remove transition lock file
		rm -f $RFC_INIT_LOCK
	fi
	

}
########################################

if [ "$DEVICE_TYPE" != "broadband" ]; then
	if [ -f $OLD_RFC_BASE/RFC/tr181store.ini ]; then
		oldTime=`grep ConfigSetTime $OLD_RFC_BASE/RFC/tr181store.ini|sed 's/=/ = /'|awk '{print $3}'`
	fi
	 
	if [ -f $RFC_BASE/RFC/tr181store.ini ]; then
		newTime=`grep ConfigSetTime $RFC_BASE/RFC/tr181store.ini|sed 's/=/ = /'|awk '{print $3}'`
	fi
	
	## check if RFC transition has to take place from old to new secure location	
        if [ -z "$oldTime" ] || [ -z "$newTime" ]; then
                echo "`/bin/timestamp` RFC: Old time or New time is empty. The value of oldTime = $oldTime and newTime = $newTime." >> $RFC_LOG_FILE
        else
	        if [ $oldTime -gt $newTime ] || [ -f $RFC_INIT_LOCK ]; then
		    echo "`/bin/timestamp` RFC: Transition - Old time $oldTime is greater then new time $newTime." >> $RFC_LOG_FILE
		    rfcMoveToNewLoc
         	else
		    echo "`/bin/timestamp` RFC: No Transition - Old time $oldTime is SMALLER then new time $newTime." >> $RFC_LOG_FILE
	        fi
        fi

	if [ ! -f /tmp/rfcdefaults.ini ]; then
		for f in /etc/rfcdefaults/*.ini; do (cat "${f}"; echo) >> /tmp/rfcdefaults.ini; done
	else
		echo "`/bin/timestamp` rfcInit: rfcdefaults.ini exist" >> $RFC_LOG_FILE
	fi
fi


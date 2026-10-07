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
##    list of features that are enabled or disabled
##    if feature configuration is effective immediately
##    updates startup parameters for each feature
##    updates the list of variables in a single file
## Author: Milorad
##################################################################

. /etc/include.properties
. /etc/rfc.properties

if [ -z $LOG_PATH ]; then
    LOG_PATH="/opt/logs/"
fi

#check lock first, retry for up to 10 seconds
RFCG_RETRY_COUNT=3
RFCG_RETRY_DELAY=1
loopgR=1
countgR=0

bRetgR=0

read_rfc_variables()
{
	rfc_file=$1
	rfc_parsed=0

	while IFS= read -r rfc_line || [ -n "$rfc_line" ]; do
		case "$rfc_line" in
			export\ *) rfc_assignment=${rfc_line#export } ;;
			*) continue ;;
		esac

		case "$rfc_assignment" in
			*=*) ;;
			*) continue ;;
		esac

		rfc_key=${rfc_assignment%%=*}
		rfc_value=${rfc_assignment#*=}
		case "$rfc_value" in
			true|false) ;;
			*) continue ;;
		esac

		case "$rfc_key" in
			RFC_ENABLE_*)
				feature_name=${rfc_key#RFC_ENABLE_}
				;;
			RFC_*_effectiveImmediate)
				feature_name=${rfc_key#RFC_}
				feature_name=${feature_name%_effectiveImmediate}
				;;
			*) continue ;;
		esac

		case "$feature_name" in
			''|*[!A-Za-z0-9_]*) continue ;;
		esac
		if [ "${#feature_name}" -ge 64 ]; then
			continue
		fi
		if export "$rfc_key=$rfc_value"; then
			rfc_parsed=1
		fi
	done < "$rfc_file"

	[ "$rfc_parsed" -eq 1 ]
}

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
		echo "`/bin/timestamp` [RFC] Parsing $RFC_PATH/rfcVariable.ini" >> $LOG_PATH/rfcscript.log

		if read_rfc_variables "$RFC_PATH/rfcVariable.ini"; then
			bRetgR=1
		fi
	else
		echo "`/bin/timestamp` [RFC] File $RFC_PATH/rfcVariable.ini does not exist" >> $LOG_PATH/rfcscript.log
	fi
else
	case "$1" in
		''|*[!A-Za-z0-9_]*)
			echo "`/bin/timestamp` [RFC] Invalid feature name" >> $LOG_PATH/rfcscript.log
			return 0
		;;
	esac
	if [ "${#1}" -ge 64 ]; then
		echo "`/bin/timestamp` [RFC] Invalid feature name" >> $LOG_PATH/rfcscript.log
		return 0
	fi
	RFC_FEATURE="$RFC_PATH/.RFC_$1.ini"
	echo "`/bin/timestamp` [RFC] Requesting $RFC_FEATURE" >> $LOG_PATH/rfcscript.log
	if [ -f "$RFC_FEATURE" ]; then
		if read_rfc_variables "$RFC_FEATURE"; then
			echo "`/bin/timestamp` [RFC] Read $RFC_FEATURE" >> $LOG_PATH/rfcscript.log
			bRetgR=1
		fi
	fi
fi

result_getRFC=$bRetgR

return $bRetgR




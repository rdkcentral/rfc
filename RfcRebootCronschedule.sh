#! /bin/sh
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


##################################################################
## Script to perform Schedule cron for reboot
##################################################################

. /etc/device.properties

CRONTAB_DIR="/var/spool/cron/"
CRONTAB_FILE=$CRONTAB_DIR"root"
CRONFILE_BK="/tmp/cron_tab$$.txt"
DCM_CONF="/tmp/DCMSettings.conf"

if [ -z $LOG_PATH ]; then
    LOG_PATH="/opt/logs"
fi
RFC_LOG_FILE="$LOG_PATH/rfcscript.log"

if [ -z $PERSISTENT_PATH ]; then
    PERSISTENT_PATH="/opt"
fi

cron=''

calcRebootExecTime()
{
    cron=''
    if [ -f $DCM_CONF ]; then
        cron=`cat $DCM_CONF | grep 'urn:settings:CheckSchedule:cron' | cut -d '=' -f2`
    else
        if [ -f $PERSISTENT_PATH/tmpDCMSettings.conf ]; then
            cron=`grep 'urn:settings:CheckSchedule:cron' $PERSISTENT_PATH/tmpDCMSettings.conf | cut -d '=' -f2`
        fi
    fi

    # Scheduling the job 15 mins ahead of scheduling deviceInitiatedFWDnld
    if [ -n "$cron" ]; then
        vc1=`echo "$cron" | awk '{print $1}'`
        vc2=`echo "$cron" | awk '{print $2}'`
        vc3=`echo "$cron" | awk '{print $3}'`
        vc4=`echo "$cron" | awk '{print $4}'`
        vc5=`echo "$cron" | awk '{print $5}'`
        if [ $vc1 -gt 44 ]; then
            # vc1 = vc1 + 15 - 60
            vc1=`expr $vc1 - 45`
            vc2=`expr $vc2 + 1`
            if  [ $vc2 -eq 24 ]; then
                vc2=0
            fi
        else
            vc1=`expr $vc1 + 15`
        fi
        cron=''
        cron=`echo "$vc1 $vc2 $vc3 $vc4 $vc5"`
    fi
}

ScheduleCron()
{
    # Dump existing cron jobs to a file & add new job
    crontab -l -c $CRONTAB_DIR > $CRONFILE_BK

    # Cron job is scheduled only if it is not already scheduled
    IsCheckCronAlreadyScheduled=`cat "$CRONFILE_BK" | grep -ci "/lib/rdk/RFC_Reboot.sh"`
    if [ $IsCheckCronAlreadyScheduled -eq 0 ]; then
        echo -e "$cron /bin/sh /lib/rdk/RFC_Reboot.sh" >> $CRONFILE_BK
        crontab $CRONFILE_BK -c $CRONTAB_DIR
    else
        echo "`/bin/timestamp` [RfcRebootCronschedule.sh] RFC Reboot cron job already scheduled" >> $RFC_LOG_FILE
    fi
    rm -rf $CRONFILE_BK
}

#calculate and schedule cron job
calcRebootExecTime
if [ -f $CRONTAB_FILE ] && [ -n "$cron" ]; then
    ScheduleCron
    echo "`/bin/timestamp` [RfcRebootCronschedule.sh] RFC Reboot cron job scheduled" >> $RFC_LOG_FILE
fi

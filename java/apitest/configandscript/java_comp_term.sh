#!/bin/bash
#
#      -*- OpenSAF  -*-
#
# (C) Copyright 2008 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
# under the GNU Lesser General Public License Version 2.1, February 1999.
# The complete license can be accessed from the following location:
# http://opensource.org/licenses/lgpl-license.php
# See the Copying file included with the OpenSAF distribution for full
# licensing terms.
#
# Author(s): Emerson Network Power
#

COMP_PID_MAP_FILE="/root/amftest/java_test_comp_pid_map"
PATTERN=$SA_AMF_COMPONENT_NAME

echo "Executing Java Component Terminate/Cleanup Script (v1.0 Andras Kovi)"
echo "CompName: $SA_AMF_COMPONENT_NAME"

for line in $(cat $COMP_PID_MAP_FILE)
do
   comp_name=`echo "$line" | cut -d: -f1`
   pid=`echo "$line" | cut -d: -f2`
   if [ $comp_name  ==  $PATTERN ]
   then
      sed -e "/$PATTERN/ d" -i $COMP_PID_MAP_FILE
      echo "Killing Comp:\"$comp_name\" with PID: $pid"
      kill -9 $pid
   fi
done

exit 0


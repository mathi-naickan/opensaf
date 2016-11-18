#!/bin/sh
#
#      -*- OpenSAF  -*-
#
# (C) Copyright 2011 The OpenSAF Foundation
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
# Author(s): Ericsson AB
#

RANDOM=$(date +%s)
testMark=$RANDOM

verbose=0

if expr "$1" '=' "-v" >/dev/null; then
    verbose=1
fi 

ver()
{
    if expr "$verbose" = 1 > /dev/null; then
        echo $1
    fi
}

ver  $testMark

test_input="1 2 3 4 5 6 7 4 8 9 4 10 3 11 2 12"
for b in $test_input; do 
    rv=`ntfsend -E $b -n "$testMark"` 
    done
nid=`echo $rv|grep -i NotificationId|awk '{ print $5}'`
ver  "nid: $nid"

read_marked()
{
    expected_sequence=$1
    sc=$2 #search criteria
    sd=$3 #search direction
    last=`ntfread -v $3 -b $sc -E 4 -n "$testMark"|grep -i eventtime|awk '{ print $3}'`
    res=`echo $last|sed -n 's/\(.*\)$/\1 /p'`
    ver  "result           :  \"$res\""
    ver  "expected_sequence:  \"$expected_sequence\""
    ver  "searchCriteria  : \"$sc\""
    ver  "search direction: \"$sd\""
     if expr "$expected_sequence" '!=' "$res" >/dev/null; then
 	echo "FAILED"
 	exit $sc
     fi
     ver  "PASSED"
    return 0
}

ver  "SA_NTF_SEARCH_BEFORE_OR_AT_TIME = 1"
read_marked "4 " "1" ""

ver  "SA_NTF_SEARCH_AT_TIME = 2"
read_marked "4 4 4 " "2" ""

ver  "SA_NTF_SEARCH_AT_OR_AFTER_TIME = 3"
read_marked "4 5 6 7 4 8 9 4 10 3 11 2 12 " "3" ""

ver  "SA_NTF_SEARCH_BEFORE_TIME = 4"
read_marked "3 " "4" ""

ver  "SA_NTF_SEARCH_AFTER_TIME = 5"
read_marked "10 3 11 2 12 " "5" ""

ver  "SA_NTF_SEARCH_NOTIFICATION_ID = 6"
read_marked "12 " "6" "-i $nid"

ver  "SA_NTF_SEARCH_ONLY_FILTER = 7"
read_marked "1 2 3 4 5 6 7 4 8 9 4 10 3 11 2 12 " "7" ""

ver  "--------------------------------------"
ver  "      Read OLDER"
ver  "--------------------------------------"
ver  "SA_NTF_SEARCH_BEFORE_OR_AT_TIME = 1"
read_marked "4 9 8 4 7 6 5 4 3 2 1 " "1" "-o"

ver "SA_NTF_SEARCH_AT_TIME = 2"
read_marked "4 4 4 " "2" "-o"

ver "SA_NTF_SEARCH_AT_OR_AFTER_TIME = 3"
read_marked "4 " "3" "-o"

ver "SA_NTF_SEARCH_BEFORE_TIME = 4"
read_marked "3 2 1 " "4" "-o"

ver "SA_NTF_SEARCH_AFTER_TIME = 5"
read_marked "10 " "5" "-o"

ver "SA_NTF_SEARCH_NOTIFICATION_ID = 6"
read_marked "12 " "6" "-i $nid"

ver "SA_NTF_SEARCH_ONLY_FILTER = 7"
read_marked "1 2 3 4 5 6 7 4 8 9 4 10 3 11 2 12 " "7" "-o"

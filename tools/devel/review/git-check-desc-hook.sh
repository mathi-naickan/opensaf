#!/bin/sh
#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
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
# Author(s): Wind River Systems
#

# This script can be used as a Git hook the enforce commit message policy
# Usage: ./git-check-desc-hook.sh <revision>
#
# The expected format of the full Git {desc} is:
#
# module: short description of the change, maximum 80 chars line [#500]
# <blank line>
# Begining of the the long description, like with the short description,
# the long description should not have lines longer than 80 chars and it
# should at least be 2 lines long.

GIT_REV=$1
MAX_DESC_LINE_LEN=80

# The tag and merge commits should not have any restrictions other than
# the module naming convention to detect those exceptions
check_commit_exception() {
	echo "$1" | grep -q -e "^Tag: .*$" -e "^Merge: .*$"
	rc=$?

	if [ $rc -eq 0 ]; then
		# We've got a 'Tag: ...' or 'Merge: ..." commit
		# this is a policy exception
		exit 0
	fi
}

check_short_desc() {
	# The patch header first line contains the short description
	short_desc=`echo "$1" | sed -n '1p'`
	short_desc_len=`expr length "$short_desc"`

	match=`expr match "$short_desc" "^[a-z]*: .*$"`
	if [ $match -eq 0 ]; then
		echo "revision $2: The short description does not begin with the 'module: ' syntax" >&2
		exit 1
	fi

	check_commit_exception "$short_desc"

	if [ $short_desc_len -gt $MAX_DESC_LINE_LEN ]; then
		echo "revision $2: The short description is longer than 80 chars" >&2
		exit 1
	fi
}

check_ticket_id() {
	# The patch header Trac ticket(s) list is on line 3, line 2 is always left blank
	short_desc=`echo "$1" | sed -n '1p'`

	if [ -z "$short_desc" ]; then
		echo "revision $2: The revision is not listing any ticket(s)" >&2
		exit 1
	fi

	echo "$short_desc" | egrep -q ".*\[#[0-9]*\]$"
	rc=$?

	if [ $rc -ne 0 ]; then
		echo "revision $2: The ticket(s) listing is not formatted properly" >&2
		exit 1
	fi
}

check_long_desc() {
	# The patch header long description starts at line 3, line 2 is always left blank
	long_desc_line1=`echo "$1" | sed -n '3p'`
	long_desc_line2=`echo "$1" | sed -n '4p'`

	if [ -z "$long_desc_line1" ] || [ -z "$long_desc_line2" ]; then
		echo "revision $2: The long description should be at least 2 lines long" >&2
		exit 1
	fi

	# Make sure we are skipping Signed-off-by and Reviewed-by lines
	echo "$1" | while read line; do
		match=`expr match "$line" ".*-by: "`
		if [ $match -eq 0 ]; then
			desc_line_len=`expr length "$line"`
			if [ $desc_line_len -gt $MAX_DESC_LINE_LEN ]; then
				echo "revision $2: One of the long description line is longer than 80 chars" >&2
				exit 1
			fi
		fi
	done
}

# Get a list of revisions in this changegroup
for rev in $(git log --pretty='format:%H' "${GIT_REV}^..${GIT_REV}"); do
	desc=$(git log --pretty='format:%B%n' "${rev}^..${rev}")
	check_short_desc "$desc" "$rev"
	check_ticket_id "$desc" "$rev"
	check_long_desc "$desc" "$rev"
done

exit 0

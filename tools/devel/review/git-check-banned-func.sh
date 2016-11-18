#!/bin/bash
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

# This script can be used as a Git hook to enforce functions banning
# policy (e.g. ban usage of strcpy, printf etc.)
# Usage: ./hg-check-banned-func.sh <changeset>
# Dependency: banned.txt and patch-tokenize.py
# TODO: Improve the tokeninzer to exclude code comment validation

GIT_REV=$1
BANNED_FUNCTIONS_DICT="banned.txt"
TOKENIZER="python patch-tokenize.py"

trap catch_errors ERR;

catch_errors() {
	exit 1
}

# Get a list of changesets in this changegroup
for rev in $(git log --pretty='format:%H' "${GIT_REV}^..${GIT_REV}"); do
	patch=$(git diff "${rev}^..${rev}")
	echo "$patch" | while read line; do
		# Sort the line tokens, this is needed for look binary search
		tokens=$($TOKENIZER "$line" | sort)
		for token in ${tokens[@]}; do
			# Dict words are anchored with |...|, to prevent partial prefix matching
			look "|$token|" $BANNED_FUNCTIONS_DICT >/dev/null 2>&1
			rc=$?
			if [ $rc -eq 0 ]; then
				echo "changeset $rev: Reference to banned function $token()" >&2
				echo "$line" >&2
				exit 1
			fi
		done
	done
done

exit 0

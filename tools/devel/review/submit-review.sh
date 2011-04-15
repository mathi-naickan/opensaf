#!/bin/bash
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
# Author(s): Wind River Systems
#

HG=`which hg`

if [ -z $EDITOR ]; then
	EDITOR="vi"
fi

dryrun=0; mq=0; cs=0;

usage="Usage: submit-review.sh [-t] [-q|-r rev] [-d dest]"

while getopts ":tqcor:d:" opt; do
	case $opt in
		t ) dryrun=1 ;;
		q ) mq=1 ;;
		r ) rev=$OPTARG; cs=1 ;;
		d ) dest=$OPTARG ;;
		\?) echo $usage
		exit 1 ;;
	esac
done

shift $(($OPTIND -1))

if [ $mq -eq 0 ] && [ $cs -eq 0 ]; then
	cs=1
fi

patchbomb_check=`hg email --help > /dev/null 2>&1`
if [ $? -eq 255 ]; then
	echo "The patchbomb extension isn't enabled in your .hgrc profile"
	echo "Add 'hgext.patchbomb =' to the '[extensions]' section"
	exit 1
fi

if [ $mq -eq 1 ]; then
	mq_check=`hg qseries --help > /dev/null 2>&1`
	if [ $? -eq 255 ]; then
		echo "The MQ extension isn't enabled in your .hgrc profile"
		echo "Add 'hgext.mq =' to the '[extensions]' section"
		exit 1
	fi
fi

if [ -z "$dest" ]; then
	rr=`mktemp -d`
	if [ $? != 0 ]; then
		echo "$0: mktemp failed"
		exit 1
	fi
else
	rr=$dest
	if [ ! -d "$rr" ]; then
		mkdir -p $rr
		if [ $? != 0 ]; then
			echo "$0: mkdir $rr failed"
			exit 1
		fi
	fi
fi

if [ $dryrun -eq 1 ]; then
	echo "***** Running in dry-run mode, nothing will be emailed *****"
fi

if [ $cs -eq 1 ]; then
	if [ -z "$rev" ]; then
		rev="tip"
	fi
	echo "Exporting changeset(s) '$rev' for review"
elif [ $mq -eq 1 ]; then
	qseries=`hg qseries`
	qapplied=`hg qapplied`
	if [ -z "$qseries" ]; then
		echo "ERROR: Patch queue empty, nothing to review."
		rm -rf $rr
		exit 1
	fi

	if [ "$qseries" != "$qapplied" ]; then
		echo "ERROR: The patch queue needs to be fully applied before sending for review."
		rm -rf $rr
		exit 1
	fi
	rev="qbase:qtip"
fi

echo "The review package is exported into $rr"

hgroot=`$HG root`

if [ $mq -eq 1 ]; then
	if [ ! -d "$hgroot/.hg/patches" ]; then
		echo "ERROR: Did you qinit the patch queue properly?"
		rm -rf $rr
		exit 1
	else
		cp $hgroot/.hg/patches/*.patch $rr
		cp $hgroot/.hg/patches/series $rr
	fi
elif [ $cs -eq 1 ]; then
	$HG export -g -o $rr/%R.patch $rev
	cd $rr
	ls -1 *.patch >> series
	cd - > /dev/null 2>&1
fi

cat <<ETX >> $rr/rr
Summary: <<FILL ME>>
Review request for Trac Ticket(s): <<IF ANY LIST THE #>>
Peer Reviewer(s): <<LIST THE TECH REVIEWER(S) / MAINTAINER(S) HERE>>
Pull request to: <<LIST THE PERSON WITH PUSH ACCESS HERE>>
Affected branch(es): <<LIST ALL AFFECTED BRANCH(ES)>>
Development branch: <<IF ANY GIVE THE REPO URL>>

--------------------------------
Impacted area       Impact y/n
--------------------------------
 Docs                    n
 Build system            n
 RPM/packaging           n
 Configuration files     n
 Startup scripts         n
 SAF services            n
 OpenSAF services        n
 Core libraries          n
 Samples                 n
 Tests                   n
 Other                   n


Comments (indicate scope for each "y" above):
---------------------------------------------
 <<EXPLAIN/COMMENT THE PATCH SERIES HERE>>

ETX
$HG log --template 'changeset {node}\nAuthor:\t{author}\nDate:\t{date|rfc822date}\n\n\t{desc|strip|fill76|tabindent}\n\n' -r $rev >> $rr/rr
cat <<ETX >> $rr/rr
ETX
new=`egrep -A 2 -s '^new file mode ' $rr/*.patch | grep -s '\+++ b/' | awk -F "b/" '{ print $2 }' | sort -u`
if [ -n "$new" ]; then
	echo "" >> $rr/rr
	echo "Added Files:" >> $rr/rr
	echo "------------" >> $rr/rr
	for l in $new; do
		echo " $l"
	done >> $rr/rr
	echo "" >> $rr/rr
fi
cat <<ETX >> $rr/rr
ETX
del=`egrep -A 1 -s '^deleted file mode ' $rr/*.patch | grep -s '\--- a/' | awk -F "a/" '{ print $2 }' | sort -u`
if [ -n "$del" ]; then
	echo "" >> $rr/rr
	echo "Removed Files:" >> $rr/rr
	echo "--------------" >> $rr/rr
	for l in $del; do
		echo " $l"
	done >> $rr/rr
	echo "" >> $rr/rr
fi
cat <<ETX >> $rr/rr

Complete diffstat:
------------------
ETX
if [ $mq -eq 1 ]; then
	$HG diff --stat -r $(hg parents -r qbase --template '{rev}') -r qtip >> $rr/rr
elif [ $cs -eq 1 ]; then
	if echo $rev | grep -q ":"; then
		cs1=`echo $rev | awk -F ":" '{ print $1 }'`
		cs1=$(($cs1-1))
		cs2=`echo $rev | awk -F ":" '{ print $2 }'`
		$HG diff --stat -g -r ${cs1}:${cs2} >> $rr/rr
	else
		$HG diff --stat -g -c $rev >> $rr/rr
	fi
fi
cat <<ETX >> $rr/rr


Testing Commands:
-----------------
 <<LIST THE COMMAND LINE TOOLS/STEPS TO TEST YOUR CHANGES>>


Testing, Expected Results:
--------------------------
 <<PASTE COMMAND OUTPUTS / TEST RESULTS>>


Conditions of Submission:
-------------------------
 <<HOW MANY DAYS BEFORE PUSHING, CONSENSUS ETC>>


Arch      Built     Started    Linux distro
-------------------------------------------
mips        n          n
mips64      n          n
x86         n          n
x86_64      n          n
powerpc     n          n
powerpc64   n          n


Reviewer Checklist:
-------------------
[Submitters: make sure that your review doesn't trigger any checkmarks!]


Your checkin has not passed review because (see checked entries):

___ Your RR template is generally incomplete; it has too many blank entries
    that need proper data filled in.

___ You have failed to nominate the proper persons for review and push.

___ Your patches do not have proper short+long header

___ You have grammar/spelling in your header that is unacceptable.

___ You have exceeded a sensible line length in your headers/comments/text.

___ You have failed to put in a proper Trac Ticket # into your commits.

___ You have incorrectly put/left internal data in your comments/files
    (i.e. internal bug tracking tool IDs, product names etc)

___ You have not given any evidence of testing beyond basic build tests.
    Demonstrate some level of runtime or other sanity testing.

___ You have ^M present in some of your files. These have to be removed.

___ You have needlessly changed whitespace or added whitespace crimes
    like trailing spaces, or spaces before tabs.

___ You have mixed real technical changes with whitespace and other
    cosmetic code cleanup changes. These have to be separate commits.

___ You need to refactor your submission into logical chunks; there is
    too much content into a single commit.

___ You have extraneous garbage in your review (merge commits etc)

___ You have giant attachments which should never have been sent;
    Instead you should place your content in a public tree to be pulled.

___ You have too many commits attached to an e-mail; resend as threaded
    commits, or place in a public tree for a pull.

___ You have resent this content multiple times without a clear indication
    of what has changed between each re-send.

___ You have failed to adequately and individually address all of the
    comments and change requests that were proposed in the initial review.

___ You have a misconfigured ~/.hgrc file (i.e. username, email etc)

___ Your computer have a badly configured date and time; confusing the
    the threaded patch review.

___ Your changes affect IPC mechanism, and you don't present any results
    for in-service upgradability test.

___ Your changes affect user manual and documentation, your patch series
    do not contain the patch that updates the Doxygen manual.

ETX

$EDITOR $rr/rr

while [ -z "$subject" ]; do
	read -p "Subject: Review Request for " -e subject
done

while [ -z "$toline" ]; do
	read -p "To: " -e toline
done

COMMAND="$HG email --plain -gd -s 'Review Request for $subject' --intro --desc \
	$rr/rr --to '$toline' --cc devel@list.opensaf.org"

if [ $dryrun -eq 1 ]; then
	COMMAND="${COMMAND} -n"
fi

COMMAND="${COMMAND} $rev"

for l in `cat $rr/series`; do
	COMMAND="y|${COMMAND}"
done
COMMAND="y|${COMMAND}"

if [ $dryrun -eq 1 ]; then
	echo "Email thread dumped into $rr/review.mbox"
	eval $COMMAND 1>$rr/review.mbox 2>/dev/null
else
	eval $COMMAND 2>/dev/null
fi


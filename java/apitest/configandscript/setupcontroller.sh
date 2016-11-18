#!/bin/bash
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
# Author(s): Andras Kovi (OptXware)
#

source /etc/opensaf/script.conf

CONFIGDIR=deploy
if [ -n "$1" ]; then 
	CONFIGDIR=$1
fi

echo "INFO Using config dir: $CONFIGDIR"

INSTALLDIR=$(cat $CONFIGDIR/.installConf)
if [ $? -ne 0 ]; then
	echo "ERROR: Config is invalid."
	exit -1
fi


echo "Setting amf-test-config.xml"
rm -f $SYSCONFDIR/amf-test-config.xml
ln -s $INSTALLDIR/config/amf-test-config.xml $SYSCONFDIR/amf-test-config.xml

echo "Resetting PSSV"
sed "s/PSS/XML/" -i $LOCALSTATEDIR/pssv_spcn_list

echo DONE
echo

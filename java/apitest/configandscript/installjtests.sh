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


echo "1. Creating install directory: $INSTALLDIR"
mkdir -p $INSTALLDIR
if [ "$?" -ne 0 ]; then
	echo "ERROR: Directory $INSTALLDIR could not be created."
	exit -1
fi

echo "2. Creating deploy archive."
tar --exclude .installConf -czf deploy.tar.gz -C $CONFIGDIR/ .

echo "3. Copying config files to install directory."
tar -xzf deploy.tar.gz -C $INSTALLDIR/

echo "DONE"
echo
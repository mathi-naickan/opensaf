#!/bin/sh
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

XTERM=""
OUT="/etc/opt/opensaf/pxy_pxd_demo"
 
echo "Executing Proxy Component Instantiate Script"

# The following environment variables are useful when running in simulation mode,
# where multiple nodes are simulated on the same Linux desktop
if [ ":$NCS_STDOUTS_PATH" = ":" ]
then
    export NCS_STDOUTS_PATH=/var/opt/opensaf/stdouts
fi
echo "NCS_STDOUTS_PATH=$NCS_STDOUTS_PATH"

$XTERM $OUT >> $NCS_STDOUTS_PATH/pxy_pxd_demo.log 2>&1 &

exit 0


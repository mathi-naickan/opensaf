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

if [ "$1" == "h" ]; then 
	echo usage: $0 [installDir] [outDir]
	exit
fi

INSTALLDIR=/opt/aisapp
if [ -n "$1" ]; then 
	INSTALLDIR=$1
fi


if [ $(expr match "$INSTALLDIR" '/*') -ne 1 ]; then
	echo "ERROR: installDir needs to be an absolute path"
	exit
fi

OUTDIR=deploy
if [ ! -z "$2" ]; then 
	OUTDIR=$2
fi

echo "Generating java scripts and configuration files..."
echo "INFO Install directory: $INSTALLDIR"
echo "INFO Config output directory: $OUTDIR"

echo "1. Creating output directory"
rm -Rf $OUTDIR
mkdir -p $OUTDIR
mkdir -p $OUTDIR/config
mkdir -p $OUTDIR/lib
mkdir -p $OUTDIR/var
mkdir -p $OUTDIR/log
mkdir -p $OUTDIR/amfscripts


echo $INSTALLDIR > $OUTDIR/.installConf

echo "2. Generating amf-test-config.xml."
cat amf-test-config.xml.template | sed "s|\[PATH_TO_INSTALL\]|$INSTALLDIR|" > $OUTDIR/config/amf-test-config.xml

echo "3. Generating common script config."
cat scripts.conf.template | sed "s|\[PATH_TO_INSTALL]|$INSTALLDIR|" > $OUTDIR/config/scripts.conf

# Select JUnit library
if [ -e ../util/junit.jar ]; then
	JUNITLIB=../util/junit.jar
else
	echo -n "Enter location of JUnit library: "
	read JUNITLIB
	if [ ! -e $JUNITLIB ]; then
		echo "ERROR: $JUNITLIB does not exist!"
		exit -1;
	fi
fi
echo "4. Using JUnit library: $JUNITLIB"
cp $JUNITLIB $OUTDIR/lib

# Copy native library
cp ${PWD}/../../ais_api_impl_native/.libs/libjava_ais_api_native.so $OUTDIR/lib/libjava_ais_api_native.so
if [ $? -ne 0 ]; then
	echo "ERROR: ../../ais_api_impl_native/.libs/libjava_ais_api_native.so could not be found. Build the native library first."
	exit -1
fi


echo "5. Copying test archive."
cp ${PWD}/../bin/opensaf_ais_api_test.jar $OUTDIR/opensaf_ais_api_test.jar
if [ $? -ne 0 ]; then
	echo "ERROR: ../bin/opensaf_ais_api_test.jar could not be found. Build the test application first."
	exit -1
fi

echo "6. Generating CLM test runner script."
cat runClmTests.sh.template | sed "s|\[PATH_TO_INSTALL]|$INSTALLDIR|" > $OUTDIR/runClmTests.sh
chmod +x $OUTDIR/runClmTests.sh

echo "7. Generating AMF test component control scripts."
cat java_comp_inst.sh.template | sed "s|\[PATH_TO_INSTALL]|$INSTALLDIR|" > $OUTDIR/amfscripts/java_comp_inst.sh
chmod +x $OUTDIR/amfscripts/java_comp_inst.sh
cat java_comp_term.sh.template | sed "s|\[PATH_TO_INSTALL]|$INSTALLDIR|" > $OUTDIR/amfscripts/java_comp_term.sh
chmod +x $OUTDIR/amfscripts/java_comp_term.sh

echo "8. Creating deploy archive."
tar --exclude .installConf -czf $OUTDIR.tar.gz -C $OUTDIR/ .

echo "DONE Output configuration generated to dir: $OUTDIR"
echo


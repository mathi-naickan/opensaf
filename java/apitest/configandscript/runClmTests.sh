#!/bin/sh
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
# This script is used for executing the Java API CLM tests.


# JUnit 3.2.8 library
JUNIT_LIB=[PATH_TO_JUNIT]/junit.jar
# The directory where the API files can be found. The script expects the
# opensaf_ais_api.jar and the native library to reside in this directory.
AIS_API_DIR=[PATH_TO_JAVA_AIS_API]

# The standard API and the implementation are packed in one archive.
AIS_API_JAR=${AIS_API_DIR}/opensaf_ais_api.jar
# The native library is in the API directory.
AIS_API_NATIVE_DIR=${AIS_API_DIR}/.
# Path to the executed classes (directory or jar file)
BIN_CLASSES=${AIS_API_DIR}/opensaf_ais_api_test.jar

XTERM=
# Use this for debugging
#XTERM="xterm gdb"

COMP_PID_MAP_FILE="/var/tmp/java_test_comp_pid_map"

JAVA="java"

# Define Java options:
#   SAF_AIS_CLM_IMPL_CLASSNAME: name of the CLM handle implementation class (optional, only required if CLM is used by component)
#   SAF_AIS_CLM_IMPL_URL: url to the CLM handle implmementation class (jar file or the fully qualified path without .class extension)
#   SAF_AIS_AMF_IMPL_CLASSNAME: name of the AMF handle implementation class
#   SAF_AIS_AMF_IMPL_URL: url to the AMF handle implmementation class (jar file or the fully qualified path without .class extension)
#   java.library.path: list of directories for natile library loading
#   nativeLibrary: name of the native library without the lib prefix and the extension / this is specific for the OpenSAF java implementation
JAVA_OPTIONS="-DSAF_AIS_CLM_IMPL_CLASSNAME=org.opensaf.ais.clm.ClmHandleImpl \
              -DSAF_AIS_CLM_IMPL_URL=file:///${AIS_API_JAR} \
              -Djava.library.path=${AIS_API_NATIVE_DIR} \
              -DnativeLibrary=Java_AIS_API_native"
#JAVA_OPTIONS for debugging
#JAVA_OPTIONS="-DSAF_AIS_CLM_IMPL_CLASSNAME=org.opensaf.ais.clm.ClmHandleImpl \
#              -DSAF_AIS_CLM_IMPL_URL=file:///${AIS_API_JAR} \
#              -Djava.library.path=${AIS_API_NATIVE_DIR} \
#              -DnativeLibrary=Java_AIS_API_native_debug"



# Execute the component.
java ${JAVA_OPTIONS} \
-classpath ${AIS_API_JAR}:${JUNIT_LIB}:${BIN_CLASSES} org.opensaf.ais.clm.test.AllTests

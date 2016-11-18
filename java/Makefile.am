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
# Author(s): Wind River Systems
#

EXTRA_DIST += \
	java/INSTALL \
	java/TODO \
	java/ais_api/src \
	java/ais_api/overview.html \
	java/ais_api/build.xml \
	java/ais_api_impl/src \
	java/ais_api_impl/build.xml \
	java/apitest/src \
	java/apitest/configandscript \
	java/apitest/build.xml \
	java/amf_agent/src \
	java/amf_agent/build.xml

if ENABLE_JAVA

BUILT_SOURCES += java/ais_api_impl/bin/opensaf_ais_api.jar

nodist_java_DATA += java/ais_api_impl/bin/opensaf_ais_api.jar

if ENABLE_AM4J

BUILT_SOURCES += java/amf_agent/bin/amf_agent.jar

nodist_java_DATA += java/amf_agent/bin/amf_agent.jar

endif

endif

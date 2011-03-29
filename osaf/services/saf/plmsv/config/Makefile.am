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
# Author(s): Emerson 
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	plmsv_objects.xml.atca \
	plmsv_objects.xml.hp \
	plmsv_objects.xml.pc

dist_pkgsysconf_DATA = \
	$(top_srcdir)/osaf/services/saf/plmsv/config/plmd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA = \
	plmsv_classes.xml \
	plmsv_objects.xml \
	plmsv_sc_template.xml \
	plmsv_pl_template.xml

endif

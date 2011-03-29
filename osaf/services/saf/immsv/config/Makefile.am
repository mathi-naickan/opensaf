#      -*- OpenSAF  -*-
#
# (C) Copyright 2009 The OpenSAF Foundation
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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

dist_pkgsysconf_DATA = \
	$(top_srcdir)/osaf/services/saf/immsv/config/immd.conf \
	$(top_srcdir)/osaf/services/saf/immsv/config/immnd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA = \
	immsv_sc_template.xml \
	immsv_pl_template.xml \
	immsv_classes.xml \
	immsv_objects.xml \
	imm_copyright.xml

endif

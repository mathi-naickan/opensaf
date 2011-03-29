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
# Author(s): Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA = \
	mqsv_sc_template.xml \
	mqsv_pl_template.xml \
	mqsv_classes.xml \
	mqsv_objects.xml

endif

dist_pkgsysconf_DATA = \
	$(top_srcdir)/osaf/services/saf/mqsv/config/msgd.conf \
	$(top_srcdir)/osaf/services/saf/mqsv/config/msgnd.conf

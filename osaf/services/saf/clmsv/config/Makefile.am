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
# Author(s):  Emerson Network Power
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

dist_pkgsysconf_DATA = \
	$(top_srcdir)/osaf/services/saf/clmsv/config/clmd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA = \
	clmsv_classes.xml \
	clmsv_objects.xml

if ENABLE_AIS_PLM
dist_pkgimmxml_svc_DATA += \
	clmsv_plm_sc_template.xml \
	clmsv_plm_pl_template.xml
else
dist_pkgimmxml_svc_DATA += \
	clmsv_sc_template.xml \
	clmsv_pl_template.xml
endif

endif

install-data-hook:
	hostname -s > $(DESTDIR)$(pkgsysconfdir)/node_name

uninstall-hook:
	rm -f $(DESTDIR)$(pkgsysconfdir)/node_name

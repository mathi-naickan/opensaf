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

MAINTAINERCLEANFILES = \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltmain.sh \
	missing \
	Makefile.in \
	m4/libtool.m4 \
	m4/ltoptions.m4 \
	m4/ltsugar.m4 \
	m4/ltversion.m4 \
	m4/lt~obsolete.m4

CLEANFILES = \
	configmake.h \
	configmake.h-t \
	osafdir.conf \
	osafdir.conf-t

include $(top_srcdir)/Makefile.common

ACLOCAL_AMFLAGS = -I m4

# Retrieve values of the variables through 'configure' followed by
# 'make', not directly through 'configure', so that a user who
# sets some of these variables consistently on the 'make' command
# line gets correct results.
#
# One advantage of this approach, compared to the classical
# approach of adding -DLIBDIR=\"$(libdir)\" etc. to AM_CPPFLAGS,
# is that it protects against the use of undefined variables.
# If, say, $(libdir) is not set in the Makefile, LIBDIR is not
# defined by this module, and code using LIBDIR gives a
# compilation error.
#
# Another advantage is that 'make' output is shorter.
#
# Listed in the same order as the GNU makefile conventions.
# The Automake-defined pkg* macros are appended, in the order
# listed in the Automake 1.10a+ documentation.
configmake.h: Makefile
	@rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	echo '#define PREFIX "$(prefix)"'; \
	echo '#define EXEC_PREFIX "$(exec_prefix)"'; \
	echo '#define BINDIR "$(bindir)"'; \
	echo '#define SBINDIR "$(sbindir)"'; \
	echo '#define LIBEXECDIR "$(libexecdir)"'; \
	echo '#define DATAROOTDIR "$(datarootdir)"'; \
	echo '#define DATADIR "$(datadir)"'; \
	echo '#define SYSCONFDIR "$(sysconfdir)"'; \
	echo '#define SHAREDSTATEDIR "$(sharedstatedir)"'; \
	echo '#define LOCALSTATEDIR "$(localstatedir)"'; \
	echo '#define INCLUDEDIR "$(includedir)"'; \
	echo '#define OLDINCLUDEDIR "$(oldincludedir)"'; \
	echo '#define DOCDIR "$(docdir)"'; \
	echo '#define INFODIR "$(infodir)"'; \
	echo '#define HTMLDIR "$(htmldir)"'; \
	echo '#define DVIDIR "$(dvidir)"'; \
	echo '#define PDFDIR "$(pdfdir)"'; \
	echo '#define PSDIR "$(psdir)"'; \
	echo '#define LIBDIR "$(libdir)"'; \
	echo '#define LISPDIR "$(lispdir)"'; \
	echo '#define LOCALEDIR "$(localedir)"'; \
	echo '#define MANDIR "$(mandir)"'; \
	echo '#define MANEXT "$(manext)"'; \
	echo '#define PKGDATADIR "$(pkgdatadir)"'; \
	echo '#define PKGINCLUDEDIR "$(pkgincludedir)"'; \
	echo '#define PKGLIBDIR "$(pkglibdir)"'; \
	echo '#define PKGLIBEXECDIR "$(pkglibexecdir)"'; \
	echo '#define PKGSYSCONFDIR "$(pkgsysconfdir)"'; \
	echo '#define PKGLOCALSTATEDIR "$(pkglocalstatedir)"'; \
	echo '#define PKGPIDDIR "$(pkgpiddir)"'; \
	echo '#define PKGLOGDIR "$(pkglogdir)"'; \
	echo '#define PKGCONFDIR "$(pkgconfigdir)"'; \
	echo '#define PKGIMMXMLDIR "$(pkgimmxmldir)"'; \
	echo '#define PKGIMMXMLSVCDIR "$(pkgimmxml_svcdir)"'; \
	echo '#define PKGCLCCLIDIR "$(pkgclcclidir)"'; \
	echo '#define JAVADIR "$(javadir)"'; \
	echo '#define JAVADOCDIR "$(javadocdir)"'; \
	echo '#define LSBINITDIR "$(lsbinitdir)"'; \
	echo '#define LOCKDIR "$(lockdir)"'; \
	} | sed '/""/d' > $@-t
	mv $@-t $@


osafdir.conf: configmake.h
	@rm -f $@-t $@
	@cp configmake.h $@-t
	@sed -i 's|\#define ||g' $@-t
	@sed -i 's| \"|=\"|g' $@-t
	@sed -i -e 's|\/\* |\# |g' -e 's| \*\/||g' $@-t
	@sed -i 1i"#!/bin/sh" $@-t
	sed -e 's/\(.*\)/\L\1/' $@-t > $@


BUILT_SOURCES = configmake.h osafdir.conf

dist_doc_DATA = \
	$(top_srcdir)/AUTHORS \
	$(top_srcdir)/ChangeLog \
	$(top_srcdir)/COPYING.LIB \
	$(top_srcdir)/INSTALL \
	$(top_srcdir)/NEWS \
	$(top_srcdir)/THANKS \
	$(top_srcdir)/README \
	$(top_srcdir)/00-README.uml \
	$(top_srcdir)/00-README.conf \
	$(top_srcdir)/00-README.samples \
	$(top_srcdir)/00-README.debug

if ENABLE_AIS_PLM

SUBDIRS = contrib/plmc osaf pkgconfig

else

SUBDIRS = osaf pkgconfig

endif

if ENABLE_JAVA

SUBDIRS += java

endif

EXTRA_DIST = samples

if ENABLE_TESTS

EXTRA_DIST += tests

SUBDIRS += \
	tests/logsv \
	tests/ntfsv \
	tests/immsv \
	tests/immsv/implementer \
	tests/immsv/management \
	tests/clmsv \
	tests/unit_test_fw

if ENABLE_AIS_PLM
SUBDIRS += \
	tests/plmsv
endif

endif

nodist_pkgsysconf_DATA = \
	$(top_builddir)/osafdir.conf

dist_pkglib_SCRIPTS = \
	$(top_srcdir)/scripts/opensaf_reboot 

if ENABLE_RPM_TARGET

RPMTOPDIR = `pwd`/rpms
RPMSOURCEDIR = $(RPMTOPDIR)/SOURCES
RPMSPECDIR = $(RPMTOPDIR)/SPECS
SRPMDIR = $(RPMTOPDIR)/SRPMS
RPMDIR = $(RPMTOPDIR)/RPMS

prep-rpm-dir: dist
	mkdir -p $(RPMTOPDIR)
	mkdir -p $(RPMTOPDIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp}
	cp $(top_srcdir)/$(PACKAGE_NAME).spec $(RPMSPECDIR)
	cp $(top_srcdir)/$(PACKAGE_NAME)-$(VERSION).tar.gz $(RPMSOURCEDIR)

srpm: prep-rpm-dir
	rpmbuild -bs --rmspec --rmsource \
		--define "_topdir $(RPMTOPDIR)" --define "_tmppath $(RPMTOPDIR)/tmp" \
		$(RPMSPECDIR)/$(PACKAGE_NAME).spec

rpm: prep-rpm-dir
	rpmbuild -bb --clean --rmspec --rmsource \
		--define "_topdir $(RPMTOPDIR)" --define "_tmppath $(RPMTOPDIR)/tmp" \
		$(RPMSPECDIR)/$(PACKAGE_NAME).spec

endif

all-local:
	echo "$(pkglibdir)" > $(top_builddir)/osaf/libs/$(PACKAGE_NAME)-$(host_cpu).conf

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkglogdir)
	$(mkinstalldirs) $(DESTDIR)$(pkgpiddir)
	$(mkinstalldirs) $(DESTDIR)$(lockdir)
	$(mkinstalldirs) $(DESTDIR)$(pkglocalstatedir)
	cp -R samples/ $(DESTDIR)$(pkgdatadir)

install-data-hook:
	@for i in $$(grep -lr -e 'xxLIBDIRxx' -e 'xxCLCCLIDIRxx' $(DESTDIR)$(pkgimmxml_svcdir)/*.xml) ; do \
		sed -i 's|xxLIBDIRxx|$(pkglibdir)|g' "$$i"; \
		sed -i 's|xxCLCCLIDIRxx|$(pkgclcclidir)|g' "$$i"; \
	done
	@for i in $$(grep -lr -e 'xxBINDIRxx' -e 'xxLIBDIRxx' -e 'xxCLCCLIDIRxx' $(DESTDIR)$(pkgsysconfdir)/*) ; do \
		sed -i 's|xxBINDIRxx|$(bindir)|g' "$$i"; \
		sed -i 's|xxLIBDIRxx|$(pkglibdir)|g' "$$i"; \
		sed -i 's|xxCLCCLIDIRxx|$(pkgclcclidir)|g' "$$i"; \
	done

uninstall-hook:
	-rm -rf $(DESTDIR)$(pkgsysconfdir)
	-rm -rf $(DESTDIR)$(pkgincludedir)
	-rm -rf $(DESTDIR)$(pkglibdir)
	-rm -rf $(DESTDIR)$(docdir)
	-rm -rf $(DESTDIR)$(pkgdatadir)
	-rm -rf $(DESTDIR)$(pkglocalstatedir)
	-rm -rf $(DESTDIR)$(pkglogdir)
	-rm -rf $(DESTDIR)$(pkgpiddir)

docs:
	doxygen

clean-local:
	-rm -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
if ENABLE_RPM_TARGET
	-rm -rf $(RPMTOPDIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp}
endif

distclean-local:
	-rm -f $(top_builddir)/osaf/libs/$(PACKAGE_NAME)-$(host_cpu).conf
	-rm -rf docs/

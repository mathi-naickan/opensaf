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
#            Ericsson AB
#

MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	compile \
	config.guess \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltmain.sh \
	m4/libtool.m4 \
	m4/ltoptions.m4 \
	m4/ltsugar.m4 \
	m4/ltversion.m4 \
	m4/lt~obsolete.m4 \
	missing \
	py-compile \
	src/osaf/config.h.in \
	test-driver

CLEANFILES = \
	cppcheck.xml \
	cppcheck_includes.txt \
	lib/$(PACKAGE_NAME)-$(host_cpu).conf \
	osafdir.conf \
	osafdir.conf-t \
	src/osaf/configmake.h

CORE_INCLUDES = \
	-I$(top_builddir)/src -I$(top_srcdir)/src

AM_CPPFLAGS = \
	-D_GNU_SOURCE -DINTERNAL_VERSION_ID='"@INTERNAL_VERSION_ID@"' \
	$(CORE_INCLUDES) \
	$(all_includes)

AM_CFLAGS = -std=gnu11 @OSAF_HARDEN_FLAGS@ -Wall -Wformat=2 -Werror
AM_CXXFLAGS = -std=gnu++11 @OSAF_HARDEN_FLAGS@ -Wall -Wformat=2 -Werror
AM_LDFLAGS = @OSAF_HARDEN_FLAGS@ -ldl -lrt -lpthread -rdynamic
ACLOCAL_AMFLAGS = -I m4
OSAF_LIB_FLAGS =

systemdversion = $(systemdmodversion)
#
# Define some useful directory prefixes/suffixes
#
pkgsysconfdir = $(sysconfdir)/$(PACKAGE_NAME)
pkglocalstatedir = $(localstatedir)/lib/$(PACKAGE_NAME)
pkgpiddir = $(localstatedir)/run/$(PACKAGE_NAME)
pkglogdir = $(localstatedir)/log/$(PACKAGE_NAME)
pkgconfigdir = $(libdir)/pkgconfig
pkgimmxmldir = $(pkgdatadir)/immxml
pkgimmxml_svcdir = $(pkgimmxmldir)/services
pkgclcclidir = $(pkglibdir)/clc-cli
docdir = $(datadir)/doc/$(PACKAGE_NAME)-$(PACKAGE_VERSION)
javadir = $(datadir)/java
javadocdir = $(datadir)/javadoc
lsbinitdir = $(initscriptdir)
systemddir = $(systemdsystemunitdir)
lockdir = $(localstatedir)/lock/subsys
pkgpyosafdir = $(pythondir)/pyosaf
pkgpyosafutilsdir = $(pythondir)/pyosaf/utils
pkgpyosafutilsimmomdir = $(pythondir)/pyosaf/utils/immom
pkgpyosafutilsclmdir = $(pythondir)/pyosaf/utils/clm
pkgpyosafutilslogdir = $(pythondir)/pyosaf/utils/log
pkgpyosafutilsimmoidir = $(pythondir)/pyosaf/utils/immoi
pkgpyosafutilsntfdir = $(pythondir)/pyosaf/utils/ntf
osaf_execbindir = $(pkglibdir)

BUILT_SOURCES =
DEFAULT_INCLUDES =
EXTRA_DIST =
SUBDIRS =
TESTS =
bin_PROGRAMS =
dist_bin_SCRIPTS =
dist_doc_DATA =
dist_lsbinit_SCRIPTS =
dist_osaf_execbin_SCRIPTS =
dist_pkgimmxml_svc_DATA =
dist_pkgsysconf_DATA =
dist_sysconf_DATA =
dist_systemd_DATA =
include_HEADERS =
lib_LTLIBRARIES =
lib_libopensaf_core_la_CPPFLAGS =
lib_libopensaf_core_la_LDFLAGS =
lib_libopensaf_core_la_LIBADD =
lib_libopensaf_core_la_SOURCES =
nodist_java_DATA =
nodist_osaf_execbin_SCRIPTS =
nodist_pkgclccli_SCRIPTS =
nodist_pkgsysconf_DATA =
noinst_HEADERS =
noinst_LTLIBRARIES =
osaf_execbin_PROGRAMS =
pkgconfig_DATA =
pkginclude_HEADERS =
pkglib_LTLIBRARIES =
sbin_PROGRAMS =

nodist_pkgsysconf_DATA += \
	$(top_builddir)/osafdir.conf

pkgconfig_DATA += pkgconfig/opensaf.pc

BUILT_SOURCES += src/osaf/configmake.h osafdir.conf

EXTRA_DIST += samples

dist_doc_DATA += \
	$(top_srcdir)/00-README.conf \
	$(top_srcdir)/00-README.debug \
	$(top_srcdir)/00-README.samples \
	$(top_srcdir)/00-README.uml \
	$(top_srcdir)/00-README.unittest \
	$(top_srcdir)/AUTHORS \
	$(top_srcdir)/COPYING.LIB \
	$(top_srcdir)/ChangeLog \
	$(top_srcdir)/INSTALL \
	$(top_srcdir)/NEWS \
	$(top_srcdir)/README \
	$(top_srcdir)/THANKS

dist_osaf_execbin_SCRIPTS += \
	$(top_srcdir)/scripts/opensaf_reboot \
	$(top_srcdir)/scripts/opensaf_scale_out \
	$(top_srcdir)/scripts/opensaf_sc_active

include $(top_srcdir)/src/base/Makefile.am
include $(top_srcdir)/src/osaf/Makefile.am
include $(top_srcdir)/src/amf/Makefile.am
include $(top_srcdir)/src/ckpt/Makefile.am
include $(top_srcdir)/src/clm/Makefile.am
include $(top_srcdir)/src/dtm/Makefile.am
include $(top_srcdir)/src/evt/Makefile.am
include $(top_srcdir)/src/fm/Makefile.am
include $(top_srcdir)/src/imm/Makefile.am
include $(top_srcdir)/src/lck/Makefile.am
include $(top_srcdir)/src/libjava/Makefile.am
include $(top_srcdir)/src/log/Makefile.am
include $(top_srcdir)/src/mbc/Makefile.am
include $(top_srcdir)/src/mds/Makefile.am
include $(top_srcdir)/src/msg/Makefile.am
include $(top_srcdir)/src/nid/Makefile.am
include $(top_srcdir)/src/ntf/Makefile.am
include $(top_srcdir)/src/plm/Makefile.am
include $(top_srcdir)/src/rde/Makefile.am
include $(top_srcdir)/src/smf/Makefile.am
include $(top_srcdir)/java/Makefile.am
include $(top_srcdir)/python/Makefile.am

check_PROGRAMS = $(TESTS)

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
src/osaf/configmake.h: Makefile
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


osafdir.conf: src/osaf/configmake.h
	@rm -f $@-t $@
	@cp src/osaf/configmake.h $@-t
	@sed -i 's|\#define ||g' $@-t
	@sed -i 's| \"|=\"|g' $@-t
	@sed -i -e 's|\/\* |\# |g' -e 's| \*\/||g' $@-t
	@sed -i 1i"#!/bin/sh" $@-t
	sed -e 's/\(.*\)/\L\1/' $@-t > $@

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
	echo "$(pkglibdir)" > $(top_builddir)/lib/$(PACKAGE_NAME)-$(host_cpu).conf

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkglogdir)
	$(mkinstalldirs) $(DESTDIR)$(pkglogdir)/dtlog
	$(mkinstalldirs) $(DESTDIR)$(pkglogdir)/saflog
	$(mkinstalldirs) $(DESTDIR)$(pkgpiddir)
	$(mkinstalldirs) $(DESTDIR)$(lockdir)
	$(mkinstalldirs) $(DESTDIR)$(pkglocalstatedir)
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	cp -R $(top_srcdir)/samples/ $(DESTDIR)$(pkgdatadir)
if ENABLE_JAVA
	$(mkinstalldirs) $(DESTDIR)$(javadocdir)/$(PACKAGE_NAME)
	@builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); \
	cd java/ais_api/javadoc; \
	for file in `find . -name \*`; do \
		$(INSTALL) -D -m 644 "$$builddir/java/ais_api/javadoc/$$file" "$(DESTDIR)$(javadocdir)/$(PACKAGE_NAME)/$$file.txt"; \
	done; \
	cd -
endif

set-default-node-config:
	@echo "*** Setting default controller node configuration ***"
	echo "2" > $(DESTDIR)$(pkgsysconfdir)/chassis_id
	echo "1" > $(DESTDIR)$(pkgsysconfdir)/slot_id
	echo "15" > $(DESTDIR)$(pkgsysconfdir)/subslot_id
	echo "controller" > $(DESTDIR)$(pkgsysconfdir)/node_type

install-data-hook: set-default-node-config
	@for i in $$(grep -lr -e 'xxLIBDIRxx' -e 'xxLOGDIRxx' -e 'xxCLCCLIDIRxx' $(DESTDIR)$(pkgimmxml_svcdir)/*.xml) ; do \
		sed -i 's|xxLIBDIRxx|$(pkglibdir)|g' "$$i"; \
		sed -i 's|xxCLCCLIDIRxx|$(pkgclcclidir)|g' "$$i"; \
		sed -i 's|xxLOGDIRxx|$(pkglogdir)|g' "$$i"; \
	done
	@for i in $$(grep -lr -e 'xxBINDIRxx' -e 'xxLIBDIRxx' -e 'xxCLCCLIDIRxx' $(DESTDIR)$(pkgsysconfdir)/*) ; do \
		sed --follow-symlinks -i 's|xxBINDIRxx|$(bindir)|g' "$$i"; \
		sed --follow-symlinks -i 's|xxLIBDIRxx|$(pkglibdir)|g' "$$i"; \
		sed --follow-symlinks -i 's|xxCLCCLIDIRxx|$(pkgclcclidir)|g' "$$i"; \
	done
	hostname -s > $(DESTDIR)$(pkgsysconfdir)/node_name
if !ENABLE_AIS_PLM
	sed -i '/plmd/d'  $(DESTDIR)$(pkgsysconfdir)/nodeinit.conf.controller
endif
if ENABLE_SYSTEMD
	if [ ${systemdmodversion} -gt 204 ]; then \
		sed -i '/ControlGroup/d'  $(DESTDIR)$(systemddir)/opensafd.service; \
	fi
endif

uninstall-hook:
	rm -f $(DESTDIR)$(pkgsysconfdir)/node_name
	-rm -rf $(DESTDIR)$(pkgsysconfdir)
	-rm -rf $(DESTDIR)$(pkgincludedir)
	-rm -rf $(DESTDIR)$(pkglibdir)
	-rm -rf $(DESTDIR)$(docdir)
	-rm -rf $(DESTDIR)$(pkgdatadir)
	-rm -rf $(DESTDIR)$(pkglocalstatedir)
	-rm -rf $(DESTDIR)$(pkglogdir)
	-rm -rf $(DESTDIR)$(pkgpiddir)
if ENABLE_JAVA
	-rm -rf $(DESTDIR)$(javadocdir)/$(PACKAGE_NAME)
	-rm -rf $(DESTDIR)$(javadir)
if ENABLE_AM4J
	-rm -rf $(DESTDIR)$(javadir)
endif
endif


docs:
	doxygen

cppcheck_includes.txt:
	@builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); \
	srcdir=$$(cd $(top_srcdir); pwd; cd - > /dev/null); \
	echo "src" > "$$builddir/cppcheck_includes.txt"; \
	cd "$$srcdir"; find "src" -type d -name saf >> "$$builddir/cppcheck_includes.txt"; \
	echo "$$builddir/src" >> "$$builddir/cppcheck_includes.txt"

cppcheck.xml: cppcheck_includes.txt
	@builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); \
	srcdir=$$(cd $(top_srcdir); pwd; cd - > /dev/null); \
	cd "$$srcdir"; cppcheck -DSA_EXTENDED_NAME_SOURCE -D__USE_GNU -D__cplusplus=201103L -DIMMA_OI -DIMMA_OM -DASAPi_DEBUG=1 -DHAVE_CONFIG_H -DNCS_CPD=1 -DNCS_CPND=1 -DNCSL_ENV_INIT_HM=1 -DNCSL_ENV_INIT_TMR=1 -DNCS_MQD=1 -DNCS_MQND=1 -DRLIMIT_RTPRIO -DTIPC_SUB_CANCEL -DENABLE_AIS_PLM -DLIBXML_SAX1_ENABLED -DLIBXML_XPATH_ENABLED --enable=all --includes-file="$$builddir/cppcheck_includes.txt" --inline-suppr --library=gnu.cfg --library=posix.cfg --xml-version=2 src cppcheck_append.cc 2> "$$builddir/cppcheck.xml"
	@echo "The result can be found in the file cppcheck.xml"
	@echo "Use the follwing command to view it: cppcheck-gui cppcheck.xml"

cppcheck: cppcheck_includes.txt
	@builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); \
	srcdir=$$(cd $(top_srcdir); pwd; cd - > /dev/null); \
	cd "$$srcdir"; cppcheck -DSA_EXTENDED_NAME_SOURCE -D__USE_GNU -D__cplusplus=201103L -DIMMA_OI -DIMMA_OM -DASAPi_DEBUG=1 -DHAVE_CONFIG_H -DNCS_CPD=1 -DNCS_CPND=1 -DNCSL_ENV_INIT_HM=1 -DNCSL_ENV_INIT_TMR=1 -DNCS_MQD=1 -DNCS_MQND=1 -DRLIMIT_RTPRIO -DTIPC_SUB_CANCEL -DENABLE_AIS_PLM -DLIBXML_SAX1_ENABLED -DLIBXML_XPATH_ENABLED --enable=all --includes-file="$$builddir/cppcheck_includes.txt" --inline-suppr --library=gnu.cfg --library=posix.cfg src cppcheck_append.cc

cpplint:
	@test -d $(top_builddir)/bin || mkdir $(top_builddir)/bin
	@cpplint=$$(find $(top_builddir)/bin -name cpplint.py -mtime -30); \
	test -z "$$cpplint" && wget -O $(top_builddir)/bin/cpplint.py https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py 2>&1 && touch $(top_builddir)/bin/cpplint.py; \
	builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); \
	srcdir=$$(cd $(top_srcdir); pwd; cd - > /dev/null); \
	cd "$$srcdir"; find src -path '*/.[a-zA-Z0-9]*' -or -path '*/cluster_sim_uml/*' -prune -or \( -name '*.[CH]' -or -name '*.hh' -or -name '*.[ch]pp' -or -name '*.[ch]xx' \) -exec echo "Invalid file name: {}" \; 1>&2 ; \
	cd "$$srcdir"; find src -path '*/.[a-zA-Z0-9]*' -or -path '*/cluster_sim_uml/*' -or -path '*/saf/*' -or -path 'src/osaf/config*.h' -or -path 'src/libjava/jni_ais*.h' -prune -or \( -name '*.cc' -or -name '*.h' \) -exec python "$$builddir/bin/cpplint.py" --root=src --extensions=cc,h {} \; ; \
	true

shellcheck:
	@builddir=$$(cd $(top_builddir); pwd; cd - > /dev/null); $(MAKE) install DESTDIR="$$builddir/tmpinstall"
	@cd "$$builddir/tmpinstall"; files=$$(grep --exclude-dir=cluster_sim_uml --exclude-dir='.[a-zA-Z0-9]*' -E '^#\! */bin/(ba)?sh' -rlI .); \
	if test -n "$$files"; then \
		shellcheck --shell=sh $$files; \
	fi; \
	true

.PHONY: cppcheck.xml cppcheck.txt cpplint shellcheck

clean-local:
	-rm -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
if ENABLE_RPM_TARGET
	-rm -rf $(RPMTOPDIR)/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}
endif
if ENABLE_JAVA
	$(ANT) -f $(top_srcdir)/java/ais_api/build.xml clean
	-rm -rf $(top_builddir)/java/ais_api/javadoc $(top_builddir)/java/ais_api/bin
	$(ANT) -f $(top_srcdir)/java/ais_api_impl/build.xml clean
	-rm -rf $(top_builddir)/java/ais_api_impl/bin
if ENABLE_AM4J
	$(ANT) -f $(top_srcdir)/java/amf_agent/build.xml clean
	-rm -rf $(top_builddir)/java/amf_agent/bin
endif
endif

distclean-local:
	-rm -f $(top_builddir)/docs $(top_builddir)/rpms $(top_builddir)/bin $(top_builddir)/lib

maintainer-clean-local:
	-rm -f $(top_builddir)/autom4te.cache

java/ais_api_impl/bin/opensaf_ais_api.jar:
	$(ANT) -f $(top_srcdir)/java/ais_api_impl/build.xml build-jar
	$(ANT) -f $(top_srcdir)/java/ais_api/build.xml javadocAPI

java/amf_agent/bin/amf_agent.jar:
	$(ANT) -f $(top_srcdir)/java/amf_agent/build.xml build-jar -Dam4j_api.location=$(AM4J_API_JAR)

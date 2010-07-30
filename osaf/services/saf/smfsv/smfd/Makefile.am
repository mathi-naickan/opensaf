#      OpenSAF
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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

pkglib_PROGRAMS = osafsmfd

noinst_HEADERS = \
	smfd_cb.h \
	smfd_evt.h \
	smfd_smfnd.h \
	smfd.h \
	SmfCampaignXmlParser.hh \
	SmfUpgradeCampaign.hh \
	SmfUpgradeProcedure.hh \
	SmfUpgradeAction.hh \
	SmfImmOperation.hh \
	SmfCampaignThread.hh \
	SmfProcedureThread.hh \
	SmfCampaign.hh \
	SmfCampState.hh \
	SmfTargetTemplate.hh \
	SmfProcState.hh \
	SmfUpgradeStep.hh \
	SmfStepState.hh \
	SmfUpgradeMethod.hh \
	SmfUtils.hh \
	SmfCampaignWrapup.hh \
	SmfCampaignInit.hh

osafsmfd_CXXFLAGS = $(AM_CXXFLAGS) @XML2_CFLAGS@

osafsmfd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/smfsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafsmfd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	smfd_amf.c \
	smfd_evt.c \
	smfd_main.c \
	smfd_smfnd.c \
	smfd_mds.c \
	SmfCampaignThread.cc \
	SmfProcedureThread.cc \
	SmfCampaign.cc \
	smfd_campaign_oi.cc \
	SmfCampaignXmlParser.cc \
	SmfUpgradeCampaign.cc \
	SmfUpgradeProcedure.cc \
	SmfCampState.cc \
	SmfProcState.cc \
	SmfImmOperation.cc \
	SmfTargetTemplate.cc \
	SmfUpgradeAction.cc \
	SmfUpgradeStep.cc \
	SmfStepState.cc \
	SmfUpgradeMethod.cc \
	SmfUtils.cc \
	SmfCampaignInit.cc \
	SmfCampaignWrapup.cc

osafsmfd_LDFLAGS = \
	$(AM_LDFLAGS) \
	@XML2_LIBS@

osafsmfd_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la \
	$(top_builddir)/osaf/libs/saf/libSaNtf/libSaNtf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/smfsv/libsmfsv_common.la

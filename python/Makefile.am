#      -*- OpenSAF  -*-
#
# (C) Copyright 2011 The OpenSAF Foundation
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
	python/Makefile.mk \
	python/MANIFEST.in \
	python/README \
	python/setup.py

if ENABLE_PYTHON

pkgpyosaf_PYTHON = \
	python/pyosaf/__init__.py \
	python/pyosaf/saAis.py \
	python/pyosaf/saAmf.py \
	python/pyosaf/saCkpt.py \
	python/pyosaf/saClm.py \
	python/pyosaf/saEnumConst.py \
	python/pyosaf/saEvt.py \
	python/pyosaf/saImmOi.py \
	python/pyosaf/saImmOm.py \
	python/pyosaf/saImm.py \
	python/pyosaf/saLck.py \
	python/pyosaf/saLog.py \
	python/pyosaf/saMsg.py \
	python/pyosaf/saNtf.py \
	python/pyosaf/saSmf.py

pkgpyosafutils_PYTHON = \
	python/pyosaf/utils/__init__.py

pkgpyosafutilsimmom_PYTHON = \
	python/pyosaf/utils/immom/__init__.py \
	python/pyosaf/utils/immom/ccb.py \
	python/pyosaf/utils/immom/iterator.py \
	python/pyosaf/utils/immom/object.py

pkgpyosafutilsclm_PYTHON = \
	python/pyosaf/utils/clm/__init__.py

pkgpyosafutilslog_PYTHON = \
	python/pyosaf/utils/log/__init__.py \
	python/pyosaf/utils/log/logger.py

pkgpyosafutilsimmoi_PYTHON = \
	python/pyosaf/utils/immoi/__init__.py \
	python/pyosaf/utils/immoi/implementer.py

pkgpyosafutilsntf_PYTHON = \
	python/pyosaf/utils/ntf/__init__.py

endif

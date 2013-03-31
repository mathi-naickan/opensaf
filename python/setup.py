#####################################################################
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
# Author(s): Wind River Systems, Inc.
#
############################################################################

from setuptools import setup, find_packages
from setuptools.command.install import install as _install

class install(_install):
    def run(self):
        _install.run(self)

name = 'pyosaf'
version = '0.1.5'
description = 'OpenSAF Python Bindings for SAF-AIS API'

setup(
	name=name,
	version=version,
	description=description,

	author = 'Wind River Systems, Inc.',
	author_email = 'opensaf-devel@lists.sourceforge.net',
	license = 'LGPLv2.1',
	keywords = 'ais opensaf pyosaf saf',
	url = 'http://sourceforge.net/projects/opensaf/',

	packages=find_packages(),
)

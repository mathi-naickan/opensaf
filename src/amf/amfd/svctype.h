/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 */

#ifndef AMF_AMFD_SVCTYPE_H_
#define AMF_AMFD_SVCTYPE_H_

#include <vector>

class AVD_SVC_TYPE {
 public:
  explicit AVD_SVC_TYPE(const std::string& dn);
  std::string name {};
  char **saAmfSvcDefActiveWeight {};
  char **saAmfSvcDefStandbyWeight {};
  std::vector<AVD_SI*> list_of_si {};
  private:
   AVD_SVC_TYPE();
   // disallow copy and assign
   AVD_SVC_TYPE(const AVD_SVC_TYPE&);
  void operator=(const AVD_SVC_TYPE&);
 };

SaAisErrorT avd_svctype_config_get(void);
void avd_svctype_add_si(AVD_SI *si);
void avd_svctype_remove_si(AVD_SI *si);
void avd_svctype_constructor(void);

extern AmfDb<std::string, AVD_SVC_TYPE> *svctype_db;

#endif  // AMF_AMFD_SVCTYPE_H_

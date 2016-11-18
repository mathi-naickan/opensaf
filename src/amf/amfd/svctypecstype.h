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

#ifndef AMF_AMFD_SVCTYPECSTYPE_H_
#define AMF_AMFD_SVCTYPECSTYPE_H_

class AVD_SVC_TYPE_CS_TYPE {
 public:
  explicit AVD_SVC_TYPE_CS_TYPE(const std::string& dn);

  std::string name {};
  SaUint32T saAmfSvctMaxNumCSIs {};

  SaUint32T curr_num_csis {};

  struct avd_amf_svc_type_tag *cs_type_on_svc_type {};
  struct avd_svc_type_cs_type_tag *cs_type_list_svc_type_next {};
 private:
  AVD_SVC_TYPE_CS_TYPE();
  // disallow copy and assign
  AVD_SVC_TYPE_CS_TYPE(const AVD_SVC_TYPE_CS_TYPE&);
  void operator=(const AVD_SVC_TYPE_CS_TYPE&);
};

SaAisErrorT avd_svctypecstypes_config_get(const std::string& svctype_name);
void avd_svctypecstypes_constructor(void);

extern AmfDb<std::string, AVD_SVC_TYPE_CS_TYPE> *svctypecstypes_db;

#endif  // AMF_AMFD_SVCTYPECSTYPE_H_

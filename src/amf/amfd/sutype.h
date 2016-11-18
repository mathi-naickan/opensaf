/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 *
 */

#ifndef AMF_AMFD_SUTYPE_H_
#define AMF_AMFD_SUTYPE_H_

#include "osaf/saf/saAis.h"
#include "amf/amfd/su.h"
#include <vector>

class AVD_SUTYPE {
 public:
  explicit AVD_SUTYPE(const std::string& dn);
  std::string name {};
  SaUint32T saAmfSutIsExternal {};
  SaUint32T saAmfSutDefSUFailover {};
  std::vector<std::string> saAmfSutProvidesSvcTypes {}; /* array of DNs, size in number_svc_types */
  unsigned int number_svc_types {};	/* size of array saAmfSutProvidesSvcTypes */
  std::vector<AVD_SU*> list_of_su {};
 private:
  AVD_SUTYPE();
  // disallow copy and assign
  AVD_SUTYPE(const AVD_SUTYPE&);
  void operator=(const AVD_SUTYPE&);
};
extern AmfDb<std::string, AVD_SUTYPE> *sutype_db;

/**
 * Get SaAmfSUType from IMM and create internal objects
 * 
 * @return SaAisErrorT
 */
extern SaAisErrorT avd_sutype_config_get(void);

/**
 * Class constructor, must be called before any other function
 */
extern void avd_sutype_constructor(void);

/**
 * Add SU to SU Type internal list
 * @param su
 */
extern void avd_sutype_add_su(AVD_SU* su);

/**
 * Remove SU from SU Type internal list
 * @param su
 */
extern void avd_sutype_remove_su(AVD_SU* su);

#endif  // AMF_AMFD_SUTYPE_H_

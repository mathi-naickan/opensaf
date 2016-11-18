/*      -*- OpenSAF  -*-            
 *
 * (C) Copyright 2008-2014 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power, Ericsson
 *
 */

/*****************************************************************************

  DESCRIPTION: Service Group Type class
  
******************************************************************************
*/

#ifndef AMF_AMFD_SGTYPE_H_
#define AMF_AMFD_SGTYPE_H_

#include "amf/saf/saAmf.h"
#include "amf/amf_db_template.h"
#include <vector>

class AVD_SG;

class AVD_AMF_SG_TYPE {
 public:
  explicit AVD_AMF_SG_TYPE(const std::string& dn);
  std::string name {};
  bool saAmfSgtDefAutoRepair_configured {}; /* True when user configures saAmfSGDefAutoRepair else false */
   /******************** B.04 model *************************************************/
  std::vector<std::string> saAmfSGtValidSuTypes {};	/* array of DNs, size in number_su_type */
  SaAmfRedundancyModelT saAmfSgtRedundancyModel {};
  SaBoolT saAmfSgtDefAutoRepair {};
  SaBoolT saAmfSgtDefAutoAdjust {};
  SaTimeT saAmfSgtDefAutoAdjustProb {};
  SaTimeT saAmfSgtDefCompRestartProb {};
  SaUint32T saAmfSgtDefCompRestartMax {};
  SaTimeT saAmfSgtDefSuRestartProb {};
  SaUint32T saAmfSgtDefSuRestartMax {};
   /******************** B.04 model *************************************************/

  uint32_t number_su_type {};	/* size of array saAmfSGtValidSuTypes */
  std::vector<AVD_SG*> list_of_sg {};
 private:
  AVD_AMF_SG_TYPE();
  void initialize();
  // disallow copy and assign
  AVD_AMF_SG_TYPE(const AVD_AMF_SG_TYPE&);
  void operator=(const AVD_AMF_SG_TYPE&);

};

extern AmfDb<std::string, AVD_AMF_SG_TYPE> *sgtype_db;
SaAisErrorT avd_sgtype_config_get(void);
AVD_AMF_SG_TYPE *avd_sgtype_get(const std::string& dn);
void avd_sgtype_add_sg(AVD_SG *sg);
void avd_sgtype_remove_sg(AVD_SG *sg);
void avd_sgtype_constructor(void);

#endif  // AMF_AMFD_SGTYPE_H_

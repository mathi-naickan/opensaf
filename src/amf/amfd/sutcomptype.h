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
 * Author(s): Ericsson
 *
 */

/*****************************************************************************

  DESCRIPTION: SuTypeCompType class definition
  
******************************************************************************
*/

#ifndef AMF_AMFD_SUTCOMPTYPE_H_
#define AMF_AMFD_SUTCOMPTYPE_H_

#include "amf/saf/saAmf.h"
#include "amf/amfd/sutype.h"

typedef struct {
	std::string name;
	SaUint32T saAmfSutMaxNumComponents;
	SaUint32T saAmfSutMinNumComponents;
	SaUint32T curr_num_components;
} AVD_SUTCOMP_TYPE;
extern AmfDb<std::string, AVD_SUTCOMP_TYPE> *sutcomptype_db;

SaAisErrorT avd_sutcomptype_config_get(const std::string& sutype_name, AVD_SUTYPE *sut);
void avd_sutcomptype_constructor(void);

#endif  // AMF_AMFD_SUTCOMPTYPE_H_

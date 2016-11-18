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
/****************************************************************************

  DESCRIPTION:

  This module is the include file for Sa Amf Application object management.
  
****************************************************************************/
#ifndef AMF_AMFD_APP_H_
#define AMF_AMFD_APP_H_

#include "amf/saf/saAmf.h"
#include "imm/saf/saImm.h"

#include "amf/amfd/apptype.h"
#include "amf/amfd/sg.h"
#include "amf/amfd/si.h"
#include "amf/amf_db_template.h"

class AVD_APP {

public:
	std::string name;
	std::string saAmfAppType;
	SaAmfAdminStateT saAmfApplicationAdminState;
	SaUint32T saAmfApplicationCurrNumSGs;
	AVD_SG *list_of_sg;
	AVD_SI *list_of_si;
	AVD_APP *app_type_list_app_next;
	AVD_APP_TYPE *app_type;
	
	AVD_APP();
	explicit AVD_APP(const std::string& dn);
	~AVD_APP();
	
	void add_si(AVD_SI *si);
	void remove_si(AVD_SI *si);
	void add_sg(AVD_SG *sg);
	void remove_sg(AVD_SG *sg);

private:
	AVD_APP(const AVD_APP&);
	AVD_APP& operator=(const AVD_APP&);
};

extern AmfDb<std::string, AVD_APP> *app_db;

extern SaAisErrorT avd_app_config_get(void);
extern void avd_app_constructor(void);

#endif  // AMF_AMFD_APP_H_

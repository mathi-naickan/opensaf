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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This file contains healthcheck database related definitions.
  
******************************************************************************
*/

#ifndef AMF_AMFND_AVND_HC_H_
#define AMF_AMFND_AVND_HC_H_
#include <string>

typedef AVSV_HLT_INFO_MSG AVND_HC_PARAM;

typedef struct avnd_hc_tag {
	AVSV_HLT_KEY key;       /* TODO remove */
	std::string name;           /* index */
	SaTimeT period;		/* periodicity value */
	SaTimeT max_dur;	/* max duration value */
	bool is_ext;	/* Whether it is for ext comp */
	bool rcvd_on_fover;	/* Temporary flag to find out whether
				 * update is received in the f-over message*/
} AVND_HC;

typedef struct avnd_hctype_tag {
	std::string name;		/* index */
	SaTimeT saAmfHctDefPeriod;	/* periodicity value */
	SaTimeT saAmfHctDefMaxDuration;	/* max duration value */
} AVND_HCTYPE;

/* Extern function declarations */
extern AVND_HC *avnd_hcdb_rec_get(struct avnd_cb_tag *cb, AVSV_HLT_KEY *hc_key);
extern void avnd_hcdb_init(struct avnd_cb_tag *);
extern AVND_HC *avnd_hcdb_rec_add(struct avnd_cb_tag *, AVND_HC_PARAM *, uint32_t *);
extern uint32_t avnd_hcdb_rec_del(struct avnd_cb_tag *, AVSV_HLT_KEY *);
extern SaAisErrorT avnd_hc_config_get(struct avnd_comp_tag *comp);
extern SaAisErrorT avnd_hctype_config_get(SaImmHandleT immOmHandle, const std::string& comptype_dn);
extern AVND_HCTYPE *avnd_hctypedb_rec_get(struct avnd_cb_tag *, const std::string& comp_type_dn, const SaAmfHealthcheckKeyT *key);
extern uint32_t avnd_hc_oper_req(struct avnd_cb_tag *, AVSV_PARAM_INFO *param);
extern uint32_t avnd_hctype_oper_req(struct avnd_cb_tag *, AVSV_PARAM_INFO *param);

#endif  // AMF_AMFND_AVND_HC_H_

/*      OpenSAF
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
 * Author(s): Ericsson AB
 *
 */

#ifndef __SMFD_H
#define __SMFD_H

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <ncsgl_defs.h>
#include <ncs_lib.h>
#include <ncs_util.h>
#include <mds_papi.h>
#include <ncs_mda_pvt.h>
#include <logtrace.h>

/* SMF files */
#include "smfd_cb.h"
#include "smfsv_evt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */
#define DEFAULT_MAX_DN_LENGTH 255
#define IMM_CONFIG_OBJECT_DN "opensafImm=opensafImm,safApp=safImmService"
#define IMM_LONG_DN_CONFIG_ATTRIBUTE_NAME "longDnsAllowed"
#define SMF_SAF_APP_DN "safApp=safSmfService"
#define SMF_CAMP_RESTART_INDICATOR_RDN "smfCampaignRestartIndicator=smf"

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

#define ONE_SECOND 1

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
	extern smfd_cb_t *smfd_cb;
	extern const SaNameT *smfApplDN;
	extern uint32_t smfd_amf_init(smfd_cb_t *);
	extern uint32_t smfd_mds_init(smfd_cb_t *);
	extern uint32_t smfd_mds_finalize(smfd_cb_t *);
	extern uint32_t smfd_mds_change_role(smfd_cb_t *);
	extern uint32_t smfd_mds_msg_send(smfd_cb_t * cb,
				       SMFSV_EVT * evt,
				       MDS_DEST * dest,
				       MDS_SYNC_SND_CTXT * mds_ctxt,
				       MDS_SEND_PRIORITY_TYPE prio);

	uint32_t campaign_oi_activate(smfd_cb_t * cb);
	uint32_t campaign_oi_deactivate(smfd_cb_t * cb);
	uint32_t campaign_oi_init(smfd_cb_t * cb);
	uint32_t read_config_and_set_control_block(smfd_cb_t * cb);
	extern  void smfd_coi_reinit_bg(smfd_cb_t *cb); 
	uint32_t updateImmAttr(const char *dn,
			    SaImmAttrNameT attributeName,
			    SaImmValueTypeT attrValueType, void *value);

	void smfd_cb_lock();
	void smfd_cb_unlock();

#ifdef __cplusplus
}
#endif
#endif				/* ifndef __SMFD_H */

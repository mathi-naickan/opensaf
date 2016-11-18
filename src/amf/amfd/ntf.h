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
  MODULE NAME: AVD_NTF.H

..............................................................................

  DESCRIPTION:   This file contains avd ntfs related definitions.

******************************************************************************/

#ifndef AMF_AMFD_NTF_H_
#define AMF_AMFD_NTF_H_

#include "amf/amfd/comp.h"
#include "amf/amfd/susi.h"

#include "imm.h"
#define AMF_NTF_SENDER "safApp=safAmfService"

/* All states like oper, readiness etc starts from 1, so defining not applicable values */
#define STATE_ID_NA  0
#define OLD_STATE_NA 0
#define NEW_STATE_NA 0

/*For nodegroup admin state notification*/
typedef enum {
	SA_AMF_NTFID_NG_ADMIN_STATE = 0x072
} SaAmfExtraNotificationMinorIdT;

/* Alarms */
void avd_send_comp_inst_failed_alarm(const std::string& comp_name, const std::string& node_name);
void avd_send_comp_clean_failed_alarm(const std::string& comp_name, const std::string& node_name);
void avd_send_cluster_reset_alarm(const std::string& comp_name);
void avd_send_si_unassigned_alarm(const std::string& si_name);
void avd_send_comp_proxy_status_unproxied_alarm(const std::string& comp_name);

/* Notifications */
void avd_send_admin_state_chg_ntf(const std::string& name, SaAmfNotificationMinorIdT minor_id,
					SaAmfAdminStateT old_state, SaAmfAdminStateT new_state);
void avd_send_oper_chg_ntf(const std::string& name, SaAmfNotificationMinorIdT minor_id, 
					SaAmfOperationalStateT old_state, SaAmfOperationalStateT new_state);
void avd_send_su_pres_state_chg_ntf(const std::string& su_name, SaAmfPresenceStateT old_state,
					SaAmfPresenceStateT new_state);
void avd_send_su_ha_state_chg_ntf(const std::string& su_name, const std::string& si_name,
					SaAmfHAStateT old_state, SaAmfHAStateT new_state);
void avd_send_su_ha_readiness_state_chg_ntf(const std::string& su_name, const std::string& si_name,
					SaAmfHAReadinessStateT old_state, SaAmfHAReadinessStateT new_state);
void avd_send_si_assigned_ntf(const std::string& si_name, SaAmfAssignmentStateT old_state,
					SaAmfAssignmentStateT new_state);
void avd_send_comp_proxy_status_proxied_ntf(const std::string& comp_name,
					SaAmfProxyStatusT old_state, SaAmfProxyStatusT new_state);

/* general functions */
SaAisErrorT fill_ntf_header_part(SaNtfNotificationHeaderT *notificationHeader,
				   SaNtfEventTypeT eventType,
				   const std::string& comp_name,
				   SaUint8T *add_text,
				   SaUint16T majorId,
				   SaUint16T minorId,
				   SaInt8T *avnd_name,
				   NCSCONTEXT add_info,
				   int type); /* add_info 0 --> no,  1--> node_name, 2--> si_name*/

uint32_t sendAlarmNotificationAvd(AVD_CL_CB *avd_cb,
					const std::string& comp_name,
					SaUint8T *add_text,
					SaUint16T majorId,
					SaUint16T minorId,
					uint32_t probableCause,
					uint32_t perceivedSeverity,
					NCSCONTEXT add_info,
					int type); /* add_info 0 --> no,  1--> node_name, 2--> si_name*/

uint32_t sendStateChangeNotificationAvd(AVD_CL_CB *avd_cb,
					      const std::string& comp_name,
					      SaUint8T *add_text,
					      SaUint16T majorId,
					      SaUint16T minorId,
					      uint32_t sourceIndicator,
					      SaUint16T stateId,
					      SaUint16T oldState,
					      SaUint16T newState,
					      NCSCONTEXT add_info,
					      int type); /* add_info 0 --> no,  1--> node_name, 2--> si_name*/

/* Clearing of alarms */
void avd_alarm_clear(const std::string& name, SaUint16T minorId, uint32_t probableCause);

void avd_send_error_report_ntf(const std::string& name, SaAmfRecommendedRecoveryT recovery);

extern SaAisErrorT avd_ntf_init(struct cl_cb_tag*);
extern SaAisErrorT avd_start_ntf_init_bg(void);

#endif  // AMF_AMFD_NTF_H_

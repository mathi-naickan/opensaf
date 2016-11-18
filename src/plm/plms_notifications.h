/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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


/* This file contains the prototypes for sending plms notifications */


#ifndef PLM_PLMS_NOTIFICATIONS_H_
#define PLM_PLMS_NOTIFICATIONS_H_

typedef struct SaPlmNtfStateChange {
	                SaNtfStateChangeT   state;
			struct SaPlmNtfStateChange *next;
}SaPlmNtfStateChangeT;


/************************ Function Prototypes *********************************/

SaAisErrorT plms_state_change_ntf_send(SaNtfHandleT      plm_ntf_hdl,
                                           SaNameT               *object,
                                           SaNtfIdentifierT      *ntf_id,
                                           SaUint16T             source_indicator,
                                           SaUint16T             no_of_state_changes,
                                           SaPlmNtfStateChangeT  *state_changes,
                                           SaUint16T             minor_id,
					   SaUint16T   no_of_corr_notifications,
					   SaNtfIdentifierT     *corr_ids,
                                           SaNameT               *dn_name /* DN of root state change PLM entity obj */
                                           );

SaAisErrorT plms_hpi_evt_ntf_send(SaNtfHandleT      plm_ntf_hdl,
                                        SaNameT           *object,
                                        SaUint32T         event_type,
                                        SaInt8T           *entity_path,
                                        SaUint32T         domain_id,
                                        SaHpiEventT       *hpi_event,
					SaUint16T      no_of_corr_notifications,
					SaNtfIdentifierT *corr_ids,
                                        SaNtfIdentifierT  *ntf_id
                                        );

SaAisErrorT plms_alarm_ntf_send(SaNtfHandleT  plm_ntf_hdl,
                                     SaNameT       *object,
                                     SaUint32T     event_type,
                                      SaInt8T      *entity_path,
                                     SaUint32T     severity,
                                     SaUint32T     cause,
                                     SaUint32T     minor_id,
				     SaUint16T         no_of_corr_notifications,
				     SaNtfIdentifierT *corr_ids,
                                     SaNtfIdentifierT *ntf_id
                                     );

#endif  // PLM_PLMS_NOTIFICATIONS_H_

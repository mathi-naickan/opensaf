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

/*****************************************************************************

  DESCRIPTION:
  This include file contains Control block and entity related data structures

******************************************************************************/
#ifndef PLMS_HSM_H
#define PLMS_HSM_H

#include <SaHpi.h>
#include <saNtf.h>

#define PLMS_MAX_HPI_SESSION_OPEN_RETRIES 30

#define PLMS_HSM_TASK_PRIORITY   5
#define PLMS_HSM_STACKSIZE       NCS_STACKSIZE_HUGEX2 


/* HSM Control block contains the following fields */
typedef struct
{
	SaHpiDomainIdT    domain_id;
	SaHpiSessionIdT   session_id;
	pthread_t         threadid;
	SaUint32T	  session_valid;
	SaUint8T          get_idr_with_event_type;
	SaHpiTimeT	  extr_pending_timeout;
	SaHpiTimeT        insert_pending_timeout;
	SaNtfHandleT 	  plm_ntf_hdl;
}PLMS_HSM_CB;


EXTERN_C PLMS_HSM_CB *hsm_cb;

EXTERN_C HSM_HA_STATE hsm_ha_state;

/* Function Declarations */
EXTERN_C SaUint32T plms_hsm_initialize(PLMS_HPI_CONFIG *hpi_cfg);
EXTERN_C SaUint32T plms_hsm_finalize(void);
EXTERN_C SaUint32T hsm_get_idr_info(SaHpiRptEntryT  *rpt_entry,
                                PLMS_INV_DATA  *inv_data);

EXTERN_C SaUint32T convert_entitypath_to_string(SaHpiEntityPathT *entity_path,
                                        SaInt8T **ent_path_str);
EXTERN_C SaUint32T plms_hsm_session_close();
#endif   /* PLMS_HSM_H */


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
 * Author(s): Emerson network power 
 *
 */


#ifndef PLM_PLMS_MBCSV_H_
#define PLM_PLMS_MBCSV_H_

#include "plms.h"
#include "mbc/mbcsv_papi.h"

#define MAX_NO_OF_TRK_STEP_INFO_RECS 5
#define MAX_NO_OF_ENTITY_GRP_INFO_RECS 5
#define MAX_NO_OF_CLIENT_INFO_RECS     10

#define m_PLMS_COPY_TRACK_STEP_INFO(dptr,sptr) { \
	dptr->dn_name.length = sptr->dn_name.length; \
	strcpy((SaInt8T *)dptr->dn_name.value,(SaInt8T *)sptr->dn_name.value); \
	dptr->opr_type = sptr->opr_type; \
	dptr->change_step = sptr->change_step; \
	dptr->root_correlation_id = sptr->root_correlation_id;\
}
#if 0
#define m_PLMS_CPY_ENTITY_GROUP_INFO(dptr,sptr) {\
	dptr->entity_group_handle = sptr->entity_grp_hdl; \
	dptr->agent_mdest_id = sptr->agent_mdest_id; \
	tmp_entity_ptr = sptr->plm_entity_list; \
	/* first find no of entities in group */ \
	while(tmp_entity_ptr) \
	{ \
		num_entities++; \
		tmp_entity_ptr = tmp_entity_ptr->next; \
	} \
	dptr->number_of_entities = num_entities; \
	/* Now fill the entity list */ \
	tmp_entity_ptr = sptr->plm_entity_list; \
	while(tmp_entity_ptr) \
	{ \
		tmp_ckpt_ent_list_ptr = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST)); \
		strcpy((SaInt8T *)tmp_ckpt_ent_list_ptr->entity_name.value, (SaInt8T *)tmp_entity_ptr->plm_entity->dn_name.value); \
		if (prev_ckpt_list_ptr == NULL) \
		{ \
			dptr->entity_list = tmp_ckpt_ent_list_ptr; \
		} else { \
			prev_ckpt_list_ptr->next = tmp_ckpt_ent_list_ptr; \
		} \
		prev_ckpt_list_ptr = tmp_ckpt_ent_list_ptr; \
		tmp_entity_ptr = tmp_entity_ptr->next; \
	} \
	dptr->options = sptr->options; \
	dptr->track_flags = sptr->track_flags; \
	dptr->track_cookie = sptr->track_cookie; \
}
#endif

typedef enum plms_mbcsv_msg_type {
        PLMS_A2S_MSG_BASE = 1,
        PLMS_A2S_MSG_TRK_STP_INFO = PLMS_A2S_MSG_BASE,
        PLMS_A2S_MSG_ENT_GRP_INFO,
        PLMS_A2S_MSG_CLIENT_INFO,
        PLMS_A2S_MSG_CLIENT_DOWN_INFO,
        PLMS_A2S_MSG_MAX_EVT
} PLMS_MBCSV_MSG_TYPE;

typedef struct plms_mbcsv_header {
	/* SaVersionT          version;  Active peer's Version */
	PLMS_MBCSV_MSG_TYPE    msg_type; /* Type of mbcsv data */
	uint32_t num_records; /* =1 for async updates,>=1 for cold/warm sync **/
	uint32_t data_len;         /* Total length of encoded data */
	/* uint16_t               checksum;  Checksum calculated on the message */
} PLMS_MBCSV_HEADER;

#if 0
typedef  struct plms_ckpt_info
{
        SaNameT             dn_name;       *key field *
        SaPlmChangeStepT    track_step;    *validate/start/completed*
} PLMS_CKPT_TRACK_STEP_INFO;
#endif
		

typedef struct plms_mbcsv_msg {
        PLMS_MBCSV_HEADER  header;
        union {
                /* Messages for replication to PLMS stby  */
		PLMS_CKPT_TRACK_STEP_INFO step_info;
                PLMS_CKPT_ENTITY_GROUP_INFO ent_grp_info;
		PLMS_CKPT_CLIENT_INFO_LIST client_info;
        } info;
} PLMS_MBCSV_MSG;

/****************** function prototypes *****************/
SaUint32T plms_edp_client_info_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,
EDU_ERR *o_err);
SaUint32T plms_mbcsv_register();
SaUint32T plms_mbcsv_finalize();
SaUint32T plms_mbcsv_chgrole();
SaUint32T plms_mbcsv_dispatch();
SaUint32T plms_mbcsv_send_async_update(PLMS_MBCSV_MSG *msg, 
						uint32_t action);
#endif  // PLM_PLMS_MBCSV_H_


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
  
  AvND-MDS interaction related definitions.
    
******************************************************************************
*/

#ifndef AVND_MDS_H
#define AVND_MDS_H

/* In Service upgrade support */
#define AVND_MDS_SUB_PART_VERSION   5

#define AVND_AVD_SUBPART_VER_MIN   1
#define AVND_AVD_SUBPART_VER_MAX   5

#define AVND_AVND_SUBPART_VER_MIN   1
#define AVND_AVND_SUBPART_VER_MAX   1

#define AVND_AVA_SUBPART_VER_MIN   1
#define AVND_AVA_SUBPART_VER_MAX   1

#define AVND_CLA_SUBPART_VER_MIN   1
#define AVND_CLA_SUBPART_VER_MAX   1

/* timer type enums */
typedef enum avnd_msg_type {
	AVND_MSG_AVD = 1,
	AVND_MSG_AVA,
	AVND_MSG_AVND,
	AVND_MSG_MAX
} AVND_MSG_TYPE;

typedef struct avnd_msg {
	AVND_MSG_TYPE type;
	union {
		AVSV_DND_MSG *avd;	/* AvD message */
		AVSV_ND2ND_AVND_MSG *avnd;	/* AvND message */
		AVSV_NDA_AVA_MSG *ava;	/* AvA message */
	} info;
} AVND_MSG;

typedef struct avnd_dnd_msg_list_tag {
	AVND_MSG msg;
	AVND_TMR resp_tmr;
	uint32_t opq_hdl;
	struct avnd_dnd_msg_list_tag *next;
} AVND_DND_MSG_LIST;

typedef struct avnd_dnd_list_tag {
	AVND_DND_MSG_LIST *head;
	AVND_DND_MSG_LIST *tail;
} AVND_DND_LIST;

/*****************************************************************************
                 Macros to fill the MDS message structure
*****************************************************************************/

/* Macro to populate the AMF API response message */
#define m_AVND_AMF_API_RSP_MSG_FILL(m, api, amf_rc) \
{ \
   (m).type = AVND_MSG_AVA; \
   (m).info.ava->type = AVSV_AVND_AMF_API_RESP_MSG; \
   (m).info.ava->info.api_resp_info.type = (api); \
   (m).info.ava->info.api_resp_info.rc = (amf_rc); \
}

/* Macro to populate the 'AMF HA State Get' response message */
#define m_AVND_AMF_HA_STATE_GET_RSP_MSG_FILL(m, has, amf_rc) \
{ \
   (m).type = AVND_MSG_AVA; \
   (m).info.ava->type = AVSV_AVND_AMF_API_RESP_MSG; \
   (m).info.ava->info.api_resp_info.type = AVSV_AMF_HA_STATE_GET; \
   (m).info.ava->info.api_resp_info.rc = (amf_rc); \
   (m).info.ava->info.api_resp_info.param.ha_get.ha = static_cast<SaAmfHAStateT>((has)); \
}

/* Macro to populate the 'component register' message */
#define m_AVND_COMP_REG_MSG_FILL(m, dst, hd, cn, pcn) \
{ \
   (m).type = AVSV_AMF_COMP_REG; \
   (m).dest = (dst); \
   (m).param.reg.hdl = (hd); \
   memcpy((m).param.reg.comp_name.value, \
                   (cn)->value, (cn)->length); \
   (m).param.reg.comp_name.length = ((cn)->length); \
   memcpy((m).param.reg.proxy_comp_name.value, \
                   (pcn)->value, (pcn)->length); \
   (m).param.reg.proxy_comp_name.length = ((pcn)->length); \
}

/* Macro to populate the 'component unregister' message */
#define m_AVND_COMP_UNREG_MSG_FILL(m, dst, hd, cn, pcn) \
{ \
   (m).type = AVSV_AMF_COMP_UNREG; \
   (m).dest = (dst); \
   (m).param.unreg.hdl = (hd); \
   memcpy((m).param.unreg.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).param.unreg.comp_name.length = (cn).length; \
   memcpy((m).param.unreg.proxy_comp_name.value, \
                   (pcn).value, (pcn).length); \
   (m).param.unreg.proxy_comp_name.length = (pcn).length; \
}

/*** Extern function declarations ***/

struct avnd_cb_tag;

uint32_t avnd_mds_reg(struct avnd_cb_tag *);

uint32_t avnd_mds_cbk(NCSMDS_CALLBACK_INFO *info);

uint32_t avnd_mds_send(struct avnd_cb_tag *, AVND_MSG *, MDS_DEST *, MDS_SYNC_SND_CTXT *);

uint32_t avnd_mds_red_send(struct avnd_cb_tag *, AVND_MSG *, MDS_DEST *, MDS_DEST *);
uint32_t avnd_avnd_mds_send(struct avnd_cb_tag *, MDS_DEST, AVND_MSG *);
uint32_t avnd_mds_vdest_reg(struct avnd_cb_tag *cb);
uint32_t avnd_mds_set_vdest_role(struct avnd_cb_tag *cb, SaAmfHAStateT role);

#endif   /* !AVND_MDS_H */

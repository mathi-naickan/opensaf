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

  This file contains declarations fro AvD interface.
  
******************************************************************************
*/

#ifndef AVND_DI_H
#define AVND_DI_H

/* macro to find the matching record (based on the msg-id) */
/* 
 * Caution!!! It is assumed that the msg-id is the 1st element in the message
 * structure. Ensure it. Else move the msg id to the common portion of the 
 * message structure (outside the msg type specific contents).
 */
#define m_AVND_DIQ_REC_FIND(cb, mid, o_rec) \
{ \
   AVND_DND_LIST *list = &((cb)->dnd_list); \
   for ((o_rec) = list->head; \
        (o_rec) && !(*((uint32_t *)(&((o_rec)->msg.info.avd->msg_info)))== (mid)); \
        (o_rec) = (o_rec)->next); \
}

/* macro to find & pop a given record */
#define m_AVND_DIQ_REC_FIND_POP(cb, rec) \
{ \
   AVND_DND_LIST *list = &((cb)->dnd_list); \
   AVND_DND_MSG_LIST *prv = list->head, *curr; \
   for (curr = list->head; \
        curr && !(curr == (rec)); \
        prv = curr, curr = curr->next); \
   if (curr) \
   { \
      if ( curr == list->head ) \
      { \
         list->head = curr->next; \
         if ( list->tail == curr ) list->tail = list->head; \
      } else { \
         prv->next = curr->next; \
         if ( list->tail == curr ) list->tail = prv; \
      } \
      curr->next = 0; \
   } \
}

struct avnd_cb_tag;

uint32_t avnd_di_oper_send(struct avnd_cb_tag *, const AVND_SU *, uint32_t);
uint32_t avnd_di_susi_resp_send(struct avnd_cb_tag *, AVND_SU *, AVND_SU_SI_REC *);
uint32_t avnd_di_object_upd_send(struct avnd_cb_tag *, AVSV_PARAM_INFO *);
uint32_t avnd_di_pg_act_send(struct avnd_cb_tag *, SaNameT *, AVSV_PG_TRACK_ACT, bool);
uint32_t avnd_di_msg_send(struct avnd_cb_tag *, AVND_MSG *);
void avnd_di_msg_ack_process(struct avnd_cb_tag *, uint32_t);
void avnd_diq_del(struct avnd_cb_tag *);
AVND_DND_MSG_LIST *avnd_diq_rec_add(struct avnd_cb_tag *cb, AVND_MSG *msg);
void avnd_diq_rec_del(struct avnd_cb_tag *cb, AVND_DND_MSG_LIST *rec);
uint32_t avnd_diq_rec_send(struct avnd_cb_tag *cb, AVND_DND_MSG_LIST *rec);
uint32_t avnd_di_reg_su_rsp_snd(struct avnd_cb_tag *cb, SaNameT *su_name, uint32_t ret_code);
uint32_t avnd_di_ack_nack_msg_send(struct avnd_cb_tag *cb, uint32_t rcv_id, uint32_t view_num);
extern void avnd_di_uns32_upd_send(int class_id, int attr_id, const SaNameT *dn, uint32_t value);

#endif   /* !AVND_OPER_H */

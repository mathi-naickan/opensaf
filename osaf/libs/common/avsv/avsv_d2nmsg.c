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

  This file contains utility routines for managing the messages between AvD & 
  AvND.
  
******************************************************************************
*/

#include "avsv.h"
#include "avsv_d2nmsg.h"

/*****************************************************************************
 * Function: free_d2n_su_msg_info
 *
 * Purpose:  This function frees the d2n SU message contents.
 *
 * Input: su_msg - Pointer to the SU message contents to be freed.
 *
 * Returns: None
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static void free_d2n_su_msg_info(AVSV_DND_MSG *su_msg)
{
	AVSV_SU_INFO_MSG *su_info;

	while (su_msg->msg_info.d2n_reg_su.su_list != NULL) {
		su_info = su_msg->msg_info.d2n_reg_su.su_list;
		su_msg->msg_info.d2n_reg_su.su_list = su_info->next;
		free(su_info);
	}
}

/*****************************************************************************
 * Function: cpy_d2n_su_msg
 *
 * Purpose:  This function makes a copy of the d2n SU message.
 *
 * Input: d_su_msg - Pointer to the SU message to be copied to.
 *        s_su_msg - Pointer to the SU message to be copied.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static uint32_t cpy_d2n_su_msg(AVSV_DND_MSG *d_su_msg, AVSV_DND_MSG *s_su_msg)
{
	AVSV_SU_INFO_MSG *s_su_info, *d_su_info;

	memset(d_su_msg, '\0', sizeof(AVSV_DND_MSG));

	memcpy(d_su_msg, s_su_msg, sizeof(AVSV_DND_MSG));
	d_su_msg->msg_info.d2n_reg_su.su_list = NULL;

	s_su_info = s_su_msg->msg_info.d2n_reg_su.su_list;

	while (s_su_info != NULL) {
		d_su_info = malloc(sizeof(AVSV_SU_INFO_MSG));
		if (d_su_info == NULL) {
			free_d2n_su_msg_info(d_su_msg);
			return NCSCC_RC_FAILURE;
		}

		memcpy(d_su_info, s_su_info, sizeof(AVSV_SU_INFO_MSG));
		d_su_info->next = d_su_msg->msg_info.d2n_reg_su.su_list;
		d_su_msg->msg_info.d2n_reg_su.su_list = d_su_info;

		/* now go to the next su info in source */
		s_su_info = s_su_info->next;
	}

	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
 * Function: free_d2n_susi_msg_info
 *
 * Purpose:  This function frees the d2n SU SI message contents.
 *
 * Input: susi_msg - Pointer to the SUSI message contents to be freed.
 *
 * Returns: none
 *
 * NOTES: It also frees the array of attributes, which are sperately
 * allocated and pointed to by AVSV_SUSI_ASGN structure.
 *
 * 
 **************************************************************************/

static void free_d2n_susi_msg_info(AVSV_DND_MSG *susi_msg)
{
	AVSV_SUSI_ASGN *compcsi_info;

	while (susi_msg->msg_info.d2n_su_si_assign.list != NULL) {
		compcsi_info = susi_msg->msg_info.d2n_su_si_assign.list;
		susi_msg->msg_info.d2n_su_si_assign.list = compcsi_info->next;
		if (compcsi_info->attrs.list != NULL) {
			free(compcsi_info->attrs.list);
			compcsi_info->attrs.list = NULL;
		}
		free(compcsi_info);
	}
}

/*****************************************************************************
 * Function: cpy_d2n_susi_msg
 *
 * Purpose:  This function makes a copy of the d2n SU SI message contents.
 *
 * Input: d_susi_msg - Pointer to the SU SI message to be copied to.
 *        s_susi_msg - Pointer to the SU SI message to be copied.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: It also allocates and copies the array of attributes, which are sperately
 * allocated and pointed to by AVSV_SUSI_ASGN structure.
 * 
 **************************************************************************/

static uint32_t cpy_d2n_susi_msg(AVSV_DND_MSG *d_susi_msg, AVSV_DND_MSG *s_susi_msg)
{
	AVSV_SUSI_ASGN *s_compcsi_info, *d_compcsi_info;

	memset(d_susi_msg, '\0', sizeof(AVSV_DND_MSG));

	memcpy(d_susi_msg, s_susi_msg, sizeof(AVSV_DND_MSG));
	d_susi_msg->msg_info.d2n_su_si_assign.list = NULL;

	s_compcsi_info = s_susi_msg->msg_info.d2n_su_si_assign.list;

	while (s_compcsi_info != NULL) {
		d_compcsi_info = malloc(sizeof(AVSV_SUSI_ASGN));
		if (d_compcsi_info == NULL) {
			free_d2n_susi_msg_info(d_susi_msg);
			return NCSCC_RC_FAILURE;
		}

		memcpy(d_compcsi_info, s_compcsi_info, sizeof(AVSV_SUSI_ASGN));

		if ((s_compcsi_info->attrs.list != NULL) && (s_compcsi_info->attrs.number > 0)) {
			d_compcsi_info->attrs.list =
				malloc(s_compcsi_info->attrs.number * sizeof(*d_compcsi_info->attrs.list));
			if (d_compcsi_info->attrs.list == NULL) {
				free_d2n_susi_msg_info(d_susi_msg);
				return NCSCC_RC_FAILURE;
			}
			memcpy(d_compcsi_info->attrs.list, s_compcsi_info->attrs.list,
			       (s_compcsi_info->attrs.number * sizeof(*d_compcsi_info->attrs.list)));
		}
		d_compcsi_info->next = d_susi_msg->msg_info.d2n_su_si_assign.list;
		d_susi_msg->msg_info.d2n_su_si_assign.list = d_compcsi_info;

		/* now go to the next su info in source */
		s_compcsi_info = s_compcsi_info->next;
	}

	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
 * Function: free_d2n_pg_msg_info
 *
 * Purpose:  This function frees the d2n PG track response message contents.
 *
 * Input: pg_msg - Pointer to the PG message contents to be freed.
 *
 * Returns: None
 *
 * NOTES: None
 *
 * 
 **************************************************************************/

static void free_d2n_pg_msg_info(AVSV_DND_MSG *pg_msg)
{
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info = &pg_msg->msg_info.d2n_pg_track_act_rsp;

	if (info->mem_list.numberOfItems)
		free(info->mem_list.notification);

	info->mem_list.notification = 0;
	info->mem_list.numberOfItems = 0;
}

/*****************************************************************************
 * Function: cpy_d2n_pg_msg
 *
 * Purpose:  This function makes a copy of the d2n PG track response message 
 *           contents.
 *
 * Input: d_pg_msg - Pointer to the PG message to be copied to.
 *        s_pg_msg - Pointer to the PG message to be copied.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: It also allocates and copies the array of attributes, which are sperately
 * allocated and pointed to by AVSV_SUSI_ASGN structure.
 * 
 **************************************************************************/

static uint32_t cpy_d2n_pg_msg(AVSV_DND_MSG *d_pg_msg, AVSV_DND_MSG *s_pg_msg)
{
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *d_info = &d_pg_msg->msg_info.d2n_pg_track_act_rsp;
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *s_info = &s_pg_msg->msg_info.d2n_pg_track_act_rsp;

	memset(d_pg_msg, '\0', sizeof(AVSV_DND_MSG));

	/* copy the common contents */
	memcpy(d_pg_msg, s_pg_msg, sizeof(AVSV_DND_MSG));

	if (!s_info->mem_list.numberOfItems)
		return NCSCC_RC_SUCCESS;

	/* alloc the memory for the notify buffer */
	d_info->mem_list.notification = 0;
	d_info->mem_list.notification = (SaAmfProtectionGroupNotificationT *)
	    malloc(sizeof(SaAmfProtectionGroupNotificationT) * s_info->mem_list.numberOfItems);
	if (!d_info->mem_list.notification)
		return NCSCC_RC_FAILURE;

	/* copy the mem-list */
	memcpy(d_info->mem_list.notification, s_info->mem_list.notification,
	       sizeof(SaAmfProtectionGroupNotificationT) * s_info->mem_list.numberOfItems);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avsv_dnd_msg_free
 
  Description   : This routine frees the Message structures used for
                  communication between AvD and AvND. 
 
  Arguments     : msg - ptr to the DND message that needs to be freed.
 
  Return Values : None
 
  Notes         : For : AVSV_D2N_REG_SU_MSG, AVSV_D2N_INFO_SU_SI_ASSIGN_MSG
                  and AVSV_D2N_PG_TRACK_ACT_RSP_MSG, this procedure calls the
                  corresponding information free function to free the
                  list information in them before freeing the message.
******************************************************************************/
void avsv_dnd_msg_free(AVSV_DND_MSG *msg)
{
	if (msg == NULL)
		return;

	/* these messages have information list in them free them
	 * first by calling the corresponding free routine.
	 */
	switch (msg->msg_type) {
	case AVSV_D2N_REG_SU_MSG:
		free_d2n_su_msg_info(msg);
		break;
	case AVSV_D2N_INFO_SU_SI_ASSIGN_MSG:
		free_d2n_susi_msg_info(msg);
		break;
	case AVSV_D2N_PG_TRACK_ACT_RSP_MSG:
		free_d2n_pg_msg_info(msg);
		break;
	default:
		break;
	}

	/* free the message */
	free(msg);
}

/****************************************************************************
  Name          : avsv_dnd_msg_copy
 
  Description   : This routine copies the Message structures and all its
                  contents used for communication between AvD and AvND.
 
  Arguments     : dmsg - ptr to the dest DND message
                  smsg - ptr to the src DND message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : For : AVSV_D2N_REG_SU_MSG, AVSV_D2N_INFO_SU_SI_ASSIGN_MSG
                  and AVSV_D2N_PG_TRACK_ACT_RSP_MSG, this procedure calls the
                  corresponding copy function which copies the list information
                  present in them also.
******************************************************************************/
uint32_t avsv_dnd_msg_copy(AVSV_DND_MSG *dmsg, AVSV_DND_MSG *smsg)
{
	if ((dmsg == NULL) || (smsg == NULL)) {
		return NCSCC_RC_FAILURE;
	}

	/* these messages have information list in them copy them
	 * along with copying the contents.
	 */
	switch (smsg->msg_type) {
	case AVSV_D2N_REG_SU_MSG:
		return cpy_d2n_su_msg(dmsg, smsg);
	case AVSV_D2N_INFO_SU_SI_ASSIGN_MSG:
		return cpy_d2n_susi_msg(dmsg, smsg);
	case AVSV_D2N_PG_TRACK_ACT_RSP_MSG:
		return cpy_d2n_pg_msg(dmsg, smsg);
	default:
		/* copy only the contents */
		memcpy(dmsg, smsg, sizeof(AVSV_DND_MSG));
		break;
	}

	return NCSCC_RC_SUCCESS;
}

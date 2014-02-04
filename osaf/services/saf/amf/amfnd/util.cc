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

  This file contains AvND utility routines. 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <configmake.h>
#include "avnd.h"

const char *presence_state[] =
{
	"OUT_OF_RANGE",
	"UNINSTANTIATED",
	"INSTANTIATING",
	"INSTANTIATED",
	"TERMINATING",
	"RESTARTING",
	"INSTANTIATION_FAILED",
	"TERMINATION_FAILED",
	"ORPHANED"
};

const char *ha_state[] = {
	"INVALID",
	"ACTIVE",
	"STANDBY",
	"QUIESCED",
	"QUIESCING"
};

/* name of file created when a comp is found in term-failed presence state */
static const char *failed_state_file_name = PKGPIDDIR "/amf_failed_state";

/****************************************************************************
  Name          : avnd_msg_content_free
 
  Description   : This routine frees the content of the AvND message.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : None
 
  Notes         : AVND_MSG structure is used as a wrapper that is never 
                  allocated. Hence it's not freed.
******************************************************************************/
void avnd_msg_content_free(AVND_CB *cb, AVND_MSG *msg)
{
	if (!msg)
		return;

	switch (msg->type) {
	case AVND_MSG_AVD:
		if (msg->info.avd) {
			dnd_msg_free(msg->info.avd);
			msg->info.avd = 0;
		}
		break;

	case AVND_MSG_AVND:
		if (msg->info.avnd) {
			nd2nd_avnd_msg_free(msg->info.avnd);
			msg->info.avnd = 0;
		}
		break;

	case AVND_MSG_AVA:
		if (msg->info.ava) {
			nda_ava_msg_free(msg->info.ava);
			msg->info.ava = 0;
		}
		break;

	default:
		break;
	}

	return;
}

/****************************************************************************
  Name          : avnd_msg_copy
 
  Description   : This routine copies the AvND message.
 
  Arguments     : cb   - ptr to the AvND control block
                  dmsg - ptr to the dest msg
                  smsg - ptr to the source msg
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avnd_msg_copy(AVND_CB *cb, AVND_MSG *dmsg, AVND_MSG *smsg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (!dmsg || !smsg) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* copy the common fields */
	memcpy(dmsg, smsg, sizeof(AVND_MSG));

	switch (smsg->type) {
	case AVND_MSG_AVD:
		dmsg->info.avd = new AVSV_DND_MSG();
		rc = avsv_dnd_msg_copy(dmsg->info.avd, smsg->info.avd);
		break;

	case AVND_MSG_AVND:
		dmsg->info.avnd = new AVSV_ND2ND_AVND_MSG();
		rc = avsv_ndnd_avnd_msg_copy(dmsg->info.avnd, smsg->info.avnd);
		break;

	case AVND_MSG_AVA:
		dmsg->info.ava = new AVSV_NDA_AVA_MSG();
		rc = avsv_nda_ava_msg_copy(dmsg->info.ava, smsg->info.ava);
		break;

	default:
		osafassert(0);
	}

 done:
	/* if failure, free the dest msg */
	if (NCSCC_RC_SUCCESS != rc && dmsg)
		avnd_msg_content_free(cb, dmsg);

	return rc;
}

/**
 * Verify that msg ID is in sequence otherwise order node reboot
 * and abort process.
 * @param rcv_msg_id
 */
void avnd_msgid_assert(uint32_t rcv_msg_id)
{
	uint32_t expected_msg_id = avnd_cb->rcv_msg_id + 1;

	if (rcv_msg_id != expected_msg_id) {
		char reason[128];
		snprintf(reason, sizeof(reason), "Message ID mismatch, rec %u, expected %u",
				 rcv_msg_id, expected_msg_id);
		opensaf_reboot(0, NULL, reason);
		abort();
	}
}

/**
 * Execute CLEANUP CLC-CLI command for component @a comp, failfast local node
 * at failure. The function is asynchronous, it does not wait for the cleanup
 * operation to completely finish. The CLEANUP script is just started/launched.
 * @param comp
 */
void avnd_comp_cleanup_launch(AVND_COMP *comp)
{
	char str[128];
	uint32_t rc;

	rc = avnd_comp_clc_fsm_run(avnd_cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to launch cleanup of '%s'", comp->name.value);
		snprintf(str, sizeof(str), "Stopping OpenSAF failed due to '%s'", comp->name.value);
		opensaf_reboot(avnd_cb->node_info.nodeId,
				(char *)avnd_cb->node_info.executionEnvironment.value, str);
		LOG_ER("exiting to aid fast reboot");
		exit(1);
	}
}

/**
 * Return true if the the failed state file exist.
 * The existence of this file means that AMF has lost control over some
 * component. A reboot or manual cleanup is needed in order to restart opensaf.
 */
bool avnd_failed_state_file_exist(void)
{
	struct stat statbuf;

	if (stat(failed_state_file_name, &statbuf) == 0) {
		return true;
	} else
		return false;
}

/**
 * Create the failed state file
 */
void avnd_failed_state_file_create(void)
{
	int fd = open(failed_state_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (fd >= 0)
		(void)close(fd);
	else
		LOG_ER("cannot create failed state file %s: %s",
				failed_state_file_name, strerror(errno));
}

/**
 * Delete the failed state file
 */
void avnd_failed_state_file_delete(void)
{
	if (unlink(failed_state_file_name) == -1)
		LOG_ER("cannot unlink failed state file %s: %s",
				failed_state_file_name, strerror(errno));
}

/**
 * Return name of failed state file
 */
const char *avnd_failed_state_file_location(void)
{
	return failed_state_file_name;
}

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
		delete su_info;
	}
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
			// AVSV_ATTR_NAME_VAL variables
			// are malloc'ed, use free()
			free(compcsi_info->attrs.list);
			compcsi_info->attrs.list = NULL;
		}
		delete compcsi_info;
	}
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

	if (info->mem_list.numberOfItems > 0)
		delete [] info->mem_list.notification;

	info->mem_list.notification = 0;
	info->mem_list.numberOfItems = 0;
}

/****************************************************************************
  Name          : dnd_msg_free
 
  Description   : This routine frees the Message structures used for
                  communication between AvD and AvND. 
 
  Arguments     : msg - ptr to the DND message that needs to be freed.
 
  Return Values : None
 
  Notes         : For : AVSV_D2N_REG_SU_MSG, AVSV_D2N_INFO_SU_SI_ASSIGN_MSG
                  and AVSV_D2N_PG_TRACK_ACT_RSP_MSG, this procedure calls the
                  corresponding information free function to free the
                  list information in them before freeing the message.
******************************************************************************/
void dnd_msg_free(AVSV_DND_MSG *msg)
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
	delete msg;
}

/****************************************************************************
  Name          : nda_ava_msg_free
 
  Description   : This routine frees the AvA message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None
 
  Notes         : None.
******************************************************************************/
void nda_ava_msg_free(AVSV_NDA_AVA_MSG *msg)
{
	if (!msg)
		return;

	/* free the message content */
	nda_ava_msg_content_free(msg);

	/* free the message */
	delete msg;

	return;
}

/****************************************************************************
  Name          : nda_ava_msg_content_free
 
  Description   : This routine frees the content of the AvA message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None.
 
  Notes         : This routine is used by AvA as it does not alloc nda 
                  message while decoding from mds.
******************************************************************************/
void nda_ava_msg_content_free(AVSV_NDA_AVA_MSG *msg)
{
	if (!msg)
		return;

	switch (msg->type) {
	case AVSV_AVA_API_MSG:
	case AVSV_AVND_AMF_API_RESP_MSG:
		break;

	case AVSV_AVND_AMF_CBK_MSG:
		if (msg->info.cbk_info) {
			amf_cbk_free(msg->info.cbk_info);
			msg->info.cbk_info = 0;
		}
		break;

	default:
		break;
	}

	return;
}

/****************************************************************************
  Name          : amf_csi_attr_list_copy
 
  Description   : This routine copies the csi attribute list.
 
  Arguments     : dattr - ptr to the destination csi-attr list
                  sattr - ptr to the src csi-attr list
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void amf_csi_attr_list_copy(SaAmfCSIAttributeListT *dattr,
	const SaAmfCSIAttributeListT *sattr)
{
	uint32_t cnt;

	if (dattr == NULL || sattr == NULL)
		goto done;

	dattr->attr = new SaAmfCSIAttributeT[sattr->number];

	for (cnt = 0; cnt < sattr->number; cnt++) {
		/* alloc memory for attr name & value */
		size_t attrNameSize = strlen((char*)sattr->attr[cnt].attrName) + 1;
		dattr->attr[cnt].attrName = new SaUint8T[attrNameSize];

		size_t attrValueSize = strlen((char*)sattr->attr[cnt].attrValue) + 1;
		dattr->attr[cnt].attrValue = new SaUint8T[attrNameSize];

		/* copy the attr name & value */
		strncpy((char*)dattr->attr[cnt].attrName,
			(char*)sattr->attr[cnt].attrName, attrNameSize);
		strncpy((char*)dattr->attr[cnt].attrValue,
			(char*)sattr->attr[cnt].attrValue, attrValueSize);

		/* increment the attr name-val pair cnt that is copied */
		dattr->number++;
	}
	
done:
	return;
}

/****************************************************************************
  Name          : amf_csi_attr_list_free
 
  Description   : This routine frees the csi attribute list.
 
  Arguments     : attr - ptr to the csi-attr list
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void amf_csi_attr_list_free(SaAmfCSIAttributeListT *attrs)
{
	uint32_t cnt;

	if (attrs == NULL)
		return;

	/* free the attr name-val pair */
	for (cnt = 0; cnt < attrs->number; cnt++) {
		delete [] attrs->attr[cnt].attrName;
		delete [] attrs->attr[cnt].attrValue;
	}			/* for */

	/* finally free the attr list ptr */
	delete [] attrs->attr;

	return;
}

/****************************************************************************
  Name          : amf_cbk_copy
 
  Description   : This routine copies the AMF callback info message.
 
  Arguments     : o_dcbk - double ptr to the dest cbk-info (o/p)
                  scbk   - ptr to the source cbk-info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t amf_cbk_copy(AVSV_AMF_CBK_INFO **o_dcbk, const AVSV_AMF_CBK_INFO *scbk)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (o_dcbk == NULL || scbk == NULL)
		return NCSCC_RC_FAILURE;

	/* allocate the dest cbk-info */
	*o_dcbk = new AVSV_AMF_CBK_INFO();

	/* copy the common fields */
	memcpy(*o_dcbk, scbk, sizeof(AVSV_AMF_CBK_INFO));

	switch (scbk->type) {
	case AVSV_AMF_HC:
	case AVSV_AMF_COMP_TERM:
	case AVSV_AMF_CSI_REM:
	case AVSV_AMF_PXIED_COMP_INST:
	case AVSV_AMF_PXIED_COMP_CLEAN:
		break;

	case AVSV_AMF_PG_TRACK:
		/* memset notify buffer */
		memset(&(*o_dcbk)->param.pg_track.buf, 0, sizeof(SaAmfProtectionGroupNotificationBufferT));

		/* copy the notify buffer, if any */
		if (scbk->param.pg_track.buf.numberOfItems > 0) {
			(*o_dcbk)->param.pg_track.buf.notification =
				new SaAmfProtectionGroupNotificationT[scbk->param.pg_track.buf.numberOfItems];
	
			for (SaUint32T i = 0; i < scbk->param.pg_track.buf.numberOfItems; ++i) {
				(*o_dcbk)->param.pg_track.buf.notification[i] =
					scbk->param.pg_track.buf.notification[i];	
			}
			(*o_dcbk)->param.pg_track.buf.numberOfItems = scbk->param.pg_track.buf.numberOfItems;
		}
		break;

	case AVSV_AMF_CSI_SET:
		/* memset avsv & amf csi attr lists */
		memset(&(*o_dcbk)->param.csi_set.attrs, 0, sizeof(AVSV_CSI_ATTRS));
		memset(&(*o_dcbk)->param.csi_set.csi_desc.csiAttr, 0, sizeof(SaAmfCSIAttributeListT));

		/* copy the avsv csi attr list */
		if (scbk->param.csi_set.attrs.number > 0) {
			(*o_dcbk)->param.csi_set.attrs.list = static_cast<AVSV_ATTR_NAME_VAL*>
				(calloc(scbk->param.csi_set.attrs.number, sizeof(AVSV_ATTR_NAME_VAL)));
			osafassert((*o_dcbk)->param.csi_set.attrs.list != NULL);

			for (uint32_t i = 0; i < scbk->param.csi_set.attrs.number; ++i) {
				(*o_dcbk)->param.csi_set.attrs.list[i] =
					scbk->param.csi_set.attrs.list[i];
			}
			(*o_dcbk)->param.csi_set.attrs.number = scbk->param.csi_set.attrs.number;
		}

		/* copy the amf csi attr list */
		if (scbk->param.csi_set.csi_desc.csiAttr.number > 0) {
			amf_csi_attr_list_copy(&(*o_dcbk)->param.csi_set.csi_desc.csiAttr,
				&scbk->param.csi_set.csi_desc.csiAttr);
		}
		break;

	default:
		osafassert(0);
	}

	return rc;
}

/****************************************************************************
  Name          : amf_cbk_free
 
  Description   : This routine frees callback information.
 
  Arguments     : cbk_info - ptr to the callback info
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void amf_cbk_free(AVSV_AMF_CBK_INFO *cbk_info)
{
	if (cbk_info == NULL)
		return;

	switch (cbk_info->type) {
	case AVSV_AMF_HC:
	case AVSV_AMF_COMP_TERM:
	case AVSV_AMF_CSI_REM:
	case AVSV_AMF_PXIED_COMP_INST:
	case AVSV_AMF_PXIED_COMP_CLEAN:
		break;

	case AVSV_AMF_PG_TRACK:
		/* free the notify buffer */
		if (cbk_info->param.pg_track.buf.numberOfItems > 0)
			delete [] cbk_info->param.pg_track.buf.notification;
		break;

	case AVSV_AMF_CSI_SET:
		/* free the avsv csi attr list */
		if (cbk_info->param.csi_set.attrs.number > 0) {
			// AVSV_ATTR_NAME_VAL variables
			// are malloc'ed, use free()
			free(cbk_info->param.csi_set.attrs.list);
		}
		
		/* free the amf csi attr list */
		amf_csi_attr_list_free(&cbk_info->param.csi_set.csi_desc.csiAttr);
		break;

	default:
		break;
	}

	/* free the cbk-info ptr */
	delete cbk_info;

	return;
}

/****************************************************************************
  Name          : nd2nd_avnd_msg_free
 
  Description   : This routine frees the AvND message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None
 
  Notes         : None.
******************************************************************************/
void nd2nd_avnd_msg_free(AVSV_ND2ND_AVND_MSG *msg)
{
	if (!msg)
		return;

	if (AVND_AVND_AVA_MSG == msg->type) {
		/* free the message content after all these are AvA content. */
		nda_ava_msg_free(msg->info.msg);
	}
	/* free the message */
	delete msg;

	return;
}
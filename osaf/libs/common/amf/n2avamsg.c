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

  This file contains utility routines for managing the messages between AvD & 
  AvA.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "amf.h"
#include "amf_amfparam.h"
#include "amf_n2avamsg.h"
#include "amf_nd2ndmsg.h"
#include "osaf_extended_name.h"

/****************************************************************************
  Name          : avsv_nda_ava_msg_free
 
  Description   : This routine frees the AvA message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None
 
  Notes         : None.
******************************************************************************/
void avsv_nda_ava_msg_free(AVSV_NDA_AVA_MSG *msg)
{
	if (!msg)
		return;

	/* free the message content */
	avsv_nda_ava_msg_content_free(msg);

	/* free the message */
	free(msg);

	return;
}

/****************************************************************************
  Name          : avsv_nd2nd_avnd_msg_free
 
  Description   : This routine frees the AvND message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None
 
  Notes         : None.
******************************************************************************/
void avsv_nd2nd_avnd_msg_free(AVSV_ND2ND_AVND_MSG *msg)
{
	if (!msg)
		return;

	if (osaf_is_an_extended_name(&msg->comp_name)) {
		osaf_extended_name_free(&msg->comp_name);
	}

	if (AVND_AVND_AVA_MSG == msg->type) {
		/* free the message content after all these are AvA content. */
		avsv_nda_ava_msg_free(msg->info.msg);
	} else if (AVND_AVND_CBK_DEL == msg->type) {
		if (osaf_is_an_extended_name(&msg->info.cbk_del.comp_name)) {
			osaf_extended_name_free(&msg->info.cbk_del.comp_name);
		}
	}

	/* free the message */
	free(msg);

	return;
}

/****************************************************************************
  Name          : avsv_nda_ava_msg_content_free
 
  Description   : This routine frees the content of the AvA message.
 
  Arguments     : msg - ptr to the AvA msg
 
  Return Values : None.
 
  Notes         : This routine is used by AvA as it does not alloc nda 
                  message while decoding from mds.
******************************************************************************/
void avsv_nda_ava_msg_content_free(AVSV_NDA_AVA_MSG *msg)
{
	if (!msg)
		return;

	switch (msg->type) {
	case AVSV_AVA_API_MSG:
		avsv_amf_api_free(&msg->info.api_info);
		break;

	case AVSV_AVND_AMF_CBK_MSG:
		if (msg->info.cbk_info) {
			avsv_amf_cbk_free(msg->info.cbk_info);
			msg->info.cbk_info = 0;
		}
		break;

	case AVSV_AVND_AMF_API_RESP_MSG:
		if (msg->info.api_resp_info.type == AVSV_AMF_HA_STATE_GET) {
			osaf_extended_name_free(&msg->info.api_resp_info.param.ha_get.comp_name);
			osaf_extended_name_free(&msg->info.api_resp_info.param.ha_get.csi_name);
		}
	default:
		break;
	}

	return;
}

/****************************************************************************
  Name          : avsv_ndnd_avnd_msg_copy
 
  Description   : This routine copies the AvND  message
 
  Arguments     : dmsg - ptr to the dest msg
                  smsg - ptr to the source msg
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avsv_ndnd_avnd_msg_copy(AVSV_ND2ND_AVND_MSG *dmsg, AVSV_ND2ND_AVND_MSG *smsg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (!dmsg || !smsg) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* copy the common fields */
	memcpy(dmsg, smsg, sizeof(AVSV_ND2ND_AVND_MSG));
	osaf_extended_name_alloc(osaf_extended_name_borrow(&smsg->comp_name), &dmsg->comp_name);
	if (AVND_AVND_AVA_MSG == smsg->type) {
		rc = avsv_nda_ava_msg_copy(dmsg->info.msg, smsg->info.msg);
	} else if (AVND_AVND_CBK_DEL == smsg->type) {
		if (osaf_is_an_extended_name(&smsg->info.cbk_del.comp_name))
			osaf_extended_name_alloc(osaf_extended_name_borrow(&smsg->info.cbk_del.comp_name), &dmsg->info.cbk_del.comp_name);
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avsv_nda_ava_msg_copy
 
  Description   : This routine copies the AvA message.
 
  Arguments     : dmsg - ptr to the dest msg
                  smsg - ptr to the source msg
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avsv_nda_ava_msg_copy(AVSV_NDA_AVA_MSG *dmsg, AVSV_NDA_AVA_MSG *smsg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (!dmsg || !smsg) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* copy the common fields */
	memcpy(dmsg, smsg, sizeof(AVSV_NDA_AVA_MSG));

	switch (smsg->type) {
	case AVSV_AVA_API_MSG:
		rc = avsv_amf_api_copy(&dmsg->info.api_info, &smsg->info.api_info);
		break;

	case AVSV_AVND_AMF_API_RESP_MSG:
		if (smsg->info.api_resp_info.type == AVSV_AMF_HA_STATE_GET) {
			osaf_extended_name_alloc(osaf_extended_name_borrow(&smsg->info.api_resp_info.param.ha_get.comp_name),
									 &dmsg->info.api_resp_info.param.ha_get.comp_name);
			osaf_extended_name_alloc(osaf_extended_name_borrow(&smsg->info.api_resp_info.param.ha_get.csi_name),
									 &dmsg->info.api_resp_info.param.ha_get.csi_name);
		}

		break;

	case AVSV_AVND_AMF_CBK_MSG:
		rc = avsv_amf_cbk_copy(&dmsg->info.cbk_info, smsg->info.cbk_info);
		break;

	default:
		osafassert(0);
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avsv_amf_cbk_copy
 
  Description   : This routine copies the AMF callback info message.
 
  Arguments     : o_dmsg - double ptr to the dest cbk-info (o/p)
                  smsg   - ptr to the source cbk-info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avsv_amf_cbk_copy(AVSV_AMF_CBK_INFO **o_dcbk, AVSV_AMF_CBK_INFO *scbk)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t i;

	if (!o_dcbk || !scbk)
		return NCSCC_RC_FAILURE;

	/* allocate the dest cbk-info */
	*o_dcbk = malloc(sizeof(AVSV_AMF_CBK_INFO));
	if (!(*o_dcbk)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* copy the common fields */
	memcpy(*o_dcbk, scbk, sizeof(AVSV_AMF_CBK_INFO));

	switch (scbk->type) {
	case AVSV_AMF_HC:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.hc.comp_name), &(*o_dcbk)->param.hc.comp_name);
		break;

	case AVSV_AMF_COMP_TERM:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.comp_term.comp_name), &(*o_dcbk)->param.comp_term.comp_name);
		break;

	case AVSV_AMF_CSI_REM:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_rem.comp_name), &(*o_dcbk)->param.csi_rem.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_rem.csi_name), &(*o_dcbk)->param.csi_rem.csi_name);
		break;

	case AVSV_AMF_PXIED_COMP_INST:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.pxied_comp_inst.comp_name), &(*o_dcbk)->param.pxied_comp_inst.comp_name);
		break;

	case AVSV_AMF_PXIED_COMP_CLEAN:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.pxied_comp_clean.comp_name), &(*o_dcbk)->param.pxied_comp_clean.comp_name);
		break;

	case AVSV_AMF_PG_TRACK:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.pg_track.csi_name), &(*o_dcbk)->param.pg_track.csi_name);
		/* memset notify buffer */
		memset(&(*o_dcbk)->param.pg_track.buf, 0, sizeof(SaAmfProtectionGroupNotificationBufferT));

		/* copy the notify buffer, if any */
		if (scbk->param.pg_track.buf.numberOfItems) {
			(*o_dcbk)->param.pg_track.buf.notification =
			    malloc(sizeof(SaAmfProtectionGroupNotificationT) * scbk->param.pg_track.buf.numberOfItems);
			if (!(*o_dcbk)->param.pg_track.buf.notification) {
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			memcpy((*o_dcbk)->param.pg_track.buf.notification,
			       scbk->param.pg_track.buf.notification,
			       sizeof(SaAmfProtectionGroupNotificationT) * scbk->param.pg_track.buf.numberOfItems);
			for (i = 0; i < scbk->param.pg_track.buf.numberOfItems; i++) {
				osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.pg_track.buf.notification[i].member.compName),
					&(*o_dcbk)->param.pg_track.buf.notification[i].member.compName);
			}
			(*o_dcbk)->param.pg_track.buf.numberOfItems = scbk->param.pg_track.buf.numberOfItems;
		}
		break;

	case AVSV_AMF_CSI_SET:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.comp_name), &(*o_dcbk)->param.csi_set.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.csi_desc.csiName), &(*o_dcbk)->param.csi_set.csi_desc.csiName);
		/* memset avsv & amf csi attr lists */
		memset(&(*o_dcbk)->param.csi_set.attrs, 0, sizeof(AVSV_CSI_ATTRS));
		memset(&(*o_dcbk)->param.csi_set.csi_desc.csiAttr, 0, sizeof(SaAmfCSIAttributeListT));

		/* copy the avsv csi attr list */
		if (scbk->param.csi_set.attrs.number) {
			(*o_dcbk)->param.csi_set.attrs.list =
			    malloc(sizeof(AVSV_ATTR_NAME_VAL) * scbk->param.csi_set.attrs.number);
			if (!(*o_dcbk)->param.csi_set.attrs.list) {
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			memcpy((*o_dcbk)->param.csi_set.attrs.list, scbk->param.csi_set.attrs.list,
			       sizeof(AVSV_ATTR_NAME_VAL) * scbk->param.csi_set.attrs.number);

			for (i = 0; i < scbk->param.csi_set.attrs.number; i++) {
				osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.attrs.list[i].name),
										 &(*o_dcbk)->param.csi_set.attrs.list[i].name);
				osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.attrs.list[i].value),
										 &(*o_dcbk)->param.csi_set.attrs.list[i].value);
			}

			(*o_dcbk)->param.csi_set.attrs.number = scbk->param.csi_set.attrs.number;
		}

		/* Copy csi state description */
		if (scbk->param.csi_set.ha == SA_AMF_HA_ACTIVE) {
			//osaf_extended_name_alloc() takes care of long and short dn.
			osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.csi_desc.csiStateDescriptor.activeDescriptor.activeCompName),
					&(*o_dcbk)->param.csi_set.csi_desc.csiStateDescriptor.activeDescriptor.activeCompName);
		}

		if (scbk->param.csi_set.ha == SA_AMF_HA_STANDBY) {
			//osaf_extended_name_alloc() takes care of long and short dn.
			osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_set.csi_desc.csiStateDescriptor.standbyDescriptor.activeCompName),
					&(*o_dcbk)->param.csi_set.csi_desc.csiStateDescriptor.standbyDescriptor.activeCompName);
		}

		/* copy the amf csi attr list */
		if (scbk->param.csi_set.csi_desc.csiAttr.number) {
			rc = avsv_amf_csi_attr_list_copy(&scbk->param.csi_set.csi_desc.csiAttr,
							 &(*o_dcbk)->param.csi_set.csi_desc.csiAttr);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
		break;
	case AVSV_AMF_CSI_ATTR_CHANGE:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_attr_change.csi_name),
				 &(*o_dcbk)->param.csi_attr_change.csi_name);
                /* memset avsv & amf csi attr lists */
		memset(&(*o_dcbk)->param.csi_attr_change.attrs, 0, sizeof(AVSV_CSI_ATTRS));
		memset(&(*o_dcbk)->param.csi_attr_change.csiAttr, 0, sizeof(SaAmfCSIAttributeListT));
                /* copy the avsv csi attr list */
                if (scbk->param.csi_attr_change.attrs.number > 0) {
                        (*o_dcbk)->param.csi_attr_change.attrs.list = 
				malloc(sizeof(AVSV_ATTR_NAME_VAL) * scbk->param.csi_attr_change.attrs.number);

			if (!(*o_dcbk)->param.csi_attr_change.attrs.list) {
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
			memcpy((*o_dcbk)->param.csi_attr_change.attrs.list,
					scbk->param.csi_attr_change.attrs.list,
					sizeof(AVSV_ATTR_NAME_VAL) * scbk->param.csi_attr_change.attrs.number);
			for (i = 0; i < scbk->param.csi_attr_change.attrs.number; i++) {
				osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_attr_change.attrs.list[i].name),
                                                                                 &(*o_dcbk)->param.csi_attr_change.attrs.list[i].name);
                                osaf_extended_name_alloc(osaf_extended_name_borrow(&scbk->param.csi_attr_change.attrs.list[i].value),
                                                                                 &(*o_dcbk)->param.csi_attr_change.attrs.list[i].value);
                        }

                        (*o_dcbk)->param.csi_attr_change.attrs.number = scbk->param.csi_attr_change.attrs.number;
                }
		/* copy the amf csi attr list */
		if (scbk->param.csi_attr_change.csiAttr.number > 0) {
                       avsv_amf_csi_attr_list_copy(&(*o_dcbk)->param.csi_attr_change.csiAttr,
                                &scbk->param.csi_attr_change.csiAttr);
                }
		break;
	default:
		osafassert(0);
	}

 done:
	if ((NCSCC_RC_SUCCESS != rc) && *o_dcbk)
		avsv_amf_cbk_free(*o_dcbk);

	return rc;
}

/****************************************************************************
  Name          : avsv_amf_cbk_free
 
  Description   : This routine frees callback information.
 
  Arguments     : cbk_info - ptr to the callback info
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avsv_amf_cbk_free(AVSV_AMF_CBK_INFO *cbk_info)
{
	uint16_t i;

	if (!cbk_info)
		return;

	switch (cbk_info->type) {
	case AVSV_AMF_HC:
		osaf_extended_name_free(&cbk_info->param.hc.comp_name);
		break;

	case AVSV_AMF_COMP_TERM:
		osaf_extended_name_free(&cbk_info->param.comp_term.comp_name);
		break;

	case AVSV_AMF_CSI_REM:
		osaf_extended_name_free(&cbk_info->param.csi_rem.comp_name);
		osaf_extended_name_free(&cbk_info->param.csi_rem.csi_name);
		break;

	case AVSV_AMF_PXIED_COMP_INST:
		osaf_extended_name_free(&cbk_info->param.pxied_comp_inst.comp_name);
		break;

	case AVSV_AMF_PXIED_COMP_CLEAN:
		osaf_extended_name_free(&cbk_info->param.pxied_comp_clean.comp_name);
		break;

	case AVSV_AMF_PG_TRACK:
		osaf_extended_name_free(&cbk_info->param.pg_track.csi_name);
		/* free the notify buffer */
		if (cbk_info->param.pg_track.buf.numberOfItems) {
			for (i = 0; i < cbk_info->param.pg_track.buf.numberOfItems; i++) {
				osaf_extended_name_free(&cbk_info->param.pg_track.buf.notification[i].member.compName);
			}
			free(cbk_info->param.pg_track.buf.notification);
		}
		break;

	case AVSV_AMF_CSI_SET:
		osaf_extended_name_free(&cbk_info->param.csi_set.comp_name);
		/* free the avsv csi attr list */
		if (cbk_info->param.csi_set.attrs.number) {
			if (cbk_info->param.csi_set.attrs.list) {
				for (i = 0; i < cbk_info->param.csi_set.attrs.number; i++)
				{
					osaf_extended_name_free(&cbk_info->param.csi_set.attrs.list[i].name);
					osaf_extended_name_free(&cbk_info->param.csi_set.attrs.list[i].value);
				}
			}
			free(cbk_info->param.csi_set.attrs.list);
		}
		/* free the amf csi attr list */
		osaf_extended_name_free(&cbk_info->param.csi_set.csi_desc.csiName);
		if (cbk_info->param.csi_set.ha == SA_AMF_HA_ACTIVE) {
			osaf_extended_name_free(&cbk_info->param.csi_set.csi_desc.csiStateDescriptor.activeDescriptor.activeCompName);
		}
		if (cbk_info->param.csi_set.ha == SA_AMF_HA_STANDBY) {
			osaf_extended_name_free(&cbk_info->param.csi_set.csi_desc.csiStateDescriptor.standbyDescriptor.activeCompName);
		}
		avsv_amf_csi_attr_list_free(&cbk_info->param.csi_set.csi_desc.csiAttr);
		break;
	case AVSV_AMF_CSI_ATTR_CHANGE:
		osaf_extended_name_free(&cbk_info->param.csi_attr_change.csi_name);
		/* free the avsv csi attr list */
		if (cbk_info->param.csi_attr_change.attrs.number) {
			if (cbk_info->param.csi_attr_change.attrs.list) {
				for (i = 0; i < cbk_info->param.csi_attr_change.attrs.number; i++)
				{
					osaf_extended_name_free(&cbk_info->param.csi_attr_change.attrs.list[i].name);
					osaf_extended_name_free(&cbk_info->param.csi_attr_change.attrs.list[i].value);
				}
			}
			free(cbk_info->param.csi_attr_change.attrs.list);
		}
		/* free the amf csi attr list */
		avsv_amf_csi_attr_list_free(&cbk_info->param.csi_attr_change.csiAttr);
		break;
	default:
		break;
	}

	/* free the cbk-info ptr */
	free(cbk_info);
	cbk_info = NULL;

	return;
}

/****************************************************************************
  Name          : avsv_amf_api_free
 
  Description   : This routine frees api information.
 
  Arguments     : api_info - ptr to the api info
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avsv_amf_api_free(AVSV_AMF_API_INFO *api_info)
{
	if (!api_info)
		return;

	switch (api_info->type) {
	case AVSV_AMF_FINALIZE:
		osaf_extended_name_free(&api_info->param.finalize.comp_name);
		break;

	case AVSV_AMF_COMP_REG:
		osaf_extended_name_free(&api_info->param.reg.comp_name);
		osaf_extended_name_free(&api_info->param.reg.proxy_comp_name);
		break;

	case AVSV_AMF_COMP_UNREG:
		osaf_extended_name_free(&api_info->param.unreg.comp_name);
		osaf_extended_name_free(&api_info->param.unreg.proxy_comp_name);
		break;

	case AVSV_AMF_PM_START:
		osaf_extended_name_free(&api_info->param.pm_start.comp_name);
		break;

	case AVSV_AMF_PM_STOP:
		osaf_extended_name_free(&api_info->param.pm_stop.comp_name);
		break;

	case AVSV_AMF_HC_START:
		osaf_extended_name_free(&api_info->param.hc_start.comp_name);
		osaf_extended_name_free(&api_info->param.hc_start.proxy_comp_name);
		break;

	case AVSV_AMF_HC_STOP:
		osaf_extended_name_free(&api_info->param.hc_stop.comp_name);
		osaf_extended_name_free(&api_info->param.hc_stop.proxy_comp_name);
		break;

	case AVSV_AMF_HC_CONFIRM:
		osaf_extended_name_free(&api_info->param.hc_confirm.comp_name);
		osaf_extended_name_free(&api_info->param.hc_confirm.proxy_comp_name);
		break;

	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
		osaf_extended_name_free(&api_info->param.csiq_compl.comp_name);
		break;

	case AVSV_AMF_HA_STATE_GET:
		osaf_extended_name_free(&api_info->param.ha_get.comp_name);
		osaf_extended_name_free(&api_info->param.ha_get.csi_name);
		break;
		
	case AVSV_AMF_PG_START:
		osaf_extended_name_free(&api_info->param.pg_start.csi_name);
		break;

	case AVSV_AMF_PG_STOP:
		osaf_extended_name_free(&api_info->param.pg_stop.csi_name);
		break;

	case AVSV_AMF_ERR_REP:
		osaf_extended_name_free(&api_info->param.err_rep.err_comp);
		break;

	case AVSV_AMF_ERR_CLEAR:
		osaf_extended_name_free(&api_info->param.err_clear.comp_name);
		break;

	case AVSV_AMF_RESP:
		osaf_extended_name_free(&api_info->param.resp.comp_name);
		break;

	default:
		break;
	}
}

/****************************************************************************
  Name          : avsv_amf_api_copy

  Description   : This routine copies api information.

  Arguments     : api_info - ptr to the api info

  Return Values : None.

  Notes         : None.
******************************************************************************/
uint32_t avsv_amf_api_copy(AVSV_AMF_API_INFO *d_api_info, AVSV_AMF_API_INFO *s_api_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (!d_api_info || !s_api_info)
		return NCSCC_RC_FAILURE;

	switch (s_api_info->type) {
	case AVSV_AMF_FINALIZE:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.finalize.comp_name),
			&s_api_info->param.finalize.comp_name);
		break;

	case AVSV_AMF_COMP_REG:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.reg.comp_name),
			&d_api_info->param.reg.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.reg.proxy_comp_name),
			&d_api_info->param.reg.proxy_comp_name);
		break;

	case AVSV_AMF_COMP_UNREG:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.unreg.comp_name),
			&d_api_info->param.unreg.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.unreg.proxy_comp_name),
			&d_api_info->param.unreg.proxy_comp_name);
		break;

	case AVSV_AMF_PM_START:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.pm_start.comp_name),
			&d_api_info->param.pm_start.comp_name);
		break;

	case AVSV_AMF_PM_STOP:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.pm_stop.comp_name),
			&d_api_info->param.pm_stop.comp_name);
		break;

	case AVSV_AMF_HC_START:

		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_start.comp_name),
			&d_api_info->param.ha_get.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_start.proxy_comp_name),
			&d_api_info->param.hc_start.proxy_comp_name);
		break;

	case AVSV_AMF_HC_STOP:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_stop.comp_name),
			&d_api_info->param.hc_stop.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_stop.proxy_comp_name),
			&d_api_info->param.hc_stop.proxy_comp_name);
		break;

	case AVSV_AMF_HC_CONFIRM:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_confirm.comp_name),
			&d_api_info->param.hc_confirm.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.hc_confirm.proxy_comp_name),
			&d_api_info->param.hc_confirm.proxy_comp_name);
		break;

	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.csiq_compl.comp_name),
			&d_api_info->param.csiq_compl.comp_name);
		break;

	case AVSV_AMF_HA_STATE_GET:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.ha_get.comp_name),
			&d_api_info->param.ha_get.comp_name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.ha_get.csi_name),
			&d_api_info->param.ha_get.csi_name);
		break;

	case AVSV_AMF_PG_START:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.pg_start.csi_name),
			&d_api_info->param.pg_start.csi_name);
		break;

	case AVSV_AMF_PG_STOP:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.pg_stop.csi_name),
			&d_api_info->param.pg_stop.csi_name);
		break;

	case AVSV_AMF_ERR_REP:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.err_rep.err_comp),
			&d_api_info->param.err_rep.err_comp);
		break;

	case AVSV_AMF_ERR_CLEAR:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.err_clear.comp_name),
			&d_api_info->param.err_clear.comp_name);
		break;

	case AVSV_AMF_RESP:
		osaf_extended_name_alloc(osaf_extended_name_borrow(&s_api_info->param.resp.comp_name),
			&d_api_info->param.resp.comp_name);
		break;

	default:
		break;
	}

	return rc;
}

/****************************************************************************
  Name          : avsv_amf_csi_attr_list_copy
 
  Description   : This routine copies the csi attribute list.
 
  Arguments     : dattr - ptr to the destination csi-attr list
                  sattr - ptr to the src csi-attr list
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_amf_csi_attr_list_copy(SaAmfCSIAttributeListT *dattr,
				     const SaAmfCSIAttributeListT *sattr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t cnt;

	if (!dattr || !sattr)
		goto done;

	dattr->attr = malloc(sizeof(SaAmfCSIAttributeT) * sattr->number);
	osafassert(dattr->attr);

	for (cnt = 0; cnt < sattr->number; cnt++) {
		/* alloc memory for attr name & value */
		size_t attrNameSize = strlen((char*)sattr->attr[cnt].attrName) + 1;
		dattr->attr[cnt].attrName = malloc(attrNameSize);
		osafassert(dattr->attr[cnt].attrName);

		size_t attrValueSize = strlen((char*)sattr->attr[cnt].attrValue) + 1;
		dattr->attr[cnt].attrValue = malloc(attrValueSize);
		osafassert(dattr->attr[cnt].attrValue);

		/* copy the attr name & value */
		strncpy((char*)dattr->attr[cnt].attrName,
			(char*)sattr->attr[cnt].attrName, attrNameSize);
		strncpy((char*)dattr->attr[cnt].attrValue,
			(char*)sattr->attr[cnt].attrValue, attrValueSize);

		/* increment the attr name-val pair cnt that is copied */
		dattr->number++;
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		avsv_amf_csi_attr_list_free(dattr);
		dattr->attr = 0;
		dattr->number = 0;
	}

	return rc;
}

/****************************************************************************
  Name          : avsv_amf_csi_attr_list_free
 
  Description   : This routine frees the csi attribute list.
 
  Arguments     : attr - ptr to the csi-attr list
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avsv_amf_csi_attr_list_free(SaAmfCSIAttributeListT *attrs)
{
	uint32_t cnt;

	if (!attrs)
		return;

	/* free the attr name-val pair */
	for (cnt = 0; cnt < attrs->number; cnt++) {
		free(attrs->attr[cnt].attrName);
		free(attrs->attr[cnt].attrValue);
	}			/* for */

	/* finally free the attr list ptr */
	if (attrs->attr)
		free(attrs->attr);

	return;
}

/****************************************************************************
  Name          : avsv_amf_csi_attr_convert
 
  Description   : This routine converts the CSI attributes to a form that the 
                  application understands.
 
  Arguments     : cbk_info - ptr to the callback info

  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_amf_csi_attr_convert(AVSV_AMF_CBK_INFO *cbk_info)
{
	SaAmfCSIAttributeListT *amf_attrs = 0;
	AVSV_CSI_ATTRS *avsv_attrs = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if ((!cbk_info) || (AVSV_AMF_CSI_SET != cbk_info->type) ||
	    (SA_AMF_CSI_ADD_ONE != cbk_info->param.csi_set.csi_desc.csiFlags))
		goto done;

	/* get the amf & avsv attr ptrs */
	amf_attrs = &cbk_info->param.csi_set.csi_desc.csiAttr;
	avsv_attrs = &cbk_info->param.csi_set.attrs;

	if (!avsv_attrs->number)
		goto done;

	rc = avsv_attrs_to_amf_attrs(amf_attrs, avsv_attrs);
done:
	return rc;
}

/**
 * @brief  Copies csi attributes from internal structure AVSV_CSI_ATTRS 
 *         to SaAmfCSIAttributeListT. Application understands SaAmfCSIAttributeListT.
 *         All memory allocation will be done here.
 *
 * @param  amf_attrs (ptr to SaAmfCSIAttributeListT).
 * @param  avsv_attrs (ptr to AVSV_CSI_ATTRS).
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
uint32_t avsv_attrs_to_amf_attrs (SaAmfCSIAttributeListT *amf_attrs, AVSV_CSI_ATTRS *avsv_attrs)
{
	uint32_t cnt, rc = NCSCC_RC_SUCCESS;

	amf_attrs->attr = malloc(sizeof(SaAmfCSIAttributeT) * avsv_attrs->number);
	if (!amf_attrs->attr) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	for (cnt = 0; cnt < avsv_attrs->number; cnt++) {
		/* alloc memory for attr name & value */
		amf_attrs->attr[cnt].attrName = malloc(osaf_extended_name_length(&avsv_attrs->list[cnt].name) + 1);
		if (!amf_attrs->attr[cnt].attrName) {
			free(amf_attrs->attr[cnt].attrName);
			goto done;
		}

		amf_attrs->attr[cnt].attrValue = malloc(strlen(avsv_attrs->list[cnt].string_ptr)+ 1);
		if (!amf_attrs->attr[cnt].attrValue) {
			free(amf_attrs->attr[cnt].attrName);
			free(amf_attrs->attr[cnt].attrValue);
			goto done;
		}

		/* copy the attr name & value */
		memcpy(amf_attrs->attr[cnt].attrName, osaf_extended_name_borrow(&avsv_attrs->list[cnt].name),
		       osaf_extended_name_length(&avsv_attrs->list[cnt].name));
		memcpy(amf_attrs->attr[cnt].attrValue, avsv_attrs->list[cnt].string_ptr,
			strlen(avsv_attrs->list[cnt].string_ptr));
		*(amf_attrs->attr[cnt].attrName + osaf_extended_name_length(&avsv_attrs->list[cnt].name)) = '\0';
		*(amf_attrs->attr[cnt].attrValue + strlen(avsv_attrs->list[cnt].string_ptr)) = '\0';

		/* increment the attr name-val pair cnt that is copied */
		amf_attrs->number++;
	}			/* for */

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		avsv_amf_csi_attr_list_free(amf_attrs);
		amf_attrs->attr = 0;
		amf_attrs->number = 0;
	}

	return rc;
}

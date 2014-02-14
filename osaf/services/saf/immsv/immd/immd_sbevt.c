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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
  FILE NAME: immd_sbevt.c

  DESCRIPTION: IMMD Standby Processing Routines
*****************************************************************************/

#include "immd.h"

/***************************************************************************
 * Name        : immd_process_sb_fevs
 
 * Description : This is used to replicate a fevs message at Standby.

 * Arguments   : fevs_msg - The IMMSV_FEVS message.

 * Return Values : Success / Error

**********************************************************************/
uint32_t immd_process_sb_fevs(IMMD_CB *cb, IMMSV_FEVS *fevs_msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_5("Message count: %llu", fevs_msg->sender_count);
	TRACE_5("size:%u", fevs_msg->msg.size);
	if (cb->fevsSendCount + 1 != fevs_msg->sender_count) {
		if(cb->fevsSendCount) {
			/* After the fix for #280 standby starts getting 
			   fevs *after* loading */
			LOG_WA("Message count:%llu + 1 != %llu", 
				cb->fevsSendCount, fevs_msg->sender_count);
		}
		cb->fevsSendCount = fevs_msg->sender_count;
	} else {
		cb->fevsSendCount++;
	}

	immd_db_save_fevs(cb, fevs_msg);

	TRACE_5("cb->fevsSendCount:%llu", cb->fevsSendCount);
	TRACE_5("cb->admo_id_count:%u", cb->admo_id_count);
	TRACE_5("cb->impl_count:%u", cb->impl_count);
	TRACE_5("cb->ccb_id_count:%u", cb->ccb_id_count);
	return rc;
}

/***************************************************************************
 * Name        : immd_sb_process_sb_count
 
 * Description : This is used to replicate counters at Standby.

 * Arguments   : counter - The Counter value.
                 evt_type- The type of counter/event.

 * Return Values : Success / Error

**********************************************************************/
uint32_t immd_process_sb_count(IMMD_CB *cb, uint32_t count, uint32_t evt_type)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	switch (evt_type) {
	case IMMD_A2S_MSG_ADMINIT:
		if (cb->admo_id_count + 1 != count) {
			LOG_WA("ADMO-id: %u +1 != %u", cb->admo_id_count, count);
		}
		cb->admo_id_count = count;
		TRACE_5("cb->admo_id_count:%u", cb->admo_id_count);
		if (cb->admo_id_count == 0xffffffff) {
			cb->admo_id_count = 0;
			LOG_WA("Standby: admo_id_count wrap arround");
		}
		break;

	case IMMD_A2S_MSG_IMPLSET:
		if (cb->impl_count + 1 != count) {
			LOG_WA("IMPL-id: %u + 1 != %u", cb->impl_count, count);
		}
		cb->impl_count = count;
		TRACE_5("cb->impl_count:%u", cb->impl_count);
		if (cb->impl_count == 0xffffffff) {
			cb->impl_count = 0;
			LOG_WA("Standby: impl_count wrap arround");
		}
		break;

	case IMMD_A2S_MSG_CCBINIT:
		if (cb->ccb_id_count + 1 != count) {
			LOG_WA("CCB-id: %u + 1 != %u", cb->ccb_id_count + 1, count);
		}
		cb->ccb_id_count = count;
		TRACE_5("cb->ccb_id_count:%u", cb->ccb_id_count);
		if (cb->ccb_id_count == 0xffffffff) {
			cb->ccb_id_count = 0;
			LOG_WA("Standby: ccb_id_count wrap arround");
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
		break;
	}

	return rc;
}

/***************************************************************************
 * Name        : immd_sb_process_node_accept
 
 * Description : This is used to replicate IMMND node-info at Standby.

**********************************************************************/
uint32_t immd_process_node_accept(IMMD_CB *cb, IMMSV_D2ND_CONTROL *ctrl)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_DEST key;
	IMMD_IMMND_INFO_NODE *immnd_info_node;
	bool checkImmd2Pbe=false;
	TRACE_ENTER();

	TRACE_5("NodeId: %x epoch:%u canBeCoord:%u isCoord:%u syncStart:%u, rulingEpoch:%u pbe:%u",
		ctrl->nodeId, ctrl->nodeEpoch, ctrl->canBeCoord, ctrl->isCoord, ctrl->syncStarted, 
		ctrl->rulingEpoch, ctrl->pbeEnabled);

	if((ctrl->canBeCoord > 1) && !(immd_cb->mIs2Pbe)) {
		LOG_ER("Active IMMD has 2PBE enabled, yet this standby is not enabled for 2PBE - exiting");
		exit(1);
	} else if((cb->immnd_coord == 0) && immd_cb->mIs2Pbe && (ctrl->canBeCoord == 1)) {
		/* If 2Pbe is enabled, then ctrl->canBeCoord must be 2 or 3 for first 
		   node accpet message for each SC. Subsequent may have ctgrl->canBeCoord
		   set to just 1. First message check done below using checkImmd2Pbe.
		*/
		checkImmd2Pbe=true;
	}

	if (cb->mRulingEpoch < ctrl->rulingEpoch) {
		cb->mRulingEpoch = ctrl->rulingEpoch;
		LOG_NO("SBY: Ruling epoch noted as:%u", cb->mRulingEpoch);
	}

	if(ctrl->pbeEnabled == 2) {ctrl->pbeEnabled = 0;} 

	osafassert(cb->is_immnd_tree_up);
	memset(&key, 0, sizeof(MDS_DEST));
	immd_immnd_info_node_getnext(&cb->immnd_tree, &key, &immnd_info_node);
	while (immnd_info_node) {
		key = immnd_info_node->immnd_dest;
		if (immnd_info_node->immnd_key == ctrl->nodeId) {
			break;	/*Out of while */
		}
		immd_immnd_info_node_getnext(&cb->immnd_tree, &key, &immnd_info_node);
	}

	if (immnd_info_node) {
		if (immnd_info_node->epoch < ctrl->nodeEpoch) {
			LOG_NO("SBY: New Epoch for IMMND process at node %x "
				"old epoch: %u  new epoch:%u", ctrl->nodeId,
				immnd_info_node->epoch, ctrl->nodeEpoch);
			

			immnd_info_node->epoch = ctrl->nodeEpoch;
		}
		if (!(immnd_info_node->isOnController) && ctrl->canBeCoord) {
			immnd_info_node->isOnController = true;
			TRACE_5("Corrected isOnController status for immnd node info");


			if(checkImmd2Pbe) {
				LOG_ER("Active IMMD does not have 2PBE enabled, yet this standby "
					"is enabled for 2PBE - exiting %u %u %u",
					cb->immnd_coord, immd_cb->mIs2Pbe, ctrl->canBeCoord);
				exit(1);
			}
		}

		immnd_info_node->isCoord = ctrl->isCoord;

		if(ctrl->isCoord) {
			SaImmRepositoryInitModeT oldRim = cb->mRim;
			cb->immnd_coord = immnd_info_node->immnd_key;
			cb->m2PbeCanLoad = true;
			LOG_NO("IMMND coord at %x", immnd_info_node->immnd_key);
			immnd_info_node->syncStarted = ctrl->syncStarted;
			cb->mRim = (ctrl->pbeEnabled==4)?SA_IMM_KEEP_REPOSITORY:SA_IMM_INIT_FROM_FILE;
			if(cb->mRim != oldRim) {
				LOG_NO("SBY: SaImmRepositoryInitModeT changed and noted as '%s'",
					(cb->mRim == SA_IMM_KEEP_REPOSITORY)?
					"SA_IMM_KEEP_REPOSITORY":"SA_IMM_INIT_FROM_FILE");
			} else {
				TRACE("SBY: SaImmRepositoryInitModeT stable as %u ctrl->pbeEnabled:%u", 
					oldRim, ctrl->pbeEnabled);
			}
		}

		int oldPid = immnd_info_node->immnd_execPid;
		if (immnd_info_node->immnd_execPid != ctrl->ndExecPid) {
			immnd_info_node->immnd_execPid = ctrl->ndExecPid;
			if (oldPid) {
				LOG_IN("Corrected execPid for immnd node info");
			}
		}

		if(!(ctrl->canBeCoord)) { /* payload node */
			/* Remove the node-id from the list of detached payloads. */
			IMMD_IMMND_DETACHED_NODE *detached_node = cb->detached_nodes;
			IMMD_IMMND_DETACHED_NODE **prev = &(cb->detached_nodes);
			while(detached_node) {
				if(detached_node->node_id == ctrl->nodeId) {
					*prev = detached_node->next;
					detached_node->next = NULL;
					break;/* out of while */
				}

				/* next iteration */
				prev = &(detached_node->next);
				detached_node = detached_node->next;
			}

			if(detached_node) {
				free(detached_node);
				LOG_IN("SBY: Received node-accept from active IMMD for "
					"payload node %x, discarding pending removal record.",
					ctrl->nodeId);
			}
		}
	} else {
		LOG_IN("Standby IMMD could not find node with nodeId:%x", ctrl->nodeId);
	}

	if(ctrl->pbeEnabled >= 3) { /* Extended intro. */
		TRACE("Standby receiving FS params: %s %s %s", 
			ctrl->dir.buf, ctrl->xmlFile.buf, ctrl->pbeFile.buf);

		if(ctrl->dir.size && cb->mDir==NULL && ctrl->canBeCoord) {
			TRACE("cb->mDir set to %s in standby", ctrl->dir.buf);
			cb->mDir = ctrl->dir.buf; /*steal*/
		} else if(ctrl->dir.size && cb->mDir) {
			/* Should not get here since fs params sent only once.*/
			if(strcmp(cb->mDir, ctrl->dir.buf)) {
				LOG_WA("SBY: Discrepancy on IMM directory: %s != %s",
					cb->mDir, ctrl->dir.buf);
			}
			free(ctrl->dir.buf);
		}
		ctrl->dir.buf=NULL;
		ctrl->dir.size=0;


		if(ctrl->xmlFile.size && cb->mFile==NULL && ctrl->canBeCoord) {
			TRACE("cb->mFile set to %s in standby",ctrl->xmlFile.buf );
			cb->mFile = ctrl->xmlFile.buf; /*steal*/
		} else if(ctrl->xmlFile.size && cb->mFile) {
			/* Should not get here since fs params sent only once.*/
			if(strcmp(cb->mFile, ctrl->xmlFile.buf)) {
				LOG_WA("SBY: Discrepancy on IMM XML file: %s != %s",
					cb->mFile, ctrl->xmlFile.buf);
			}
			free(ctrl->xmlFile.buf);
		}
		ctrl->xmlFile.buf=NULL;
		ctrl->xmlFile.size=0;


		if(ctrl->pbeFile.size && cb->mPbeFile==NULL && ctrl->canBeCoord) {
			TRACE("cb->mPbeFile set to %s in standby", ctrl->pbeFile.buf);
			cb->mPbeFile = ctrl->pbeFile.buf; /*steal*/
		} else if(ctrl->pbeFile.size && cb->mPbeFile) {
			/* Should not get here since fs params sent only once.*/
			if(strcmp(cb->mPbeFile, ctrl->pbeFile.buf)) {
				LOG_WA("SBY: Discrepancy on IMM PBE file: %s != %s",
					cb->mPbeFile, ctrl->pbeFile.buf);
			}
			free(ctrl->pbeFile.buf);
		}
		ctrl->pbeFile.buf=NULL;
		ctrl->pbeFile.size=0;
	}


	TRACE_LEAVE();
	immd_cb_dump();
	return rc;
}

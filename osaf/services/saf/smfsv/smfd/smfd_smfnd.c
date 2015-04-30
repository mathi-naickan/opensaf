/*      OpenSAF 
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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <configmake.h>

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "saAis.h"
#include <logtrace.h>
#include <saf_error.h>
#include "osaf_extended_name.h"

#include "smfd.h"
#include "smfd_smfnd.h"
#include "smfsv_evt.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

static SmfndNodeT *firstSmfnd = NULL;
static SaVersionT clmVersion = { 'B', 1, 1 };

static pthread_mutex_t smfnd_list_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t smfnd_remote_cmd(const char *i_cmd, MDS_DEST i_smfnd_dest, uint32_t i_timeout);

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

 /**
 * smfnd_for_name
 * @param  i_nodeName Clm node name
 * @param  o_nodeDest Node destination output if return true.
 */
bool smfnd_for_name(const char *i_nodeName, SmfndNodeDest* o_nodeDest)
{
	SmfndNodeT *smfnd = NULL;

        pthread_mutex_lock(&smfnd_list_lock);
        smfnd = firstSmfnd;     
	while (smfnd != NULL) {
		if ((strcmp(osaf_extended_name_borrow(&smfnd->clmInfo.nodeName),i_nodeName) == 0) &&
                    (smfnd->nd_state == ndUp)) {
                        o_nodeDest->dest = smfnd->dest;
                        o_nodeDest->rem_svc_pvt_ver = smfnd->rem_svc_pvt_ver;
                        o_nodeDest->nd_up_cntr = smfnd->nd_up_cntr;
                        pthread_mutex_unlock(&smfnd_list_lock);
                        return true;
		}
		smfnd = smfnd->next;
	}

        pthread_mutex_unlock(&smfnd_list_lock);
	return false;
}

/**
 * get_smfnd
 * @param i_node_id Clm node id
 */
SmfndNodeT *get_smfnd(SaClmNodeIdT i_node_id)
{
	SmfndNodeT *smfnd = firstSmfnd;
        /* Lock must already be taken */
	while (smfnd != NULL) {
		if (smfnd->clmInfo.nodeId == i_node_id) {
			return smfnd;
		}
		smfnd = smfnd->next;
	}

	return NULL;
}

/**
 * smfnd_up
 * @param i_node_id Clm node id
 * @param i_smfnd_dest Destination to Clm node
 */
uint32_t smfnd_up(SaClmNodeIdT i_node_id, MDS_DEST i_smfnd_dest, MDS_SVC_PVT_SUB_PART_VER i_rem_svc_pvt_ver)
{
	SaAisErrorT rc;
	SaClmHandleT clmHandle;
	SmfndNodeT *smfnd = NULL;
        bool newNode = false;

	TRACE("SMFND UP for node id %x, version %u", i_node_id, i_rem_svc_pvt_ver);

	/* Check if the node id does already exists */
        pthread_mutex_lock(&smfnd_list_lock);
	smfnd = get_smfnd(i_node_id);

	if (smfnd == NULL) {
                TRACE("New node Id, create new SmfndNodeT structure");
                smfnd = calloc(1, sizeof(SmfndNodeT));
                if (smfnd == NULL) {
                        LOG_ER("alloc of SmfndNodeT failed");
                        pthread_mutex_unlock(&smfnd_list_lock);
                        return NCSCC_RC_FAILURE;
                }
                newNode = true;
	}

	/* Find Clm info about the node */
	rc = saClmInitialize(&clmHandle, NULL, &clmVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmInitialize failed, rc=%s", saf_error(rc));
		if (newNode) free(smfnd);
		pthread_mutex_unlock(&smfnd_list_lock);
		return NCSCC_RC_FAILURE;
	}

	/* Get Clm info about the node */
	SaClmClusterNodeT clmInfo;
	rc = saClmClusterNodeGet(clmHandle, i_node_id,
				 10000000000LL, &clmInfo);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmClusterNodeGet failed, rc=%s", saf_error(rc));
		if (newNode) free(smfnd);
		rc = saClmFinalize(clmHandle);
		if (rc != SA_AIS_OK) {
			LOG_ER("saClmFinalize failed, rc=%s", saf_error(rc));
		}
		pthread_mutex_unlock(&smfnd_list_lock);
		return NCSCC_RC_FAILURE;
	}

	rc = saClmFinalize(clmHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmFinalize failed, rc=%s", saf_error(rc));
	}

	/* Store cluster node info */
	memcpy(&smfnd->clmInfo, &clmInfo, sizeof(clmInfo));

	/* Store the destination to the smfnd */
	smfnd->dest = i_smfnd_dest;

        /* Store the version of the smfnd */
        smfnd->rem_svc_pvt_ver = i_rem_svc_pvt_ver;
        /* Step up-counter */
        smfnd->nd_up_cntr++;
        /* Store the nd state */
        smfnd->nd_state = ndUp;

        /* If the node was new, link it to the smfnd list */
        if (newNode == true) {
                smfnd->next = firstSmfnd;
                firstSmfnd = smfnd;
                TRACE("New SMFND data linked to node list for node name %s, id %x, version %u",
                      osaf_extended_name_borrow(&smfnd->clmInfo.nodeName), smfnd->clmInfo.nodeId,
                      smfnd->rem_svc_pvt_ver);
        } else {
                TRACE("Existing SMFND data updated for node name %s, id %x, version %u",
                      osaf_extended_name_borrow(&smfnd->clmInfo.nodeName), smfnd->clmInfo.nodeId,
                      smfnd->rem_svc_pvt_ver);
        }

        pthread_mutex_unlock(&smfnd_list_lock);

        return NCSCC_RC_SUCCESS;
}

/**
 * smfnd_down
 * @param i_node_id Clm node id
 */
uint32_t smfnd_down(SaClmNodeIdT i_node_id)
{
	SmfndNodeT *smfnd = NULL;

	TRACE("SMFND DOWN for node id %x", i_node_id);

        pthread_mutex_lock(&smfnd_list_lock);
        smfnd = firstSmfnd;

        /* Update the node info */
	while (smfnd != NULL) {
  		if (smfnd->clmInfo.nodeId == i_node_id) {
                        /* Store the nd state */
                        TRACE("SMFND state updated for node [%s], id [%x]", 
                              osaf_extended_name_borrow(&smfnd->clmInfo.nodeName), smfnd->clmInfo.nodeId);
                        smfnd->nd_state = ndDown;
                        pthread_mutex_unlock(&smfnd_list_lock);
                        return NCSCC_RC_SUCCESS;
                }

                smfnd = smfnd->next;
        }

        pthread_mutex_unlock(&smfnd_list_lock);
        LOG_ER("node id [%x] not found for state update DOWN", i_node_id);
        return NCSCC_RC_FAILURE;
}

/**
 * smfnd_exec_remote_cmd
 * @param i_cmd Remote command to be executed
 * @param i_smfnd Info about the smfnd node where to execute
 *                     the command
 * @param i_timeout Max time the command may take in 10 ms
 */
uint32_t smfnd_exec_remote_cmd(const char *i_cmd, 
                               const SmfndNodeDest* i_smfnd, 
                               uint32_t i_timeout,
                               uint32_t i_localTimeout)
{
	SMFSV_EVT cmd_req_asynch;
	SMFSV_EVT *cmd_rsp = 0;
	uint32_t rc;

	/* If i_cmd is empty, do nothing just return OK
           This have two purposes:
	   1) Handle the case the campaign writer specify an empty string, which is allowed in the xml schema.
           2) Make it possible to specify e.g. a "doCliCommand" together with an empty "undoCliCommand"
	*/
	if((i_cmd == NULL) || (*i_cmd == 0)) {
		TRACE("Empty remote command, continue");
		return 0;
	}

        if (i_timeout == 0) {
                LOG_ER("Command %s executed with zero timeout", i_cmd);
		return SMFSV_CMD_EXEC_FAILED;
        }

        if (i_localTimeout == 0) {
                /* The cli timeout is now handled by the smfnd so the local timeout
                   is set to +30 sec unless specified by caller (e.g. for reboot cmd) */
                i_localTimeout = i_timeout + 3000;
        }

        if (i_smfnd->rem_svc_pvt_ver == 1) {
                /* This addressed smfnd can only handle the old cmd req message format */
                return smfnd_remote_cmd(i_cmd, i_smfnd->dest, i_timeout);
        }

        /* A new smfnd can handle the asynch message */
	cmd_req_asynch.type = SMFSV_EVT_TYPE_SMFND;
	cmd_req_asynch.info.smfnd.type = SMFND_EVT_CMD_REQ_ASYNCH;
	cmd_req_asynch.info.smfnd.event.cmd_req_asynch.timeout = i_timeout * 10; /* ms */
	cmd_req_asynch.info.smfnd.event.cmd_req_asynch.cmd = (char *)i_cmd;
	cmd_req_asynch.info.smfnd.event.cmd_req_asynch.cmd_len = strlen(i_cmd);

	rc = smfsv_mds_msg_sync_send(smfd_cb->mds_handle,
				     NCSMDS_SVC_ID_SMFND,
				     i_smfnd->dest,
				     NCSMDS_SVC_ID_SMFD,
				     &cmd_req_asynch, 
                                     &cmd_rsp, 
                                     i_localTimeout); /* cs (10 ms) */

	if (rc != NCSCC_RC_SUCCESS) {
                if (rc == NCSCC_RC_REQ_TIMOUT) {
                        LOG_NO("Command %s timed out locally", i_cmd);
                }
                else {
                        LOG_ER("Command %s failed rc %d (error in communication with SMFND)", i_cmd, rc);
                }
		return SMFSV_CMD_EXEC_FAILED;
	}

        if (cmd_rsp->info.smfd.event.cmd_rsp.result != 0) { /* 0 = cmd OK */
                switch (cmd_rsp->info.smfd.event.cmd_rsp.result & 0xffff0000) {
                case SMFSV_CMD_EXEC_FAILED: {
                        LOG_ER("Command %s failed to start (%u)", 
                               i_cmd, cmd_rsp->info.smfd.event.cmd_rsp.result & 0xffff);
                        break;
                }
                case SMFSV_CMD_TIMEOUT: {
                        LOG_ER("Command %s timed out (timeout %u ms)", i_cmd, i_timeout * 10);
                        break;
                }
                case SMFSV_CMD_RESULT_CODE: {
                        LOG_ER("Command %s returned error %u", 
                               i_cmd, cmd_rsp->info.smfd.event.cmd_rsp.result & 0xffff);
                        break;
                }
                case SMFSV_CMD_SIGNAL_TERM: {
                        LOG_ER("Command %s terminated by signal %u", 
                               i_cmd, cmd_rsp->info.smfd.event.cmd_rsp.result & 0xffff);
                        break;
                }
                default:{
                        LOG_ER("Command %s failed by unknown reason %x", 
                               i_cmd, cmd_rsp->info.smfd.event.cmd_rsp.result);
                        break;
                }
                }
        }

	rc = cmd_rsp->info.smfd.event.cmd_rsp.result;
	free(cmd_rsp);
	return rc;
}

/**
 * smfnd_remote_cmd
 * @param i_cmd Remote command to be executed
 * @param i_smfnd_dest Destination to the node where to execute
 *                     the command
 * @param i_timeout Max time the command may take
 */
uint32_t smfnd_remote_cmd(const char *i_cmd, MDS_DEST i_smfnd_dest, uint32_t i_timeout)
{
	SMFSV_EVT cmd_req;
	SMFSV_EVT *cmd_rsp = NULL;
	uint32_t rc;

	/* If i_cmd is empty, do nothing just return OK
           This have two purposes:
	   1) Handle the case the campaign writer specify an empty string, which is allowed in the xml schema.
           2) Make it possible to specify e.g. a "doCliCommand" together with an empty "undoCliCommand"
	*/
	if((i_cmd == NULL) || (*i_cmd == 0)) {
		TRACE("Empty remote command, continue");
		return 0;
	}

	cmd_req.type = SMFSV_EVT_TYPE_SMFND;
	cmd_req.info.smfnd.type = SMFND_EVT_CMD_REQ;
	cmd_req.info.smfnd.event.cmd_req.cmd = (char *)i_cmd;
	cmd_req.info.smfnd.event.cmd_req.cmd_len = strlen(i_cmd);

	rc = smfsv_mds_msg_sync_send(smfd_cb->mds_handle,
				     NCSMDS_SVC_ID_SMFND,
				     i_smfnd_dest,
				     NCSMDS_SVC_ID_SMFD,
				     &cmd_req, &cmd_rsp, i_timeout);

	if (rc != NCSCC_RC_SUCCESS) {
                if (rc == NCSCC_RC_REQ_TIMOUT) {
                        LOG_ER("Command %s timed out", i_cmd);
                }
                else {
                        LOG_ER("Command %s failed rc %d (error in communication with SMFND)", i_cmd, rc);
                }
		return SMFSV_CMD_EXEC_FAILED;
	}

	rc = cmd_rsp->info.smfd.event.cmd_rsp.result;
	free(cmd_rsp);
	return rc;
}

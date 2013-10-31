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

#include <saAis.h>
#include <logtrace.h>
#include <avd_cb.h>
#include <avd.h>
#include <avd_clm.h>
#include <avd_node.h>

static SaVersionT clmVersion = { 'B', 4, 1 };

static void clm_node_join_complete(AVD_AVND *node)
{
	AVD_SU *su;

	TRACE_ENTER();
	/* For each of the SUs calculate the readiness state. 
	 ** call the SG FSM with the new readiness state.
	 */

	if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		LOG_NO("Amf Node is in locked-inst state, amf admin state is %u", node->saAmfNodeAdminState);
		goto done;
	}

	avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_ENABLED);
	su = node->list_of_su;
	while (su != NULL) {
		/* For non-preinstantiable SU unlock-inst will not lead to its inst until unlock. */
		if ( su->saAmfSUPreInstantiable == false ) {
			/* Skip the instantiation. */
		} else {

			if ((node->node_state == AVD_AVND_STATE_PRESENT)   ||
					(node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
					(node->node_state == AVD_AVND_STATE_NCS_INIT)) {
				if ((su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
						(su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
						(su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED)) {
					/* When the SU will instantiate then prescence state change message will come
					   and so store the callback parameters to send response later on. */
					if (su->sg_of_su->saAmfSGNumPrefInserviceSUs >
							(sg_instantiated_su_count(su->sg_of_su) +
							 su->sg_of_su->try_inst_counter)) {
						if (avd_snd_presence_msg(avd_cb, su, false) == NCSCC_RC_SUCCESS) {
							m_AVD_SET_SU_TERM(avd_cb, su, false);
							su->sg_of_su->try_inst_counter++;
						} else {
							LOG_ER("Internal error, could not send message to avnd");
						}
					}
				}
			}
		}
		/* get the next SU on the node */
		su = su->avnd_list_su_next;
	}

	node_reset_su_try_inst_counter(node);

done:
	TRACE_LEAVE();
}


/* validating this node for a graceful exit */ 
static void clm_node_exit_validate(AVD_AVND *node)
{
	AVD_SU *su;
	AVD_SU_SI_REL *susi;
	SaBoolT reject = SA_FALSE;
	SaAisErrorT rc = SA_AIS_OK;

	/*
	 * Reject validate step on self node as this is active controller 
	 */
	if (node->node_info.nodeId == avd_cb->node_id_avd) {
		reject = SA_TRUE;
		LOG_NO("Validate Step on Active Controller %d",
				avd_cb->node_id_avd);
		goto done;
	}

	/* now go through each SU to determine whether
	 any SI assigned becomes unassigned due to node exit*/
	su = node->list_of_su;
	while (su != NULL) {
		susi = su->list_of_susi;
		/* now evalutate each SI that is assigned to this SU */
		while (susi != NULL) {
			if ((susi->state == SA_AMF_HA_ACTIVE) &&
			    (susi->si->saAmfSINumCurrActiveAssignments == 1) &&
			    (susi->si->saAmfSINumCurrStandbyAssignments == 0) &&
			    (su->sg_of_su->sg_redundancy_model != SA_AMF_NO_REDUNDANCY_MODEL)) {
				/* there is only one active assignment w.r.t this SUSI */
				reject = SA_TRUE;
				goto done;
			}
			susi = susi->su_next;
		}
	}

done:
	if(reject == SA_FALSE) { 
		rc = saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv,
						SA_CLM_CALLBACK_RESPONSE_OK);
		LOG_NO("CLM track callback VALIDATE::ACCEPTED %u", rc);
	}
	else { 
		rc = saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv,
						SA_CLM_CALLBACK_RESPONSE_REJECTED);
		LOG_NO("CLM track callback VALIDATE::REJECTED %u", rc);
	}

	node->clm_pend_inv = 0;
	TRACE_LEAVE();
} 

/* perform required operations on this node for a graceful exit */ 
static void clm_node_exit_start(AVD_AVND *node, SaClmClusterChangesT change)
{
	TRACE_ENTER();

	if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
		saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv, 
			SA_CLM_CALLBACK_RESPONSE_OK);
		node->clm_pend_inv = 0; 
		goto done;
	}

	if (node->saAmfNodeAdminState != SA_AMF_ADMIN_UNLOCKED) {
		LOG_NO("Amf Node is not in unlocked state, amf admin state is '%u'", node->saAmfNodeAdminState);
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED) {
			clm_node_terminate(node);
			goto done;
		}
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv, 
					SA_CLM_CALLBACK_RESPONSE_OK);
			node->clm_pend_inv = 0; 
			goto done;
		}
	}

	if (change == SA_CLM_NODE_SHUTDOWN) {
		/* call with NULL invocation to differentiate it with AMF node shutdown. */
		avd_node_admin_lock_unlock_shutdown(node, 0, SA_AMF_ADMIN_SHUTDOWN); 
	}
	else {/* SA_CLM_NODE_LEFT case */
		avd_node_admin_lock_unlock_shutdown(node, 0, SA_AMF_ADMIN_LOCK);
	}

	if (node->su_cnt_admin_oper == 0 && node->clm_pend_inv != 0) {
		clm_node_terminate(node);
	}
	/* else wait for the su's on this node that are undergoing the required operation */
done:
	TRACE_LEAVE();
}

static void clm_node_exit_complete(SaClmNodeIdT nodeId)
{
	AVD_AVND *node = avd_node_find_nodeid(nodeId);

	TRACE_ENTER2("%x", nodeId);

	if (node == NULL) {
		LOG_IN("Node %x left but is not an AMF cluster member", nodeId);
		goto done;
	}

	node->node_info.member = SA_FALSE;

	if  (((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)
		|| (AVD_AVND_STATE_SHUTTING_DOWN == node->node_state)) && (nodeId == avd_cb->node_id_avd)) {
	
		TRACE("Invalid node state %u", node->node_state);
		goto done;
        }

	avd_node_failover(node);

done:
	TRACE_LEAVE();
}

static void clm_track_cb(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
	SaUint32T numberOfMembers, SaInvocationT invocation,
	const SaNameT *rootCauseEntity, const SaNtfCorrelationIdsT *correlationIds,
	SaClmChangeStepT step, SaTimeT timeSupervision, SaAisErrorT error)
{
	int i;
	SaClmClusterNotificationT_4 *notifItem;
	AVD_AVND *node;

	TRACE_ENTER2("'%llu' '%u' '%u'", invocation, step, error);

	if (error != SA_AIS_OK) {
		LOG_ER("ClmTrackCallback received in error");
		goto done;
	}

	/*
	** The CLM cluster can be larger than the AMF cluster thus it is not an
	** error if the corresponding AMF node cannot be found.
	*/
	for (i = 0; i < notificationBuffer->numberOfItems; i++)
	{
		notifItem = &notificationBuffer->notification[i];
		switch(step) {
		case SA_CLM_CHANGE_VALIDATE:
			if(notifItem->clusterChange == SA_CLM_NODE_LEFT) {
				node = avd_node_find_nodeid(notifItem->clusterNode.nodeId);
				if (node == NULL) {
					LOG_IN("%s: CLM node '%s' is not an AMF cluster member",
						   __FUNCTION__, notifItem->clusterNode.nodeName.value);
					goto done;
				}
				/* store the invocation for clm response */
				node->clm_pend_inv = invocation;
				/* Validate the node leaving cluster if not acceptable then
				   reject it through saClmResponse_4 */
				clm_node_exit_validate(node);
			}
			else /* All other Cluster changes are not valid in this step */
				LOG_ER("Invalid Cluster change in VALIDATE step");
			break;

		case SA_CLM_CHANGE_START:
			node = avd_node_find_nodeid(notifItem->clusterNode.nodeId);
			if (node == NULL) {
				LOG_IN("%s: CLM node '%s' is not an AMF cluster member",
					   __FUNCTION__, notifItem->clusterNode.nodeName.value);
				goto done;
			}
			if ( notifItem->clusterChange == SA_CLM_NODE_LEFT ||
			     notifItem->clusterChange == SA_CLM_NODE_SHUTDOWN ) {
				/* invocation to be used by pending clm response */ 
				node->clm_pend_inv = invocation;
				clm_node_exit_start(node, notifItem->clusterChange);
				if (strncmp((char *)rootCauseEntity->value, "safNode=", 8) == 0) {
					/* We need to differentiate between CLM lock and error scenario.*/
					node->clm_change_start_preceded = true;
				}
			}
			else
				LOG_ER("Invalid Cluster change in START step");
			break;

		case SA_CLM_CHANGE_COMPLETED:
			if( (notifItem->clusterChange == SA_CLM_NODE_LEFT)||
			    (notifItem->clusterChange == SA_CLM_NODE_SHUTDOWN)) {
				node = avd_node_find_nodeid(notifItem->clusterNode.nodeId);
				if (node == NULL) {
					LOG_IN("%s: CLM node '%s' is not an AMF cluster member",
						   __FUNCTION__, notifItem->clusterNode.nodeName.value);
					goto done;
				}
				TRACE(" Node Left: rootCauseEntity %s for node %u", rootCauseEntity->value, 
						notifItem->clusterNode.nodeId);
				if(strncmp((char *)rootCauseEntity->value, "safEE=", 6) == 0) {
					/* This callback is because of operation on PLM, so we need to mark the node
					   absent, because PLCD will anyway call opensafd stop.*/
					clm_node_exit_complete(notifItem->clusterNode.nodeId);
				} else if (strncmp((char *)rootCauseEntity->value, "safNode=", 8) == 0) {
					/* This callback is because of operation on CLM.*/
					if(true == node->clm_change_start_preceded) {
						/* We have got a completed callback with start cbk step before, so 
						   already locking applications might have been done. So, no action
						   is needed.*/
						node->clm_change_start_preceded = false; 
						node->node_info.member = SA_FALSE;
					}
					else
					{
						/* We have encountered a completed callback without start step, there
						   seems error condition, node would have gone down suddenly. */
						clm_node_exit_complete(notifItem->clusterNode.nodeId);
					}


				}
				else {
					/* We shouldn't get into this situation.*/
					LOG_ER("Wrong rootCauseEntity %s",rootCauseEntity->value);
					osafassert(0);
				}
			}
			else if(notifItem->clusterChange == SA_CLM_NODE_RECONFIGURED) {
				/* delete, reconfigure, re-add to the node-id db
				   incase if the node id has changed */
				node = avd_node_find_nodeid(notifItem->clusterNode.nodeId);
				if (node == NULL) {
					LOG_NO("Node not a member");
					goto done;
				}
				avd_node_delete_nodeid(node);
				memcpy(&(node->node_info), &(notifItem->clusterNode),
						sizeof(SaClmClusterNodeT_4));
				avd_node_add_nodeid(node);
			}
			else if(notifItem->clusterChange == SA_CLM_NODE_JOINED || 
			        notifItem->clusterChange == SA_CLM_NODE_NO_CHANGE) {
				/* This may be due to track flags set to 
				   SA_TRACK_CURRENT|CHANGES_ONLY and supply no buffer
				   in saClmClusterTrack call so update the local database */
				/* get the first node */
				node = avd_node_getnext(NULL);
				while (node != NULL && 
					0 != strncmp((char*)node->saAmfNodeClmNode.value,
						(char*)notifItem->clusterNode.nodeName.value,
						notifItem->clusterNode.nodeName.length))
				{
					node = avd_node_getnext(&node->name);
				}
				if ( node != NULL ) {
					memcpy(&(node->node_info), &(notifItem->clusterNode),
							sizeof(SaClmClusterNodeT_4));
					avd_node_add_nodeid(node);
					TRACE("Node Joined '%s' '%x'", 
						notifItem->clusterNode.nodeName.value,
						notifItem->clusterNode.nodeName.length);

					node->node_info.member = SA_TRUE;
					if (avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4) {
						m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);
					}
					else {
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);
					}
					if (node->node_state == AVD_AVND_STATE_PRESENT) {
						TRACE("Node already up and configured");
						/* now try to instantiate all the SUs that need to be */
						clm_node_join_complete(node);
					}
				} else {
					LOG_IN("AMF-node not configured on this CLM-node '%s'",
							notifItem->clusterNode.nodeName.value);
				}
			}
			else
				LOG_NO("clmTrackCallback in COMPLETED::UNLOCK");
			break;

		case SA_CLM_CHANGE_ABORTED:
			/* Do nothing */
			LOG_NO("ClmTrackCallback ABORTED_STEP" );
			break;
		default:
			osafassert(0);
		}
	}
done:
	TRACE_LEAVE();
}

static const SaClmCallbacksT_4 clm_callbacks = {
	.saClmClusterTrackCallback = clm_track_cb
};

SaAisErrorT avd_clm_init(void)
{
        SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();
	error = saClmInitialize_4(&avd_cb->clmHandle, &clm_callbacks, &clmVersion);
	if (SA_AIS_OK != error) {
		LOG_ER("Failed to initialize with CLM %u", error);
		goto done;
	}
	error = saClmSelectionObjectGet(avd_cb->clmHandle, &avd_cb->clm_sel_obj);
	if (SA_AIS_OK != error) { 
		LOG_ER("Failed to get selection object from CLM %u", error);
		goto done;
	}

	TRACE("Successfully initialized CLM");

done:
	TRACE_LEAVE();
	return error;
}

SaAisErrorT avd_clm_track_start(void)
{
        SaAisErrorT error = SA_AIS_OK;
	SaUint8T trackFlags = SA_TRACK_CURRENT|SA_TRACK_CHANGES_ONLY|SA_TRACK_VALIDATE_STEP|SA_TRACK_START_STEP; 
        
	TRACE_ENTER();
	error = saClmClusterTrack_4(avd_cb->clmHandle, trackFlags, NULL);
        if (SA_AIS_OK != error)
                LOG_ER("Failed to start cluster tracking %u", error);

        TRACE_LEAVE();
	return error;
}

SaAisErrorT avd_clm_track_stop(void)
{
        SaAisErrorT error = SA_AIS_OK;

        TRACE_ENTER();
	error = saClmClusterTrackStop(avd_cb->clmHandle);
        if (SA_AIS_OK != error) 
                LOG_ER("Failed to stop cluster tracking %u", error);
	else
		TRACE("Sucessfully stops cluster tracking");

        TRACE_LEAVE();
        return error;
}

void clm_node_terminate(AVD_AVND *node)
{
	SaAisErrorT rc=SA_AIS_OK;

	if (NCSCC_RC_SUCCESS != avd_node_admin_lock_instantiation(node)) { 
		rc = saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_ERROR);
		LOG_NO("Pending Response sent for CLM track callback::ERROR '%u'", rc);
		node->clm_pend_inv = 0;
	} /* NCSCC_RC_SUCCESS means that either you had no SUs terminate or you sent
	     all the SUs on this node the termination message */
	else if (node->su_cnt_admin_oper == 0) { 
		rc = saClmResponse_4(avd_cb->clmHandle, node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_OK);
		LOG_NO("Pending Response sent for CLM track callback::OK '%u'", rc);
		node->clm_pend_inv = 0;
	}
	else
		TRACE("Waiting for the pending SU presence state updates");
}

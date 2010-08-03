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

#include "clms.h"

/* have to do error handling in callback*/
static void clms_plm_readiness_track_callback(SaPlmEntityGroupHandleT entityGrpHdl,
					      SaUint64T trackCookie,
					      SaInvocationT invocation,
					      SaPlmTrackCauseT cause,
					      const SaNameT *rootCauseEntity,
					      SaNtfIdentifierT rootCorrelationId,
					      const SaPlmReadinessTrackedEntitiesT * trackedEntities,
					      SaPlmChangeStepT step, SaAisErrorT error)
{
	SaUint32T i, rc = SA_AIS_OK;
	SaAisErrorT ais_er;
	CLMS_CLUSTER_NODE *node = NULL, *tmp_node = NULL;
	uns32 node_id = 0;

	TRACE_ENTER2("step=%d,error=%d,number of tracked entites %d", step, error, trackedEntities->numberOfEntities);

	if (clms_cb->ha_state == SA_AMF_HA_STANDBY){
		if ((step == SA_PLM_CHANGE_VALIDATE)||(step == SA_PLM_CHANGE_START)){
			ais_er = saPlmReadinessTrackResponse(clms_cb->ent_group_hdl, invocation, SA_PLM_CALLBACK_RESPONSE_OK);
			if (ais_er != SA_AIS_OK) {
				TRACE("saPlmReadinessTrackResponse FAILED");
				goto done;
			}

		}
		for (i = 0; i < trackedEntities->numberOfEntities; i++) {
			node = clms_node_get_by_eename(&trackedEntities->entities[i].entityName);
			if (node == NULL) {
				TRACE("node not in database");
				assert(0);
			}
			node->ee_red_state = trackedEntities->entities[i].currentReadinessStatus.readinessState;
		}

		goto done;
	}

	/*Check for the expectedReadinessStatus of the entityName */
	if( (step == SA_PLM_CHANGE_VALIDATE) || (step == SA_PLM_CHANGE_START) || (step == SA_PLM_CHANGE_ABORTED)) {

		for (i = 0; i < trackedEntities->numberOfEntities; i++) {

			node = clms_node_get_by_eename(&trackedEntities->entities[i].entityName);

			if (node == NULL) {
				TRACE("node not in database");
				assert(0);
			}
			/* clearing node dependency list for each entity coming in the callback
			   in case of failover,after validate step completed will come and we need to clear node list
			   as we dont no the order of entity coming from plm, better to remove dependency list on each node */
			clms_clear_node_dep_list(node);

			if (node->nodeup &&
			    trackedEntities->entities[i].expectedReadinessStatus.readinessState ==
			    SA_PLM_READINESS_OUT_OF_SERVICE) {

				if (node->member == SA_TRUE) {	/*Self node will be done during init */
					node->stat_change = SA_TRUE;
					node->admin_op = PLM;
					node->change = SA_CLM_NODE_LEFT;
					
					if (step == SA_PLM_CHANGE_START){
						rc = clms_chk_sub_exist(SA_TRACK_START_STEP);
					}else if (step == SA_PLM_CHANGE_VALIDATE) {
						rc = clms_chk_sub_exist(SA_TRACK_VALIDATE_STEP);
					}

					if (rc != NCSCC_RC_SUCCESS){
						TRACE("No clients exists for validate/start step");
						ais_er = saPlmReadinessTrackResponse(clms_cb->ent_group_hdl, invocation,SA_PLM_CALLBACK_RESPONSE_OK);
						if (ais_er != SA_AIS_OK) {
							TRACE("saPlmReadinessTrackResponse FAILED");
							goto done;
						}
						TRACE("PLM Track Response Send Succedeed");
					}
				}
			}
			/* Checkpoint node data */
			ckpt_node_rec(node);
		}
		node = clms_node_get_by_eename(&trackedEntities->entities[0].entityName);
		node->admin_op = PLM;
		node->plm_invid = invocation;

	}else if (step == SA_PLM_CHANGE_COMPLETED) {

		for (i = 0; i < trackedEntities->numberOfEntities; i++) {
			TRACE("Entity Name %s", trackedEntities->entities[i].entityName.value);
			TRACE("Entity Readiness State %d",
			      trackedEntities->entities[i].currentReadinessStatus.readinessState);

			node = clms_node_get_by_eename(&trackedEntities->entities[i].entityName);

			if (cause == SA_PLM_CAUSE_GROUP_CHANGE){
				if (trackedEntities->entities[i].change == SA_PLM_GROUP_MEMBER_REMOVED){
					if (node == NULL)
						TRACE("Node removed from PLM Group,callback received for entity removal");
						goto done;
				}
			} else {
				if (node == NULL){
					TRACE("node not in database");
					assert(0);
				}
			}

			TRACE("nodeup %d", node->nodeup);

			node->plm_invid = 0;	/* No resp */
			node->ee_red_state = trackedEntities->entities[i].currentReadinessStatus.readinessState;

			if(node->nodeup) {
				if(trackedEntities->entities[i].currentReadinessStatus.readinessState  ==
						SA_PLM_READINESS_OUT_OF_SERVICE) {

					if (node->member == SA_TRUE) {
						node->admin_op = PLM;
						node->stat_change = SA_TRUE;

						node->change = SA_CLM_NODE_LEFT;
						++(clms_cb->cluster_view_num);
						--(osaf_cluster->num_nodes);
						node->member = SA_FALSE;
						rc = clms_node_exit_ntf(clms_cb, node);
						if (rc != SA_AIS_OK) {
							TRACE("clms_node_exit_ntf failed %u", rc);
							/*goto done; */
						}
					
						rc = clms_send_is_member_info(clms_cb, node->node_id, node->member, TRUE);
						if (rc != NCSCC_RC_SUCCESS) {
							TRACE("clms_send_is_member_info failed %u", rc);
							goto done;
						}

						/*Update the admin_state change to IMMSv */
						clms_admin_state_update_rattr(node);
					} else {
						node->change = SA_CLM_NODE_NO_CHANGE;
					}

					clms_node_update_rattr(node);
					/* Checkpoint node and cluster data */
					ckpt_node_rec(node);
					ckpt_cluster_rec();

				}else if(trackedEntities->entities[i].currentReadinessStatus.readinessState ==
						SA_PLM_READINESS_IN_SERVICE) {

					if (node->admin_state == SA_CLM_ADMIN_UNLOCKED){
						if (node->member == SA_FALSE) {
							node->admin_op = PLM;
							node->stat_change = SA_TRUE;
							++(osaf_cluster->num_nodes);
							node->init_view = (++(clms_cb->cluster_view_num));
							node->member = SA_TRUE;
							node->change = SA_CLM_NODE_JOINED;
							rc = clms_node_join_ntf(clms_cb, node);
							if (rc != SA_AIS_OK) {
								TRACE("clms_node_join_ntf failed %u", rc);
								/*goto done; */
							}

							rc = clms_send_is_member_info(clms_cb, node->node_id, node->member, TRUE);
							if (rc != NCSCC_RC_SUCCESS) {
								TRACE("clms_send_is_member_info failed %u", rc);
								goto done;
							}

							/*Update the admin_state change to IMMSv */
							clms_admin_state_update_rattr(node);
						} else {
							node->change = SA_CLM_NODE_NO_CHANGE;
						}
						clms_node_update_rattr(node);
						/* Checkpoint node and cluster data */
						ckpt_node_rec(node);
						ckpt_cluster_rec();
					}
				}
			}
		}

	}

	node = clms_node_get_by_eename(&trackedEntities->entities[0].entityName);
	/*this dependency list will be cleared in clms_send_track() */
	while ((tmp_node = clms_node_getnext_by_id(node_id)) != NULL) {
		node_id = tmp_node->node_id;

		if ((tmp_node->stat_change == SA_TRUE) && (tmp_node != node)) {
			if (node->dep_node_list == NULL)
				node->dep_node_list = tmp_node;
			else {
				tmp_node->next = node->dep_node_list;
				node->dep_node_list = tmp_node;
			}
		}
	}

	if(node->nodeup) {
		if((node->member == SA_FALSE) && (node->admin_state == SA_CLM_ADMIN_LOCKED)) {
			if(step == SA_PLM_CHANGE_START) {
				ais_er = saPlmReadinessTrackResponse(clms_cb->ent_group_hdl, invocation, SA_PLM_CALLBACK_RESPONSE_OK);
				if (ais_er != SA_AIS_OK) {
					TRACE("saPlmReadinessTrackResponse FAILED");
					goto done;
				}
			}/*In completed step we don't need to send Response*/
		} else {
			if (node->admin_state == SA_CLM_ADMIN_UNLOCKED){
				if (step == SA_PLM_CHANGE_COMPLETED){
					clms_cluster_update_rattr(osaf_cluster);
				}
				clms_send_track(clms_cb, node, step);	/*dude you need to checkpoint admin_op admin_state when track is complete or not decide */
			}
		}
	}

	/* Clear admin_op and stat_change for the completed step */
	if ((step == SA_PLM_CHANGE_COMPLETED) || (step == SA_PLM_CHANGE_ABORTED)){
		clms_clear_node_dep_list(node);
	}

 done:
	TRACE_LEAVE();
}

static const SaPlmCallbacksT callbacks = {
	.saPlmReadinessTrackCallback = clms_plm_readiness_track_callback
};

SaAisErrorT clms_plm_init(CLMS_CB * cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaVersionT plmVersion = { 'A', 0x01, 0x01 };
	SaNameT *entityNames = NULL;
	CLMS_CLUSTER_NODE *node = NULL;
	SaNameT nodename;
	SaUint32T i = 0, entityNamesNumber = ncs_patricia_tree_size(&clms_cb->ee_lookup);
	SaPlmReadinessTrackedEntitiesT *trackedEntities = NULL;

	TRACE_ENTER();

	rc = saPlmInitialize(&cb->plm_hdl, &callbacks, &plmVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("saPlmInitialize FAILED rc = %d", rc);
		return rc;
	}

	rc = saPlmSelectionObjectGet(cb->plm_hdl, &cb->plm_sel_obj);
	if (rc != SA_AIS_OK) {
		LOG_ER("saPlmSelectionObjectGet FAILED rc = %d", rc);
		return rc;
	}
	
	if (clms_cb->reg_with_plm == SA_TRUE){
	
		rc = saPlmEntityGroupCreate(cb->plm_hdl, &cb->ent_group_hdl);
		if (rc != SA_AIS_OK) {
			LOG_ER("saPlmEntityGroupCreate FAILED rc = %d", rc);
			return rc;
		}

		memset(&nodename, '\0', sizeof(SaNameT));
		entityNames = (SaNameT *)malloc(sizeof(SaNameT) * entityNamesNumber);

		if (!entityNames) {
			LOG_ER("Malloc failed for entityNames");
			assert(0);
		}

		TRACE("entityNamesNumber %d", entityNamesNumber);

		while ((node = clms_node_getnext_by_name(&nodename)) != NULL) {
			memcpy(&nodename, &node->node_name, sizeof(SaNameT));
			if (node->ee_name.length != 0){
				entityNames[i].length = node->ee_name.length;
				(void)memcpy(entityNames[i].value, node->ee_name.value, entityNames[i].length);
				i++;
			}
		}

		rc = saPlmEntityGroupAdd(cb->ent_group_hdl, entityNames, entityNamesNumber, SA_PLM_GROUP_SINGLE_ENTITY);

		if (rc != SA_AIS_OK) {
			LOG_ER("saPlmEntityGroupAdd FAILED rc = %d", rc);
			return rc;
		}

		trackedEntities = (SaPlmReadinessTrackedEntitiesT *)
			malloc(entityNamesNumber * sizeof(SaPlmReadinessTrackedEntitiesT));

		if(!trackedEntities) {
			LOG_ER("Malloc failed for trackedEntities");
			assert(0);
		}

		memset(trackedEntities, 0, (entityNamesNumber * sizeof(SaPlmReadinessTrackedEntitiesT)));

		rc = saPlmReadinessTrack(cb->ent_group_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY | SA_TRACK_START_STEP | SA_TRACK_VALIDATE_STEP), 1, trackedEntities);	/* trackCookie TBD */

		if (rc != SA_AIS_OK) {
			LOG_ER("saPlmReadinessTrack FAILED rc = %d", rc);
			return rc;
		}

		TRACE("trackedEntities->numberOfEntities %d", trackedEntities->numberOfEntities);

		for (i = 0; i < trackedEntities->numberOfEntities; i++) {
			node = clms_node_get_by_eename(&trackedEntities->entities[i].entityName);
			if (node != NULL) {
				node->ee_red_state = trackedEntities->entities[i].currentReadinessStatus.readinessState;
				node->change = trackedEntities->entities[i].change;
				TRACE("node->ee_red_state %d", node->ee_red_state);
			}

		}
	}

	free(entityNames);
	free(trackedEntities);

	TRACE_LEAVE();

	return rc;
}

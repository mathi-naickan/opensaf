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

  This file contains cluster membership routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <logtrace.h>
#include <immutil.h>
#include "avnd.h"
#include "mds_pvt.h"
#include "nid_api.h"
#include "amf_si_assign.h"
#include "osaf_time.h"

#include <string>


static void clm_node_left(SaClmNodeIdT node_id)
{
	AVND_COMP *comp = nullptr;
	uint32_t rc;
	AVND_COMP_PXIED_REC *pxd_rec = 0, *curr_rec = 0;
	std::string name = "";

	TRACE_ENTER2("%u", node_id);

	if(node_id == avnd_cb->node_info.nodeId) {
	/* if you received a node left indication from clm for self node
	   terminate all the non_ncs components; ncs components :-TBD */
	   
		LOG_NO("This node has exited the cluster");
		avnd_cb->node_info.member = SA_FALSE;
		for (comp = avnd_compdb_rec_get_next(avnd_cb->compdb, "");
			 comp != nullptr;
			 comp = avnd_compdb_rec_get_next(avnd_cb->compdb, comp->name)) {
			if(comp->su->is_ncs != true ) {
				avnd_comp_clc_fsm_run(avnd_cb, comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
			}
		}
		goto done;
	}

	/* We received a node left indication for external node, 
	   check if any internode component related to this Node exists */
	for (comp = avnd_compdb_rec_get_next(avnd_cb->internode_avail_comp_db, name);
		 comp != nullptr;
		 comp = avnd_compdb_rec_get_next(avnd_cb->internode_avail_comp_db, name)) {
		 name = comp->name;
		/* Check the node id */
		if (comp->node_id == node_id) {
			if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
				/* We need to delete the proxied component and remove
				   this component from the proxy's pxied_list */
				/* Remove the proxied component from pxied_list  */
				avnd_comp_proxied_del(avnd_cb, comp, comp->pxy_comp, false, nullptr);
				/* Delete the proxied component */
				avnd_internode_comp_del(avnd_cb, comp->name);
			} /* if(m_AVND_COMP_TYPE_IS_PROXIED(comp)) */
			else if (m_AVND_COMP_TYPE_IS_PROXY(comp)) {
				/* We need to delete the proxy component and make
				   all its proxied component as orphan */
				/* look in each proxied for this handle */
				pxd_rec =(AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->pxied_list);
				while (pxd_rec) {
					curr_rec = pxd_rec;
					pxd_rec =(AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_NEXT(&pxd_rec->comp_dll_node);
					rc = avnd_comp_unreg_prc(avnd_cb, curr_rec->pxied_comp, comp);

					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("avnd_evt_avd_node_update_msg:Unreg failed: %s", curr_rec->pxied_comp->name.c_str());
					}
				}	/* while */
			}	/* else if(m_AVND_COMP_TYPE_IS_PROXY(comp)) */
		}	/* if(comp->node_id == rec->info.node_id)  */
	}	/* for(comp = m_AVND_COMPDB_REC_GET_NEXT()  */
done:
	TRACE_LEAVE();
}

static void clm_to_amf_node(void)
{
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaNameT amfdn, clmdn;
	SaImmSearchParametersT_2 searchParam;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfNode";
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = { 'A', 2, 15 };

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = saImmOmInitialize_cond(&immOmHandle, nullptr, &immVersion);
	if (SA_AIS_OK != error) {
		LOG_CR("saImmOmInitialize failed. Use previous value of nodeName.");
		osafassert(avnd_cb->amf_nodeName.empty() == false);
		goto done1;
	}

	error = amf_saImmOmSearchInitialize_o2(immOmHandle, "", SA_IMM_SUBTREE,
					SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
					&searchParam, nullptr, searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("No objects found");
		goto done;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &amfdn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeClmNode"), attributes, 0, &clmdn) == SA_AIS_OK) {
			std::string clm_node(Amf::to_string(&clmdn)); 

			if (clm_node.compare(osaf_extended_name_borrow(&avnd_cb->node_info.nodeName)) == 0) {
				avnd_cb->amf_nodeName = Amf::to_string(&amfdn);
				break;
			}
		} 
	}

done:
	immutil_saImmOmSearchFinalize(searchHandle);
	immutil_saImmOmFinalize(immOmHandle);
done1:
	TRACE_LEAVE2("%u", error);
}

/****************************************************************************
  Name          : avnd_evt_avd_node_up_msg
 
  Description   : This routine processes the node-up response message from 
                  AvD. It contains a list of current nodes in the cluster.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_avd_node_up_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_NODE_UP_MSG_INFO *info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	info = &evt->info.avd->msg_info.d2n_node_up;

	/*** update this node with the supplied parameters ***/
	cb->su_failover_max = info->su_failover_max;
	cb->su_failover_prob = info->su_failover_prob;

	TRACE_LEAVE();
	return rc;
}

static void clm_track_cb(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
        SaUint32T numberOfMembers, SaInvocationT invocation,
        const SaNameT *rootCauseEntity, const SaNtfCorrelationIdsT *correlationIds,
        SaClmChangeStepT step, SaTimeT timeSupervision, SaAisErrorT error)
{
	SaClmClusterNotificationT_4 *notifItem;
	uint32_t i;
	TRACE_ENTER2("'%llu' '%u' '%u'",invocation, step, error);

	if (error != SA_AIS_OK) {
		LOG_ER("ClmTrackCallback received in error");
		goto done;
	}

	if(step != SA_CLM_CHANGE_COMPLETED) {
		LOG_ER("ClmTrackCallback received in %d step", step);
		goto done;
	}

        for (i = 0; i < notificationBuffer->numberOfItems; i++)
        {
                notifItem = &notificationBuffer->notification[i];
		if( (notifItem->clusterChange == SA_CLM_NODE_LEFT)||
		    (notifItem->clusterChange == SA_CLM_NODE_SHUTDOWN)) {
                      
			TRACE("Node has left the cluster '%s', avnd_cb->first_time_up %u," 
					"notifItem->clusterNode.nodeId %u, avnd_cb->node_info.nodeId %u",
					osaf_extended_name_borrow(&notifItem->clusterNode.nodeName), avnd_cb->first_time_up,
					notifItem->clusterNode.nodeId, avnd_cb->node_info.nodeId);
			if(false == avnd_cb->first_time_up) {
				/* When node reboots, we will get an exit cbk, so ignore if avnd_cb->first_time_up
				   is false. */
				if(notifItem->clusterNode.nodeId == avnd_cb->node_info.nodeId) {
					clm_node_left(notifItem->clusterNode.nodeId);	
				}
			}

		}
		else if(notifItem->clusterChange == SA_CLM_NODE_RECONFIGURED) {
			if (avnd_cb->node_info.nodeId == notifItem->clusterNode.nodeId) {
				TRACE("local CLM node reconfigured");
				/* update the local node info */
				memcpy(&(avnd_cb->node_info), &(notifItem->clusterNode),
					sizeof(SaClmClusterNodeT_4));
			}
		}
		else if(notifItem->clusterChange == SA_CLM_NODE_JOINED  ||
		        notifItem->clusterChange == SA_CLM_NODE_NO_CHANGE) {
			TRACE("Node has joined the Cluster '%s'",
					 osaf_extended_name_borrow(&notifItem->clusterNode.nodeName));
			/* This may be also due to track flags set to 
			 SA_TRACK_CURRENT|CHANGES_ONLY and supply no buffer
			 in saClmClusterTrack call so update the local database */
			if (notifItem->clusterNode.nodeId == m_NCS_NODE_ID_FROM_MDS_DEST(avnd_cb->avnd_dest)) {
				if (avnd_cb->first_time_up == true) { 
					/* store the local node info */
					memcpy(&(avnd_cb->node_info),
					       &(notifItem->clusterNode),
						sizeof(SaClmClusterNodeT_4));
					/*get the amf node from clm node name */
					clm_to_amf_node();
					avnd_send_node_up_msg();
					avnd_cb->first_time_up = false;
				}
			}
		}
		else
			TRACE("clm_track_cb in COMPLETED::UNLOCK");
	}

done:
	TRACE_LEAVE();
	return;
}

static const SaClmCallbacksT_4 callbacks = {
        0,
        /*.saClmClusterTrackCallback =*/ clm_track_cb
};

SaAisErrorT avnd_clm_init(AVND_CB* cb)
{
        SaAisErrorT error = SA_AIS_OK;
        SaUint8T trackFlags = SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY;

	TRACE_ENTER();

	cb->first_time_up = true;
	cb->clmHandle = 0;
	for (;;) {
		SaVersionT Version = { 'B', 4, 1 };
		error = saClmInitialize_4(&cb->clmHandle, &callbacks, &Version);
		if (error == SA_AIS_ERR_TRY_AGAIN ||
		    error == SA_AIS_ERR_TIMEOUT ||
                    error == SA_AIS_ERR_UNAVAILABLE) {
			if (error != SA_AIS_ERR_TRY_AGAIN) {
				LOG_WA("saClmInitialize_4 returned %u",
				       (unsigned) error);
			}
			osaf_nanosleep(&kHundredMilliseconds);
			continue;
		}
		if (error == SA_AIS_OK) break;
		LOG_ER("Failed to Initialize with CLM: %u", error);
		goto done;
	}
	error = saClmSelectionObjectGet(cb->clmHandle, &cb->clm_sel_obj);
	if (SA_AIS_OK != error) {
		LOG_ER("Failed to get CLM selectionObject: %u", error);
		goto done;
	}
	error = saClmClusterTrack_4(cb->clmHandle, trackFlags, nullptr);
	if (SA_AIS_OK != error) {
		LOG_ER("Failed to start cluster tracking: %u", error);
		goto done;
	}

done:
	TRACE_LEAVE();
        return error;
}

static void* avnd_clm_init_thread(void* arg)
{
	TRACE_ENTER();
	AVND_CB* cb = static_cast<AVND_CB*>(arg);

	avnd_clm_init(cb);

	TRACE_LEAVE();
	return nullptr;
}

SaAisErrorT avnd_start_clm_init_bg()
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avnd_clm_init_thread, avnd_cb)
	    != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	pthread_attr_destroy(&attr);

	return SA_AIS_OK;
}

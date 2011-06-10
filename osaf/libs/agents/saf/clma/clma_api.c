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

/*****************************************************************************
..............................................................................

  DESCRIPTION:

  This file contains the CLMSv API implementation.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "clma.h"
#include "ncs_main_papi.h"

#define CLMS_WAIT_TIME 1000
#define CLM_API_MIN_TIMEOUT 10 /* ten milli seconds */

static SaAisErrorT clmainitialize(SaClmHandleT *clmHandle, const SaClmCallbacksT *reg_cbks_1,
	const SaClmCallbacksT_4 *reg_cbks_4, SaVersionT *version);
static SaAisErrorT clmaclustertrack(SaClmHandleT clmHandle, SaUint8T flags,
	SaClmClusterNotificationBufferT *buf,
	SaClmClusterNotificationBufferT_4 *buf_4);
static SaAisErrorT clmaclusternodeget(SaClmHandleT clmHandle,
	SaClmNodeIdT node_id, SaTimeT timeout, SaClmClusterNodeT *cluster_node,
	SaClmClusterNodeT_4 *cluster_node_4);

/* Macro to validate the dispatch flags */
#define m_DISPATCH_FLAG_IS_VALID(flag) \
   ( (SA_DISPATCH_ONE == flag) || \
     (SA_DISPATCH_ALL == flag) || \
     (SA_DISPATCH_BLOCKING == flag) )

/* The main controle block */
clma_cb_t clma_cb = {
	.cb_lock = PTHREAD_MUTEX_INITIALIZER,
};

/* Macro for Verifying the input Handle & global handle */
#define m_CLA_API_HDL_VERIFY(cbhdl, hdl, o_rc) \
{ \
   /* is library Initialized && handle a 32 bit value*/ \
   if(!(cbhdl) || (hdl) > AVSV_UNS32_HDL_MAX) \
      (o_rc) = SA_AIS_ERR_BAD_HANDLE;\
};

static SaAisErrorT clma_validate_flags_buf(clma_client_hdl_rec_t * hdl_rec, SaUint8T flags,
					   SaClmClusterNotificationBufferT *buf);
static SaAisErrorT clma_validate_flags_buf_4(clma_client_hdl_rec_t * hdl_rec, SaUint8T flags,
					     SaClmClusterNotificationBufferT_4 * buf);
static SaAisErrorT clma_fill_cluster_ntf_buf_from_omsg(SaClmClusterNotificationBufferT *buf, CLMSV_MSG * msg_rsp);
static SaAisErrorT clma_fill_cluster_ntf_buf4_from_omsg(SaClmClusterNotificationBufferT_4 * buf_4, CLMSV_MSG * msg_rsp);
static SaAisErrorT clma_send_mds_msg_get_clusternotificationbuf(clma_client_hdl_rec_t * hdl_rec,
								SaUint8T flags,
								CLMSV_MSG i_msg, SaClmClusterNotificationBufferT *buf);
static SaAisErrorT clma_send_mds_msg_get_clusternotificationbuf_4(clma_client_hdl_rec_t * hdl_rec,
								  SaUint8T flags,
								  CLMSV_MSG i_msg,
								  SaClmClusterNotificationBufferT_4 * buf_4);

void clma_fill_node_from_node4(SaClmClusterNodeT *clusterNode, SaClmClusterNodeT_4 clusterNode_4)
{
	clusterNode->nodeId = clusterNode_4.nodeId;
	clusterNode->nodeAddress.family = clusterNode_4.nodeAddress.family;
	clusterNode->nodeAddress.length = clusterNode_4.nodeAddress.length;
	(void)memcpy(clusterNode->nodeAddress.value, clusterNode_4.nodeAddress.value, clusterNode->nodeAddress.length);
	clusterNode->nodeName.length = clusterNode_4.nodeName.length;
	(void)memcpy(clusterNode->nodeName.value, clusterNode_4.nodeName.value, clusterNode->nodeName.length);
	clusterNode->member = clusterNode_4.member;
	clusterNode->bootTimestamp = clusterNode_4.bootTimestamp;
	clusterNode->initialViewNumber = clusterNode_4.initialViewNumber;
}

static SaAisErrorT clma_validate_flags_buf(clma_client_hdl_rec_t * hdl_rec, SaUint8T flags,
					   SaClmClusterNotificationBufferT *buf)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	/* validate the flags */
	if (!((flags & SA_TRACK_CURRENT) || (flags & SA_TRACK_CHANGES) ||
	      (flags & SA_TRACK_CHANGES_ONLY) || (flags & SA_TRACK_LOCAL))) {
		TRACE("oneeeeeeeeeeee");
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((flags & SA_TRACK_CHANGES) && (flags & SA_TRACK_CHANGES_ONLY)) {

		TRACE("twoeeeeeeeeeeee");
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((flags & SA_TRACK_LOCAL) &&
		!((flags & SA_TRACK_CURRENT) || (flags & SA_TRACK_CHANGES) || (flags & SA_TRACK_CHANGES_ONLY))) {
		return SA_AIS_ERR_BAD_FLAGS;
	}

	/* validate the notify buffer */
	if ((flags & SA_TRACK_CURRENT) && buf && buf->notification) {
		if (!buf->numberOfItems)
			return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Validate if flag is TRACK_CURRENT and no callback and no buffer provided */
	if (((flags & SA_TRACK_CURRENT) && ((!buf) || ((buf) && !(buf->notification)))) &&
	    (!(hdl_rec->cbk_param.reg_cbk.saClmClusterTrackCallback))) {
		return SA_AIS_ERR_INIT;
	}

	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT clma_validate_flags_buf_4(clma_client_hdl_rec_t * hdl_rec, SaUint8T flags,
					     SaClmClusterNotificationBufferT_4 * buf)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("flags=0x%x", flags);

	/* validate the flags */
	if (!((flags & SA_TRACK_CURRENT) || (flags & SA_TRACK_CHANGES) ||
	      (flags & SA_TRACK_CHANGES_ONLY) || (flags & SA_TRACK_LOCAL))) {
		TRACE("1st case");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((flags & SA_TRACK_CHANGES) && (flags & SA_TRACK_CHANGES_ONLY)) {
		TRACE("2nd Case");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((flags & SA_TRACK_LOCAL) &&
		!((flags & SA_TRACK_CURRENT) || (flags & SA_TRACK_CHANGES) || (flags & SA_TRACK_CHANGES_ONLY))) {
		return SA_AIS_ERR_BAD_FLAGS;
	}

	/* validate the notify buffer */
	if ((flags & SA_TRACK_CURRENT) && buf && buf->notification) {
		if (!buf->numberOfItems) {
			TRACE_LEAVE();
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	/* Validate if flag is TRACK_CURRENT and no callback and no buffer provided */
	if ((flags & SA_TRACK_CURRENT) && ((!buf) || ((buf) && !(buf->notification))) &&
	    (!(hdl_rec->cbk_param.reg_cbk_4.saClmClusterTrackCallback))) {
		TRACE_LEAVE();
		return SA_AIS_ERR_INIT;
	}

	TRACE_LEAVE();
	return rc;
}

/* Copy the cluster node info into buf 
* 
* Perform the sanity check whether sufficient memory is supplied 
* by buf pointer before invoking the callback.
*
* Check the number of items and num fields.
*/

static SaAisErrorT clma_fill_cluster_ntf_buf_from_omsg(SaClmClusterNotificationBufferT *buf, CLMSV_MSG * msg_rsp)
{
	if (msg_rsp->info.api_resp_info.param.track.notify_info == NULL)
		return SA_AIS_ERR_NO_MEMORY;

	if (buf->notification != NULL &&
	    (buf->numberOfItems >= msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems)) {
		/* Overwrite the numberOfItems and copy it to buffer */
		buf->numberOfItems = msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems;
		buf->viewNumber = msg_rsp->info.api_resp_info.param.track.notify_info->viewNumber;

		memset(buf->notification, 0, sizeof(SaClmClusterNotificationT) * buf->numberOfItems);
		clma_fill_clusterbuf_from_buf_4(buf, msg_rsp->info.api_resp_info.param.track.notify_info);
	}else if(buf->notification != NULL &&
		(buf->numberOfItems < msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems)) {
		return SA_AIS_ERR_NO_SPACE;
	} else {
		/* we need to ignore the numberOfItems and allocate the space
		 ** This memory will be freed by the application */
		buf->numberOfItems = msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems;
		buf->viewNumber = msg_rsp->info.api_resp_info.param.track.notify_info->viewNumber;
		buf->notification =
		    (SaClmClusterNotificationT *)malloc(sizeof(SaClmClusterNotificationT) * buf->numberOfItems);
		memset(buf->notification, 0, sizeof(SaClmClusterNotificationT) * buf->numberOfItems);
		clma_fill_clusterbuf_from_buf_4(buf, msg_rsp->info.api_resp_info.param.track.notify_info);
	}
	return SA_AIS_OK;
}

/* Copy the cluster node info into buf 
* 
* Perform the sanity check whether sufficient memory is supplied 
* by buf pointer before invoking the callback.
*
* Check the number of items and num fields.
*/

static SaAisErrorT clma_fill_cluster_ntf_buf4_from_omsg(SaClmClusterNotificationBufferT_4 * buf_4, CLMSV_MSG * msg_rsp)
{
	if (msg_rsp->info.api_resp_info.param.track.notify_info == NULL)
		return SA_AIS_ERR_NO_MEMORY;

	if (buf_4->notification != NULL &&
	    (buf_4->numberOfItems >= msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems)) {
		/* Overwrite the numberOfItems and copy it to buffer */
		buf_4->numberOfItems = msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems;
		buf_4->viewNumber = msg_rsp->info.api_resp_info.param.track.notify_info->viewNumber;

		memset(buf_4->notification, 0, sizeof(SaClmClusterNotificationT_4) * buf_4->numberOfItems);
		memcpy(buf_4->notification, msg_rsp->info.api_resp_info.param.track.notify_info->notification,
		       sizeof(SaClmClusterNotificationT_4) * buf_4->numberOfItems);
        } else if(buf_4->notification != NULL &&
		(buf_4->numberOfItems < msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems)) {
		return SA_AIS_ERR_NO_SPACE;
	} else {
		/* we need to ignore the numberOfItems and allocate the space
		 ** This memory will be freed by the application */
		buf_4->numberOfItems = msg_rsp->info.api_resp_info.param.track.notify_info->numberOfItems;
		buf_4->viewNumber = msg_rsp->info.api_resp_info.param.track.notify_info->viewNumber;
		buf_4->notification =
		    (SaClmClusterNotificationT_4 *) malloc(sizeof(SaClmClusterNotificationT_4) * buf_4->numberOfItems);
		memset(buf_4->notification, 0, sizeof(SaClmClusterNotificationT_4) * buf_4->numberOfItems);
		memcpy(buf_4->notification, msg_rsp->info.api_resp_info.param.track.notify_info->notification,
		       sizeof(SaClmClusterNotificationT_4) * buf_4->numberOfItems);

	}
	return SA_AIS_OK;
}

static SaAisErrorT clma_send_mds_msg_get_clusternotificationbuf(clma_client_hdl_rec_t * hdl_rec,
								SaUint8T flags,
								CLMSV_MSG i_msg, SaClmClusterNotificationBufferT *buf)
{
	SaAisErrorT rc = SA_AIS_OK;
	CLMSV_MSG *o_msg = NULL;
	uns32 mds_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (flags & SA_TRACK_CURRENT) {
		TRACE("track flag SA_TRACK_CURRENT");

		if (!buf) {
			TRACE("Tracking requested for buffer NULL");
			i_msg.info.api_info.param.track_start.sync_resp = FALSE;

			if (!hdl_rec->cbk_param.reg_cbk.saClmClusterTrackCallback) {
				rc = SA_AIS_ERR_INIT;
				goto done;
			}

			mds_rc = clma_mds_msg_async_send(&clma_cb, &i_msg, MDS_SEND_PRIORITY_HIGH);	/* fix me ?? */
			switch (mds_rc) {
			case NCSCC_RC_SUCCESS:
				rc = SA_AIS_OK;	/*let it be */
				goto done;
			case NCSCC_RC_REQ_TIMOUT:
				TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
				rc = SA_AIS_ERR_TIMEOUT;
				goto done;
			default:
				TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
				rc = SA_AIS_ERR_NO_RESOURCES;
				goto done;
			}
		}

		/* Do a sync mds send and get information about all
		 * nodes that are currently members in the cluster 
		 */
		TRACE("Tracking requested for buffer != NULL");
		i_msg.info.api_info.param.track_start.sync_resp = TRUE;
		mds_rc = clma_mds_msg_sync_send(&clma_cb, &i_msg, &o_msg, CLMS_WAIT_TIME);
		switch (mds_rc) {
		case NCSCC_RC_SUCCESS:
			rc = SA_AIS_OK;
			break;
		case NCSCC_RC_REQ_TIMOUT:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_TIMEOUT;
			goto done;
		default:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		if (o_msg != NULL)
			rc = o_msg->info.api_resp_info.rc;
		else {
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		if (rc == SA_AIS_OK) {
			rc = clma_fill_cluster_ntf_buf_from_omsg(buf, o_msg);
			/* destroy o_msg */
			clma_msg_destroy(o_msg);
			goto done;
		} else {
			/* destroy o_msg */
			clma_msg_destroy(o_msg);
			goto done;
		}

	} else {
		i_msg.info.api_info.param.track_start.sync_resp = FALSE;

		if (!hdl_rec->cbk_param.reg_cbk.saClmClusterTrackCallback) {
			rc = SA_AIS_ERR_INIT;
			goto done;
		}

		mds_rc = clma_mds_msg_async_send(&clma_cb, &i_msg, MDS_SEND_PRIORITY_HIGH);
		switch (mds_rc) {
		case NCSCC_RC_SUCCESS:
			rc = SA_AIS_OK;
			goto done;
		case NCSCC_RC_REQ_TIMOUT:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_TIMEOUT;
			goto done;
		default:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_NO_RESOURCES;
		}
	}

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT clma_send_mds_msg_get_clusternotificationbuf_4(clma_client_hdl_rec_t * hdl_rec,
								  SaUint8T flags,
								  CLMSV_MSG i_msg,
								  SaClmClusterNotificationBufferT_4 * buf_4)
{
	SaAisErrorT rc = SA_AIS_OK;
	CLMSV_MSG *o_msg = NULL;
	uns32 mds_rc = NCSCC_RC_SUCCESS;

	if (flags & SA_TRACK_CURRENT) {

		if (!buf_4) {
			i_msg.info.api_info.param.track_start.sync_resp = FALSE;

			if (!hdl_rec->cbk_param.reg_cbk_4.saClmClusterTrackCallback) {
				rc = SA_AIS_ERR_INIT;
				goto done;
			}

			mds_rc = clma_mds_msg_async_send(&clma_cb, &i_msg, MDS_SEND_PRIORITY_HIGH);
			switch (mds_rc) {
			case NCSCC_RC_SUCCESS:
				rc = SA_AIS_OK;
				goto done;
			case NCSCC_RC_REQ_TIMOUT:
				TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
				rc = SA_AIS_ERR_TIMEOUT;
				goto done;
			default:
				TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
				rc = SA_AIS_ERR_NO_RESOURCES;
				goto done;
			}
		}

		/* Do a sync mds send and get information about all
		 * nodes that are currently members in the cluster 
		 */
		i_msg.info.api_info.param.track_start.sync_resp = TRUE;
		mds_rc = clma_mds_msg_sync_send(&clma_cb, &i_msg, &o_msg, CLMS_WAIT_TIME);
		switch (mds_rc) {
		case NCSCC_RC_SUCCESS:
			rc = SA_AIS_OK;
			break;
		case NCSCC_RC_REQ_TIMOUT:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_TIMEOUT;
			goto done;
		default:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		if (o_msg != NULL)
			rc = o_msg->info.api_resp_info.rc;
		else {
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		if (rc == SA_AIS_OK) {
			rc = clma_fill_cluster_ntf_buf4_from_omsg(buf_4, o_msg);
			/* destroy o_msg */
			clma_msg_destroy(o_msg);
			goto done;
		} else {
			/* destroy o_msg */
			clma_msg_destroy(o_msg);
			goto done;
		}

	} else {
		i_msg.info.api_info.param.track_start.sync_resp = FALSE;

		if (!hdl_rec->cbk_param.reg_cbk_4.saClmClusterTrackCallback) {
			rc = SA_AIS_ERR_INIT;
			goto done;
		}

		mds_rc = clma_mds_msg_async_send(&clma_cb, &i_msg, MDS_SEND_PRIORITY_HIGH);	/* fix me ?? */
		switch (mds_rc) {
		case NCSCC_RC_SUCCESS:
			rc = SA_AIS_OK;
			return rc;
		case NCSCC_RC_REQ_TIMOUT:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			return SA_AIS_ERR_TIMEOUT;
		default:
			TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
			return SA_AIS_ERR_NO_RESOURCES;
		}
	}
 done:
	TRACE_LEAVE();
	return rc;
}

void clma_fill_clusterbuf_from_buf_4(SaClmClusterNotificationBufferT *buf, SaClmClusterNotificationBufferT_4 * buf_4)
{
	SaUint32T i = 0;
	for (i = 0; i < buf->numberOfItems; i++) {
		buf->notification[i].clusterChange = buf_4->notification[i].clusterChange;

		buf->notification[i].clusterNode.nodeId = buf_4->notification[i].clusterNode.nodeId;
		buf->notification[i].clusterNode.nodeAddress.family =
		    buf_4->notification[i].clusterNode.nodeAddress.family;
		buf->notification[i].clusterNode.nodeAddress.length =
		    buf_4->notification[i].clusterNode.nodeAddress.length;
		(void)memcpy(buf->notification[i].clusterNode.nodeAddress.value,
			     buf_4->notification[i].clusterNode.nodeAddress.value,
			     buf->notification[i].clusterNode.nodeAddress.length);
		buf->notification[i].clusterNode.nodeName.length = buf_4->notification[i].clusterNode.nodeName.length;
		(void)memcpy(buf->notification[i].clusterNode.nodeName.value,
			     buf_4->notification[i].clusterNode.nodeName.value,
			     buf->notification[i].clusterNode.nodeName.length);
		buf->notification[i].clusterNode.member = buf_4->notification[i].clusterNode.member;
		buf->notification[i].clusterNode.bootTimestamp = buf_4->notification[i].clusterNode.bootTimestamp;
		buf->notification[i].clusterNode.initialViewNumber =
		    buf_4->notification[i].clusterNode.initialViewNumber;
	}
}

/****************************************************************************
  Name          : saClmInitialize
 
  Description   : This function initializes the CLM for the invoking process
                  and registers the various callback functions.
 
  Arguments     : clmHandle  - ptr to the CLM handle
                  reg_cbks  - ptr to a SaClmCallbacksT structure
                  version   - Version of the CLM implementation being used 
                             by the invoking process.
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmInitialize(SaClmHandleT *clmHandle, const SaClmCallbacksT *reg_cbks, SaVersionT *version)
{
	SaAisErrorT rc;

	TRACE_ENTER();

	if ((clmHandle == NULL) || (version == NULL)) {
		TRACE("version or handle FAILED");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* validate the version */
	if ((version->releaseCode == CLM_RELEASE_CODE) && (version->majorVersion <= CLM_MAJOR_VERSION_1) &&
	    (0 < version->majorVersion)) {
		version->majorVersion = CLM_MAJOR_VERSION_1;
		version->minorVersion = CLM_MINOR_VERSION;
	} else {
		TRACE("version FAILED, required: %c.%u.%u, supported: %c.%u.%u\n",
		      version->releaseCode, version->majorVersion, version->minorVersion,
		      CLM_RELEASE_CODE, CLM_MAJOR_VERSION_1, CLM_MINOR_VERSION);
		version->releaseCode = CLM_RELEASE_CODE;
		version->majorVersion = CLM_MAJOR_VERSION_1;
		version->minorVersion = CLM_MINOR_VERSION;
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	rc = clmainitialize(clmHandle, reg_cbks, NULL, version);
 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmInitialize_4
 
  Description   : This function initializes the CLM for the invoking process
                  and registers the various callback functions.
 
  Arguments     : clmHandle   - ptr to the CLM handle
                  reg_cbks   - ptr to a SaClmCallbacksT structure
                  version    - Version of the CLM implementation being used 
                             by the invoking process.
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmInitialize_4(SaClmHandleT *clmHandle, const SaClmCallbacksT_4 * reg_cbks, SaVersionT *version)
{
	SaAisErrorT rc;

	TRACE_ENTER();

	if ((clmHandle == NULL) || (version == NULL)) {
		TRACE("version or handle FAILED");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* validate the version */
	if ((version->releaseCode == CLM_RELEASE_CODE) && 
		(version->majorVersion > CLM_MAJOR_VERSION_1 && version->majorVersion <= CLM_MAJOR_VERSION_4) &&
		(0 < version->majorVersion)) {
		version->majorVersion = CLM_MAJOR_VERSION_4;
		version->minorVersion = CLM_MINOR_VERSION;
	} else {
		TRACE("version FAILED, required: %c.%u.%u, supported: %c.%u.%u\n",
		      version->releaseCode, version->majorVersion, version->minorVersion,
		      CLM_RELEASE_CODE, CLM_MAJOR_VERSION_4, CLM_MINOR_VERSION);
		version->releaseCode = CLM_RELEASE_CODE;
		version->majorVersion = CLM_MAJOR_VERSION_4;
		version->minorVersion = CLM_MINOR_VERSION;
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	rc = clmainitialize(clmHandle, NULL, reg_cbks, version);
 done:
	TRACE_LEAVE();
	return rc;

}

static SaAisErrorT clmainitialize(SaClmHandleT *clmHandle, const SaClmCallbacksT *reg_cbks_1,
				  const SaClmCallbacksT_4 * reg_cbks_4, SaVersionT *version)
{
	clma_client_hdl_rec_t *clma_hdl_rec;
	CLMSV_MSG i_msg, *o_msg = NULL;
	SaAisErrorT ais_rc = SA_AIS_OK;
	uns32 client_id, rc;

	TRACE_ENTER();

	if ((rc = clma_startup()) != NCSCC_RC_SUCCESS) {
		TRACE("clma_startup FAILED");
		ais_rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	if (!clma_cb.clms_up) {
		clma_shutdown();
		TRACE("CLMS server is down");
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* Populate the message to be sent to the CLMS */
	memset(&i_msg, 0, sizeof(CLMSV_MSG));
	i_msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	i_msg.info.api_info.type = CLMSV_INITIALIZE_REQ;
	i_msg.info.api_info.param.init.version = *version;

	/* Send a message to CLMS to obtain a client_id/server ref id which is cluster
	 * wide unique.
	 */
	rc = clma_mds_msg_sync_send(&clma_cb, &i_msg, &o_msg, CLMS_WAIT_TIME);
	switch (rc) {
	case NCSCC_RC_SUCCESS:
		ais_rc = SA_AIS_OK;
		break;
	case NCSCC_RC_REQ_TIMOUT:
		ais_rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", ais_rc);
		goto err;
	default:
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE("clma_mds_msg_sync_send FAILED: %u", ais_rc);
		goto err;
	}

	/** Make sure the CLMS return status was SA_AIS_OK
     	**/
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		ais_rc = o_msg->info.api_resp_info.rc;
		TRACE("CLMS return FAILED");
		goto err;
	}

	/** Store the Client Id returned by the CLMS
     	** to pass into the next routine
     	**/
	client_id = o_msg->info.api_resp_info.param.client_id;

	/* create the hdl record & store the callbacks */
	clma_hdl_rec = clma_hdl_rec_add(&clma_cb, reg_cbks_1, reg_cbks_4, version, client_id);
	if (clma_hdl_rec == NULL) {
		ais_rc = SA_AIS_ERR_NO_MEMORY;
		goto err;
	}

	/* pass the handle value to the appl */
	if (SA_AIS_OK == ais_rc)
		*clmHandle = clma_hdl_rec->local_hdl;
	TRACE_1("OK intitialize with client_id: %u", client_id);

 err:
	/* free up the response message */
	if (o_msg)
		clma_msg_destroy(o_msg);	/*need to do */

	if (ais_rc != SA_AIS_OK) {
		TRACE_2("CLMA INIT FAILED\n");
		clma_shutdown();
	}

 done:
	TRACE_LEAVE();
	return ais_rc;
}

/****************************************************************************
  Name          : saClmSelectionObjectGet
 
  Description   : This function creates & returns the operating system handle 
                  associated with the CLM Handle.
 
  Arguments     : clmHandle       - CLM handle
                  selectionObject - ptr to the selection object
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmSelectionObjectGet(SaClmHandleT clmHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc = SA_AIS_OK;
	clma_client_hdl_rec_t *hdl_rec;
	NCS_SEL_OBJ sel_obj;

	TRACE_ENTER();

	if (selectionObject == NULL) {
		TRACE("selectionObject is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
		TRACE("Node is unconfigured");
		rc = SA_AIS_ERR_UNAVAILABLE;
		goto done_give_hdl;
	}

	/* Obtain the selection object from the IPC queue */
	sel_obj = m_NCS_IPC_GET_SEL_OBJ(&hdl_rec->mbx);

	/* everything's fine.. pass the sel fd to the appl */
	*selectionObject = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(sel_obj);

done_give_hdl:
	/* return hdl rec */
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmDispatch
 
  Description   : This function invokes, in the context of the calling thread,
                  the next pending callback for the CLM handle.
 
  Arguments     : hdl   - CLM handle
                  flags - flags that specify the callback execution behavior
                  of the saClmDispatch() function,
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmDispatch(SaClmHandleT clmHandle, SaDispatchFlagsT dispatchFlags)
{
	clma_client_hdl_rec_t *hdl_rec;
	SaAisErrorT rc;

	TRACE_ENTER();

	if (!m_DISPATCH_FLAG_IS_VALID(dispatchFlags)) {
		TRACE("Invalid dispatchFlags");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl clmHandle ");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }

	if ((rc = clma_hdl_cbk_dispatch(&clma_cb, hdl_rec, dispatchFlags)) != SA_AIS_OK)	/*need to do */
		TRACE("CLMA_DISPATCH_FAILURE");

done_give_hdl:
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmFinalize
 
  Description   : This function closes the association, represented by the 
                  CLM handle, between the invoking process and the CLM.
 
  Arguments     : hdl - CLM handle
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmFinalize(SaClmHandleT clmHandle)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 mds_rc;

	TRACE_ENTER();

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	/** populate & send the finalize message
     	** and make sure the finalize from the server
     	** end returned before deleting the local records.
     	**/
	memset(&msg, 0, sizeof(CLMSV_MSG));
	msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	msg.info.api_info.type = CLMSV_FINALIZE_REQ;
	msg.info.api_info.param.finalize.client_id = hdl_rec->clms_client_id;

	mds_rc = clma_mds_msg_sync_send(&clma_cb, &msg, &o_msg, CLMS_WAIT_TIME);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
		clma_msg_destroy(o_msg);	/*need to do */
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

	if (rc == SA_AIS_OK) {
	/** delete the hdl rec
         ** including all resources allocated by client if MDS send is 
         ** succesful. 
         **/
		rc = clma_hdl_rec_del(&clma_cb.client_list, hdl_rec);	/*need to do */
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_1("clma_hdl_rec_del failed");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
	}

 done_give_hdl:
	ncshm_give_hdl(clmHandle);

	if (rc == SA_AIS_OK) {
		rc = clma_shutdown();
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_1("clma_shutdown failed");
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : saClmClusterTrack
 
  Description   : This fuction requests CLM to start tracking the changes
                  in the cluster membership.
 
  Arguments     : clmHandle   - CLM handle
                  flags - flags that determines when the CLM track callback is 
                          invoked
                  buf   - ptr to the linear buffer provided by the appl

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterTrack(SaClmHandleT clmHandle, SaUint8T flags, SaClmClusterNotificationBufferT *buf)
{
	SaAisErrorT rc;
	TRACE_ENTER();

	rc = clmaclustertrack(clmHandle, flags, buf, NULL);

	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmClusterTrack_4
 
  Description   : This fuction requests CLM to start tracking the changes
                  in the cluster membership.
 
  Arguments     : clmHandle   - CLM handle
                  flags - flags that determines when the CLM track callback is 
                          invoked
                  buf   - ptr to the linear buffer provided by the appl

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterTrack_4(SaClmHandleT clmHandle, SaUint8T flags, SaClmClusterNotificationBufferT_4 * buf)
{
	SaAisErrorT rc;
	TRACE_ENTER();

	rc = clmaclustertrack(clmHandle, flags, NULL, buf);

	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : clmaclustertrack
 
  Description   : This fuction requests CLM to start tracking the changes
                  in the cluster membership.
 
  Arguments     : clmHandle   - CLM handle
                  flags - flags that determines when the CLM track callback is 
                          invoked
                  buf   - SaClmClusterNotificationBufferT ptr to the linear 
			  buffer provided by the appl
                  buf_4   - SaClmClusterNotificationBufferT_4 ptr to the linear
			   buffer provided by the appl

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
static SaAisErrorT clmaclustertrack(SaClmHandleT clmHandle, SaUint8T flags,
				    SaClmClusterNotificationBufferT *buf, SaClmClusterNotificationBufferT_4 * buf_4)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG i_msg;
	SaAisErrorT rc;

	TRACE_ENTER();

	if (flags == 0) {
		TRACE("clmHandle or flags NULL");
		rc = SA_AIS_ERR_BAD_FLAGS;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (buf_4 == NULL && buf != NULL) {
		if (!clma_validate_version(hdl_rec->version)) {
			TRACE("Version error from saClmClusterTrack");
			rc = SA_AIS_ERR_VERSION;
			goto done_give_hdl;
		}
	} else if (buf == NULL && buf_4 != NULL) {
		if (clma_validate_version(hdl_rec->version)) {
			TRACE("Version error from saClmClusterTrack_4");
			rc = SA_AIS_ERR_VERSION;
			goto done_give_hdl;
		}
	}

	if (clma_validate_version(hdl_rec->version)) {
		if ((rc = clma_validate_flags_buf(hdl_rec, flags, buf)) != SA_AIS_OK)
			goto done_give_hdl;
	} else {
		TRACE("B.4.1 version");
		if ((rc = clma_validate_flags_buf_4(hdl_rec, flags, buf_4)) != SA_AIS_OK)
			goto done_give_hdl;
	}

	if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
		TRACE("Node is unconfigured");
		rc = SA_AIS_ERR_UNAVAILABLE;
		goto done_give_hdl; 
	}

	TRACE("RC after validate flagsTrack %d", rc);
	/* Populate the message to be sent to the CLMS */
	memset(&i_msg, 0, sizeof(CLMSV_MSG));
	i_msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	i_msg.info.api_info.type = CLMSV_TRACK_START_REQ;
	i_msg.info.api_info.param.track_start.flags = flags;
	i_msg.info.api_info.param.track_start.client_id = hdl_rec->clms_client_id;

	if (clma_validate_version(hdl_rec->version)) {
		rc = clma_send_mds_msg_get_clusternotificationbuf(hdl_rec, flags, i_msg, buf);
	} else {
		rc = clma_send_mds_msg_get_clusternotificationbuf_4(hdl_rec, flags, i_msg, buf_4);
	}

 done_give_hdl:
	TRACE("RC before give handle flagsTrack %d", rc);
	ncshm_give_hdl(clmHandle);
 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmClusterTrackStop
 
  Description   : This fuction requests CLM to stop tracking the changes
                  in the cluster membership.
 
  Arguments     : hdl   - CLM handle

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterTrackStop(SaClmHandleT clmHandle)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CLMSV_MSG *cbk_msg;
	CLMSV_MSG *async_cbk_msg = NULL, *process = NULL;
	uns32 mds_rc;

	TRACE_ENTER();

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }

	/** populate & send the finalize message
        ** and make sure the finalize from the server
        ** end returned before deleting the local records.
        **/
	memset(&msg, 0, sizeof(CLMSV_MSG));
	msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	msg.info.api_info.type = CLMSV_TRACK_STOP_REQ;
	msg.info.api_info.param.track_stop.client_id = hdl_rec->clms_client_id;

	mds_rc = clma_mds_msg_sync_send(&clma_cb, &msg, &o_msg, CLMS_WAIT_TIME);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
		clma_msg_destroy(o_msg);	/*need to do */
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

	if (rc == SA_AIS_OK) {
		do {
			if (NULL == (cbk_msg = (CLMSV_MSG *)
				     m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg)))
				break;
			if (cbk_msg->info.cbk_info.type == CLMSV_TRACK_CBK) {
				TRACE_2("Dropping Track Callback %d", cbk_msg->info.cbk_info.type);
				clma_msg_destroy(cbk_msg);
			} else if (cbk_msg->info.cbk_info.type == CLMSV_NODE_ASYNC_GET_CBK) {
				clma_add_to_async_cbk_msg_list(&async_cbk_msg, cbk_msg);
			} else {
				TRACE("Dropping unsupported callback type %d", cbk_msg->info.cbk_info.type);
				clma_msg_destroy(cbk_msg);
			}
		} while (1);

		process = async_cbk_msg;
		while (process) {
			/* IPC send is making next as NULL in process pointer */
			/* process the message */
			async_cbk_msg = async_cbk_msg->next;
			rc = clma_clms_msg_proc(&clma_cb, process, MDS_SEND_PRIORITY_MEDIUM);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_2("From TrackStop clma_clms_msg_proc returned: %d", rc);
			}
			process = async_cbk_msg;
			/*async_cbk_msg->next = NULL;
			   async_cbk_msg = process; */
		}
	}

 done_give_hdl:
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : saClmClusterNodeGet
 
  Description   : This fuction synchronously gets the information about a 
                  cluster member (identified by node-id).
 
  Arguments     : hdl          - CLM handle
                  node_id      - node-id for which information is to be 
                                 retrieved
                  timeout      - time-interval within which the information 
                                 should be returned
                  cluster_node - ptr to the node info

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterNodeGet(SaClmHandleT clmHandle,
				SaClmNodeIdT node_id, SaTimeT timeout, SaClmClusterNodeT *cluster_node)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	if (cluster_node == NULL) {
		TRACE("cluster_node is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	rc = clmaclusternodeget(clmHandle, node_id, timeout, cluster_node, NULL);
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : saClmClusterNodeGet_4
 
  Description   : This fuction synchronously gets the information about a 
                  cluster member (identified by node-id).
 
  Arguments     : hdl          - CLM handle
                  node_id      - node-id for which information is to be 
                                 retrieved
                  timeout      - time-interval within which the information 
                                 should be returned
                  cluster_node_4 - ptr to the node info

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterNodeGet_4(SaClmHandleT clmHandle,
				  SaClmNodeIdT node_id, SaTimeT timeout, SaClmClusterNodeT_4 * cluster_node_4)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	if (cluster_node_4 == NULL) {
		TRACE("cluster_node is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	rc = clmaclusternodeget(clmHandle, node_id, timeout, NULL, cluster_node_4);
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clmaclusternodeget
 
  Description   : This fuction synchronously gets the information about a 
                  cluster member (identified by node-id).
 
  Arguments     : hdl          - CLM handle
                  node_id      - node-id for which information is to be 
                                 retrieved
                  timeout      - time-interval within which the information 
                                 should be returned
		  cluster_node - ptr to the node info SaClmClusterNodeT
                  cluster_node_4 - ptr to the node info SaClmClusterNodeT_4

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/

static SaAisErrorT clmaclusternodeget(SaClmHandleT clmHandle,
				      SaClmNodeIdT node_id,
				      SaTimeT timeout,
				      SaClmClusterNodeT *cluster_node, SaClmClusterNodeT_4 * cluster_node_4)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 ncs_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (node_id == 0) {
		TRACE("node_id is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* convert SaTimeT into tens of milli seconds */
	timeout = m_CLMA_CONVERT_SATIME_TEN_MILLI_SEC(timeout);
	if (timeout < 0) {
		TRACE("Timeout value is negative");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}
	if (timeout < CLM_API_MIN_TIMEOUT) {
		TRACE("timeout is LESS THAN 100ms");
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	}
	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (cluster_node_4 == NULL) {
		if (!clma_validate_version(hdl_rec->version)) {
			TRACE("Version error from saClmClusterNodeGet");
			rc = SA_AIS_ERR_VERSION;
			goto done_give_hdl;
		}
	} else {
		if (clma_validate_version(hdl_rec->version)) {
			TRACE("Version error from saClmClusterNodeGet_4");
			rc = SA_AIS_ERR_VERSION;
			goto done_give_hdl;
		}
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }

	if((hdl_rec->is_member == FALSE) && (!clma_validate_version(hdl_rec->version))) { 
		TRACE("Node is not a member");
		rc = SA_AIS_ERR_UNAVAILABLE;
		goto done_give_hdl;
	}
	/** populate & send the finalize message
        ** and make sure the finalize from the server
        ** end returned before deleting the local records.
        **/
	memset(&msg, 0, sizeof(CLMSV_MSG));
	msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	msg.info.api_info.type = CLMSV_NODE_GET_REQ;
	msg.info.api_info.param.node_get.client_id = hdl_rec->clms_client_id;
	msg.info.api_info.param.node_get.node_id = node_id;

	ncs_rc = clma_mds_msg_sync_send(&clma_cb, &msg, &o_msg, timeout);
	switch (ncs_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

	if (rc == SA_AIS_OK) {
		if (clma_validate_version(hdl_rec->version)) {
			clma_fill_node_from_node4(cluster_node, o_msg->info.api_resp_info.param.node_get);
		} else {
			memset(cluster_node_4, 0, sizeof(SaClmClusterNodeT_4));
			memcpy(cluster_node_4, &o_msg->info.api_resp_info.param.node_get, sizeof(SaClmClusterNodeT_4));
		}
	}

 done_give_hdl:
	clma_msg_destroy(o_msg);
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : saClmClusterNodeGetAsync
 
  Description   : This fuction asynchronously gets the information about a 
                  cluster member (identified by node-id).
 
  Arguments     : hdl          - CLM handle
                  inv          - invocation value
                  node_id      - node-id for which information is to be 
                                 retrieved
                  cluster_node - ptr to the node info

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterNodeGetAsync(SaClmHandleT clmHandle, SaInvocationT inv, SaClmNodeIdT node_id)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG msg;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 mds_rc;

	TRACE_ENTER();

	if ((node_id == 0) || (inv == 0)) {
		TRACE("node_id or invocation is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (clma_validate_version(hdl_rec->version)) {
		if (!hdl_rec->cbk_param.reg_cbk.saClmClusterNodeGetCallback) {
			rc = SA_AIS_ERR_INIT;
			goto done_give_hdl;
		}
	} else {
		if (!hdl_rec->cbk_param.reg_cbk_4.saClmClusterNodeGetCallback) {
			rc = SA_AIS_ERR_INIT;
			goto done_give_hdl;
		}
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }

        if((hdl_rec->is_member == FALSE) && (!clma_validate_version(hdl_rec->version))) {     
                TRACE("Node is not a member");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl;
        }
	/** populate & send the finalize message
        ** and make sure the finalize from the server
        ** end returned before deleting the local records.
        **/
	memset(&msg, 0, sizeof(CLMSV_MSG));
	msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	msg.info.api_info.type = CLMSV_NODE_GET_ASYNC_REQ;
	msg.info.api_info.param.node_get_async.client_id = hdl_rec->clms_client_id;
	msg.info.api_info.param.node_get_async.inv = inv;
	msg.info.api_info.param.node_get_async.node_id = node_id;

	mds_rc = clma_mds_msg_async_send(&clma_cb, &msg, MDS_SEND_PRIORITY_HIGH);	/* fix me ?? */
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

 done_give_hdl:
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmClusterNotificationFree_4
 
  Description   : This fuction free SaClmClusterNotificationT_4 *notification
                  which was used in saClmClusterTrack_4.
 
  Arguments     : clmHandle          - CLM handle
                  notification - SaClmClusterNotificationT_4 *notification

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saClmClusterNotificationFree_4(SaClmHandleT clmHandle, SaClmClusterNotificationT_4 * notification)
{
	clma_client_hdl_rec_t *hdl_rec;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	if (notification == NULL) {
		TRACE("notification is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (clma_validate_version(hdl_rec->version)) {
		TRACE("not supported in the version specified");
		rc = SA_AIS_ERR_VERSION;
		goto done_give_hdl;
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }

	free(notification);

 done_give_hdl:
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : saClmResponse_4
 
  Description   : This function is used by a process to provide a response to the
		  saClmTrackCallback() callback 
                  which was used in saClmClusterTrack_4.
 
  Arguments     : clmHandle          - CLM handle
                  notification - SaClmClusterNotificationT_4 *notification

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/

SaAisErrorT saClmResponse_4(SaClmHandleT clmHandle, SaInvocationT invocation, SaClmResponseT response)
{
	clma_client_hdl_rec_t *hdl_rec;
	CLMSV_MSG i_msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 mds_rc;

	TRACE_ENTER();

	if (invocation == 0) {
		TRACE("invocation is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if ((response < SA_CLM_CALLBACK_RESPONSE_OK) || (response > SA_CLM_CALLBACK_RESPONSE_ERROR)) {
		TRACE("response is invalid");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* Check Whether CLMS is up or not */
	if (!clma_cb.clms_up) {
		TRACE("CLMS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_CLMA, clmHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

        if((hdl_rec->is_configured == FALSE) && (!clma_validate_version(hdl_rec->version))) {
                TRACE("Node is unconfigured");
                rc = SA_AIS_ERR_UNAVAILABLE;
                goto done_give_hdl; 
        }
	/** populate & send the finalize message
        ** and make sure the finalize from the server
        ** end returned before deleting the local records.
        **/
	memset(&i_msg, 0, sizeof(CLMSV_MSG));
	i_msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
	i_msg.info.api_info.type = CLMSV_RESPONSE_REQ;
	i_msg.info.api_info.param.clm_resp.client_id = hdl_rec->clms_client_id;
	i_msg.info.api_info.param.clm_resp.inv = invocation;
	i_msg.info.api_info.param.clm_resp.resp = response;

	mds_rc = clma_mds_msg_sync_send(&clma_cb, &i_msg, &o_msg, CLMS_WAIT_TIME);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("clma_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
		clma_msg_destroy(o_msg);
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

 done_give_hdl:
	ncshm_give_hdl(clmHandle);

 done:
	TRACE_LEAVE();
	return rc;

}

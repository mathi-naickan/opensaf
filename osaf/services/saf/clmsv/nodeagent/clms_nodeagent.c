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
 * Author(s):  Emerson Network Power
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <configmake.h>
#include <daemon.h>

#include <nid_api.h>
#include <ncs_main_papi.h>
#include <ncssysf_ipc.h>
#include <mds_papi.h>
#include <ncs_hdl_pub.h>
#include <ncsencdec_pub.h>
#include <ncs_mda_pvt.h>
#include <ncs_util.h>
#include <logtrace.h>
#include <saClm.h>
#include <configmake.h>
#include <nid_api.h>
#include "clmsv_msg.h"
#include "clmsv_enc_dec.h"

/*TBD : Add nodeaddress as well for future use*/

typedef struct node_detail_t {
	SaUint32T node_id;
	SaNameT node_name;
} NODE_INFO;

#define CLMNA_MDS_SUB_PART_VERSION   1
#define CLMS_NODEUP_WAIT_TIME 1000
#define CLMNA_SVC_PVT_SUBPART_VERSION  1
#define CLMNA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT 1
#define CLMNA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT 1
#define CLMNA_WRT_CLMS_SUBPART_VER_RANGE             \
        (CLMNA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT - \
         CLMNA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/*msg format version for CLMNA subpart version 1 */
static MDS_CLIENT_MSG_FORMAT_VER
 CLMNA_WRT_CLMS_MSG_FMT_ARRAY[CLMNA_WRT_CLMS_SUBPART_VER_RANGE] = { 1 };

/* CLMS CLMNA sync params */
int clms_sync_awaited;
NCS_SEL_OBJ clms_sync_sel;
MDS_HDL mds_hdl;
MDS_DEST clms_mds_dest;		/* CLMS absolute/virtual address */

static uns32 clmna_mds_enc(struct ncsmds_callback_info *info);
static uns32 clmna_mds_callback(struct ncsmds_callback_info *info);

static uns32 clmna_mds_cpy(struct ncsmds_callback_info *info)
{
	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_dec(struct ncsmds_callback_info *info)
{
	uns8 *p8;
	CLMSV_MSG *msg;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uns8 local_data[20];
	uns32 total_bytes = 0;
	TRACE_ENTER();

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   CLMNA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT,
					   CLMNA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT, CLMNA_WRT_CLMS_MSG_FMT_ARRAY)) {
		TRACE("Invalid message format!!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/** Allocate a new msg in both sync/async cases 
     	**/
	if (NULL == (msg = calloc(1, sizeof(CLMSV_MSG)))) {
		TRACE("calloc failed\n");
		return NCSCC_RC_FAILURE;
	}

	info->info.dec.o_msg = (uns8 *)msg;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->evt_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	switch (msg->evt_type) {
	case CLMSV_CLMS_TO_CLMA_API_RESP_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.api_resp_info.type = ncs_decode_32bit(&p8);
			msg->info.api_resp_info.rc = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			total_bytes += 8;
			TRACE_2("CLMSV_CLMA_API_RESP_MSG for Node_Up rc = %d", (int)msg->info.api_resp_info.rc);

			switch (msg->info.api_resp_info.type) {
			case CLMSV_CLUSTER_JOIN_RESP:
				total_bytes += clmsv_decodeSaNameT(uba, &(msg->info.api_resp_info.param.node_name));
				break;
			default:
				TRACE_2("Unknown API RSP type %d", msg->info.api_resp_info.type);
				free(msg);
				return NCSCC_RC_FAILURE;
			}
		}
		break;
	default:
		TRACE("Unknown MSG type %d", msg->evt_type);
		free(msg);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine */
	rc = clmna_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds_dec FAILED ");
	}
	return rc;
}

static uns32 clmna_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_svc_evt(struct ncsmds_callback_info *mds_cb_info)
{
	TRACE_ENTER2("%d", mds_cb_info->info.svc_evt.i_change);

	switch (mds_cb_info->info.svc_evt.i_change) {
	case NCSMDS_NEW_ACTIVE:
	case NCSMDS_UP:
		switch (mds_cb_info->info.svc_evt.i_svc_id) {
			TRACE("svc_id %d", mds_cb_info->info.svc_evt.i_svc_id);
		case NCSMDS_SVC_ID_CLMS:
			clms_mds_dest = mds_cb_info->info.svc_evt.i_dest;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_enc_flat(struct ncsmds_callback_info *info)
{
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = info->info.enc_flat;
	/* Invoke the regular mds_enc routine  */
	return clmna_mds_enc(info);
}

static uns32 clmna_mds_enc(struct ncsmds_callback_info *info)
{
	CLMSV_MSG *msg;
	NCS_UBAID *uba;
	uns8 *p8;
	uns32 total_bytes = 0;

	TRACE_ENTER();

	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						CLMNA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT,
						CLMNA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT,
						CLMNA_WRT_CLMS_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		LOG_ER("Wrong msg_fmt_version!!");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (CLMSV_MSG *) info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	TRACE_2("msgtype: %d", msg->evt_type);

	if (uba == NULL) {
		LOG_ER("uba == NULL");
		return NCSCC_RC_FAILURE;
	}

	/** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		LOG_ER("NULL pointer");
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&p8, msg->evt_type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (CLMSV_CLMA_TO_CLMS_API_MSG == msg->evt_type) {

		/* encode the nodeupinfo */
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			LOG_ER("NULL pointer");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		ncs_encode_32bit(&p8, msg->info.api_info.type);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		TRACE_2("api_info.type: %d", msg->info.api_info.type);

		if (msg->info.api_info.type == CLMSV_CLUSTER_JOIN_REQ) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				LOG_ER("NULL pointer");
				TRACE_LEAVE();
				return NCSCC_RC_FAILURE;
			}

			ncs_encode_32bit(&p8, msg->info.api_info.param.nodeup_info.node_id);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
			total_bytes += clmsv_encodeSaNameT(uba, &(msg->info.api_info.param.nodeup_info.node_name));
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_callback(struct ncsmds_callback_info *info)
{
	uns32 rc;

	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		clmna_mds_cpy,	/* MDS_CALLBACK_COPY      0 */
		clmna_mds_enc,	/* MDS_CALLBACK_ENC       1 */
		clmna_mds_dec,	/* MDS_CALLBACK_DEC       2 */
		clmna_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  3 */
		clmna_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  4 */
		clmna_mds_rcv,	/* MDS_CALLBACK_RECEIVE   5 */
		clmna_mds_svc_evt
	};

	if (info->i_op <= MDS_CALLBACK_SVC_EVENT) {
		rc = (*cb_set[info->i_op]) (info);
		if (rc != NCSCC_RC_SUCCESS)
			LOG_ER("MDS_CALLBACK_SVC_EVENT not in range");

		TRACE_LEAVE();
	}

	return NCSCC_RC_SUCCESS;
}

static uns32 clmna_mds_init(void)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO mds_info;
	uns32 rc = NCSCC_RC_SUCCESS;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_CLMS;

	TRACE_ENTER();

	/** Create the ADEST for CLMNA and get the pwe hdl**/
	memset(&ada_info, '\0', sizeof(ada_info));
	ada_info.req = NCSADA_GET_HDLS;

	if (NCSCC_RC_SUCCESS != (rc = ncsada_api(&ada_info))) {
		LOG_ER("NCSADA_GET_HDLS failed, rc = %d", rc);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/** Store the info obtained from MDS ADEST creation  **/
	mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;

	/** Now install into mds **/
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMNA;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* PWE scope */
	mds_info.info.svc_install.i_svc_cb = clmna_mds_callback;	/* callback */
	mds_info.info.svc_install.i_mds_q_ownership = FALSE;	/* CLMNA doesn't own the mds queue */
	mds_info.info.svc_install.i_mds_svc_pvt_ver = CLMNA_SVC_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		LOG_ER("mds api call failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Now subscribe for events that will be generated by MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMNA;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("mds api call failed");
		TRACE_LEAVE();
		return rc;
	}

	TRACE_LEAVE();
	return rc;
}

static int get_node_info(NODE_INFO *node)
{
	FILE *fp;

	fp = fopen(PKGSYSCONFDIR "/node_name", "r");
	if (fp == NULL) {
		LOG_ER("Could not open file %s - %s", PKGSYSCONFDIR "node_name", strerror(errno));
		return -1;
	}

	if(EOF == fscanf(fp, "%s", node->node_name.value)){
		fclose(fp);
		LOG_ER("Could not get node name - %s", strerror(errno));
		return -1;
	}
	fclose(fp);
	node->node_name.length = strlen((char *)node->node_name.value);
	TRACE("%s", node->node_name.value);

	fp = fopen(PKGLOCALSTATEDIR "/node_id", "r");
	if (fp == NULL) {
		LOG_ER("Could not open file %s - %s", PKGLOCALSTATEDIR "node_id", strerror(errno));
		return -1;
	}

	if(EOF == fscanf(fp, "%x", &node->node_id)){
		fclose(fp);
		LOG_ER("Could not get node id - %s", strerror(errno));
		return -1;
	}
	fclose(fp);
	TRACE("%d", node->node_id);

	return 0;
}

static uns32 clmna_mds_msg_sync_send(CLMSV_MSG * i_msg, CLMSV_MSG ** o_msg, uns32 timeout)
{
	NCSMDS_INFO mds_info;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMNA;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_CLMS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms FIX!!! */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = clms_mds_dest;

	/* send the message */
	if (NCSCC_RC_SUCCESS == (rc = ncsmds_api(&mds_info))) {
		/* Retrieve the response and take ownership of the memory  */
		*o_msg = (CLMSV_MSG *) mds_info.info.svc_send.info.sndrsp.o_rsp;
		mds_info.info.svc_send.info.sndrsp.o_rsp = NULL;
	} else
		TRACE("clma_mds_msg_sync_send FAILED");

	TRACE_LEAVE();
	return rc;
}

int main(int argc, char *argv[])
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CLMSV_MSG msg, *o_msg = NULL;;
	struct pollfd fds[1];
	int ret;
	NODE_INFO node_info;

	daemonize(argc, argv);

	if (get_node_info(&node_info) != 0) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup FAILED");
		goto done;
	}

	if (clmna_mds_init() != NCSCC_RC_SUCCESS)
		goto done;

	/* Poll every second until we have the MDS adress of the server */
	while (1) {
		ret = poll(fds, 0, 100);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			syslog(LOG_ERR, "%s: poll failed - %s", __FUNCTION__, strerror(errno));
			break;
		}

		if (clms_mds_dest != 0) {
			msg.evt_type = CLMSV_CLMA_TO_CLMS_API_MSG;
			msg.info.api_info.type = CLMSV_CLUSTER_JOIN_REQ;
			msg.info.api_info.param.nodeup_info.node_id = node_info.node_id;
			msg.info.api_info.param.nodeup_info.node_name = node_info.node_name;
			rc = clmna_mds_msg_sync_send(&msg, &o_msg, CLMS_NODEUP_WAIT_TIME);
			switch (rc) {
			case NCSCC_RC_SUCCESS:
				break;
			case NCSCC_RC_REQ_TIMOUT:
				LOG_ER("clmna_mds_msg_sync_send Timed Out");
				goto done;
			default:
				LOG_ER("clmna_mds_msg_sync_send FAILED: %u", rc);
				goto done;
			}

			if (o_msg != NULL) {
				rc = o_msg->info.api_resp_info.rc;
			} else
				rc = SA_AIS_ERR_NO_RESOURCES;

			if (rc == SA_AIS_ERR_NOT_EXIST) {
				LOG_ER("Node : %s is Not a member of cluster",
				       o_msg->info.api_resp_info.param.node_name.value);
				free(o_msg);
				goto done;
			}

			if (rc == SA_AIS_OK)
				LOG_NO("Node : %s has Joined the cluster",
				       o_msg->info.api_resp_info.param.node_name.value);

			if (o_msg != NULL) 
					free(o_msg);

			break;	/*get out of while loop */
		}
	}

 done:
	(void)nid_notify("CLMNA", rc, NULL);
	LOG_NO("node name:%s, node ID:%x,exiting", node_info.node_name.value, node_info.node_id);
	return 0;
}

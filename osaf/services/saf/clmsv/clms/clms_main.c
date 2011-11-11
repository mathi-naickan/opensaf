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
 * Author(s):  Emerson Network Power
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <poll.h>

#include <configmake.h>
#include <daemon.h>

#include <nid_api.h>
#include "clms.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

#ifdef ENABLE_AIS_PLM
#define FD_USR1 0
#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_PLM 3
#define FD_IMM 4 		/* Must be the last in the fds array */
#define NUM_FD 5
#else
#define FD_USR1 0
#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_IMM 3 		/* Must be the last in the fds array */
#define NUM_FD 4
#endif



/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

static CLMS_CB _clms_cb;
CLMS_CB *clms_cb = &_clms_cb;

CLMS_CLUSTER_INFO *osaf_cluster;
static struct pollfd fds[NUM_FD];
static nfds_t nfds = NUM_FD;
static NCS_SEL_OBJ usr1_sel_obj;

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * Callback from RDA. Post a message/event to the clms mailbox.
 * @param cb_hdl
 * @param cb_info
 * @param error_code
 */
static void rda_cb(uns32 cb_hdl, PCS_RDA_CB_INFO *cb_info, PCSRDA_RETURN_CODE error_code)
{
	uns32 rc;
	CLMSV_CLMS_EVT *evt = NULL;

	TRACE_ENTER();

	evt = calloc(1, sizeof(CLMSV_CLMS_EVT));
	if (NULL == evt) {
		LOG_ER("calloc failed");
		goto done;
	}

	evt->type = CLMSV_CLMS_RDA_EVT;
	evt->info.rda_info.io_role = cb_info->info.io_role;

	rc = ncs_ipc_send(&clms_cb->mbx, (NCS_IPC_MSG *)evt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("IPC send failed %d", rc);

 done:
	TRACE_LEAVE();
}

/**
 * USR1 signal is used when AMF wants to instantiate us as a
 * component. Wakes up the main thread to register with
 * AMF.
 * 
 * @param i_sig_num
 */
static void sigusr1_handler(int sig)
{
	(void)sig;
	signal(SIGUSR1, SIG_IGN);
	ncs_sel_obj_ind(usr1_sel_obj);
}

/**
* Get the Self node info
*/
static uns32 clms_self_node_info(void)
{
	uns32 rc = NCSCC_RC_FAILURE;
	FILE *fp;
	SaNameT node_name_dn = { 0 };
	CLMS_CLUSTER_NODE *node = NULL;
	char node_name[256];
	const char *node_name_file = PKGSYSCONFDIR "/node_name";

	clms_cb->node_id = m_NCS_GET_NODE_ID;

	TRACE_ENTER();

	if (clms_cb->ha_state == SA_AMF_HA_STANDBY)
		return NCSCC_RC_SUCCESS;

	fp = fopen(node_name_file, "r");
	if (fp == NULL) {
		LOG_ER("Could not open file %s - %s", node_name_file, strerror(errno));
		goto done;
	}

	if(EOF == fscanf(fp, "%s", node_name)) {
		LOG_ER("Could not read node name - %s", strerror(errno));
		fclose(fp);
		goto done;
	}

	fclose(fp);

	/* Generate a CLM node DN with the help of cluster DN */
	node_name_dn.length = snprintf((char *)node_name_dn.value, sizeof(node_name_dn.value),
				       "safNode=%s,%s", node_name, osaf_cluster->name.value);

	TRACE("%s", node_name_dn.value);

	node = clms_node_get_by_name(&node_name_dn);

	if (node == NULL) {
		LOG_ER("%s not found in the database. Please verify %s", node_name_dn.value, node_name_file);
		goto done;
	}

	node->node_id = clms_cb->node_id;
	node->nodeup = SA_TRUE;
	if (clms_node_add(node, 0) != NCSCC_RC_SUCCESS)
		goto done;

	if (node->admin_state == SA_CLM_ADMIN_UNLOCKED) {
		node->member = SA_TRUE;
		++(osaf_cluster->num_nodes);
		node->boot_time = clms_get_SaTime();
#ifdef ENABLE_AIS_PLM
		node->ee_red_state = SA_PLM_READINESS_IN_SERVICE;	/*TBD : changed when plm scripts are added to rc scripts */
#endif
		osaf_cluster->init_time = node->boot_time;
	}
	rc = NCSCC_RC_SUCCESS;
 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * This function initializes the CLMS_CB including the
 * Patricia trees.
 *
 * @param clms_cb * - Pointer to the CLMS_CB.
 *
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
uns32 clms_cb_init(CLMS_CB * clms_cb)
{
	NCS_PATRICIA_PARAMS client_param;
	NCS_PATRICIA_PARAMS id_param;
	NCS_PATRICIA_PARAMS eename_param;
	NCS_PATRICIA_PARAMS nodename_param;

	TRACE_ENTER();

	memset(&client_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&id_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&eename_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&nodename_param, 0, sizeof(NCS_PATRICIA_PARAMS));

	client_param.key_size = sizeof(uns32);
	id_param.key_size = sizeof(uns32);
	eename_param.key_size = sizeof(SaNameT);
	nodename_param.key_size = sizeof(SaNameT);

	/* Assign Initial HA state */
	clms_cb->ha_state = CLMS_HA_INIT_STATE;
	osaf_cluster = NULL;
	clms_cb->reg_with_plm = SA_FALSE;
	clms_cb->cluster_view_num = 0;
	clms_cb->csi_assigned = FALSE;
	clms_cb->curr_invid = 1;
	clms_cb->immOiHandle = 0;

	/* Assign Version. Currently, hardcoded, This will change later */
	clms_cb->clm_ver.releaseCode = CLM_RELEASE_CODE;
	clms_cb->clm_ver.majorVersion = CLM_MAJOR_VERSION_4;
	clms_cb->clm_ver.minorVersion = CLM_MINOR_VERSION;

	/* Initialize patricia tree for reg list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&clms_cb->client_db, &client_param))
		return NCSCC_RC_FAILURE;

	/* Initialize nodes_db patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&clms_cb->nodes_db, &nodename_param))
		return NCSCC_RC_FAILURE;

	/* Initialize ee_lookup patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&clms_cb->ee_lookup, &eename_param))
		return NCSCC_RC_FAILURE;

	/* Initialize id_lookup patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&clms_cb->id_lookup, &id_param))
		return NCSCC_RC_FAILURE;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Initialize clm
 * 
 * @return uns32
 */
static uns32 clms_init(void)
{
	uns32 rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		TRACE("ncs_core_agents_startup FAILED");
		goto done;
	}

	/* Initialize clms control block */
	if ((rc = clms_cb_init(clms_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_cb_init FAILED");
		goto done;
	}

	if ((rc = rda_get_role(&clms_cb->ha_state)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_get_role FAILED");
		goto done;
	}
	TRACE("Current RDA Role %d", clms_cb->ha_state);

	if ((rc = rda_register_callback(0, rda_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_register_callback FAILED %u", rc);
		goto done;
	}

	/* Create the mailbox used for communication with CLMS */
	if ((rc = m_NCS_IPC_CREATE(&clms_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		goto done;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&clms_cb->mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
		goto done;
	}

	/*Initialize mds */
	if ((rc = clms_mds_init(clms_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_mds_init FAILED %d", rc);
		goto done;
	}

	/* Initialize with MBCSV */
	if ((rc = clms_mbcsv_init(clms_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_mbcsv_init FAILED");
		goto done;
	}

	/* Initialize with IMMSv */
	if ((rc = clms_imm_init(clms_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_imm_init FAILED");
		goto done;
	}

	/* Declare as implementer && Read configuration data from IMM */
	if ((rc = clms_imm_activate(clms_cb)) != SA_AIS_OK) {
		LOG_ER("clms_imm_activate FAILED");
		goto done;
	}

	if ((rc = clms_ntf_init(clms_cb)) != SA_AIS_OK) {
		LOG_ER("clms_ntf_init FAILED");
		goto done;
	}

#ifdef ENABLE_AIS_PLM
	if ((rc = clms_plm_init(clms_cb)) != SA_AIS_OK) {
		LOG_ER("clms_plm_init FAILED");
		goto done;
	}
#endif

	/*Self Node update */
	if ((rc = clms_self_node_info()) != NCSCC_RC_SUCCESS)
		goto done;

	/* Create a selection object */
	if ((rc = ncs_sel_obj_create(&usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	/*
	 ** Initialize a signal handler that will use the selection object.
	 ** The signal is sent from our script when AMF does instantiate.
	 */
	if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 failed: %s", strerror(errno));
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;

 done:
	if (nid_notify("CLMD", rc, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * The main routine for the clms daemon.
 * @param argc
 * @param argv
 * 
 * @return int
 */
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;
	uns32 rc;
	osaf_cluster = NULL;

	daemonize(argc, argv);

	if (clms_init() != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_init failed");
		goto done;
	}

	mbx_fd = ncs_ipc_get_sel_obj(&clms_cb->mbx);

	/* Set up all file descriptors to listen to */
	fds[FD_USR1].fd = usr1_sel_obj.rmv_obj;
	fds[FD_USR1].events = POLLIN;
	fds[FD_MBCSV].fd = clms_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = clms_cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

#ifdef ENABLE_AIS_PLM
	fds[FD_PLM].fd = clms_cb->plm_sel_obj;
	fds[FD_PLM].events = POLLIN;
#endif

	while (1) {

		if (clms_cb->immOiHandle != 0) {
			fds[FD_IMM].fd = clms_cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = NUM_FD;
		} else {
			nfds = NUM_FD - 1;
		}

		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}
		if (fds[FD_AMF].revents & POLLIN) {
			if (clms_cb->amf_hdl != 0) {
				if ((error = saAmfDispatch(clms_cb->amf_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
					LOG_ER("saAmfDispatch failed: %u", error);
					break;
				}
			} else {
				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_rmv_ind(usr1_sel_obj, TRUE, TRUE);
				ncs_sel_obj_destroy(usr1_sel_obj);

				if (clms_amf_init(clms_cb) != NCSCC_RC_SUCCESS) {
					LOG_ER("AMF Initialization failed");
					break;
				}

				TRACE("AMF Initialization SUCCESS......");
				fds[FD_AMF].fd = clms_cb->amf_sel_obj;
			}

		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			if ((rc = clms_mbcsv_dispatch(clms_cb->mbcsv_hdl)) != NCSCC_RC_SUCCESS) {
				LOG_ER("MBCSv Dispatch Failed");
				break;
			}
		}

		if (fds[FD_MBX].revents & POLLIN) {
			clms_process_mbx(&clms_cb->mbx);
		}
#ifdef ENABLE_AIS_PLM
		/*Incase the Immnd restart is not supported fully,have to reint imm - TO Be Done */
		if (clms_cb->reg_with_plm == SA_TRUE){
			if (fds[FD_PLM].revents & POLLIN) {
				if ((error = saPlmDispatch(clms_cb->plm_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
					LOG_ER("saPlmDispatch FAILED: %u", error);
					break;
				}
			}
		}
#endif

		if (clms_cb->immOiHandle && fds[FD_IMM].revents & POLLIN) {
			if ((error = saImmOiDispatch(clms_cb->immOiHandle, SA_DISPATCH_ALL)) != SA_AIS_OK) {
				if (error == SA_AIS_ERR_BAD_HANDLE) {
					TRACE("main :saImmOiDispatch returned BAD_HANDLE");

					/* 
					 * Invalidate the IMM OI handle, this info is used in other
					 * locations. E.g. giving TRY_AGAIN responses to a create and
					 * close app stream requests. That is needed since the IMM OI
					 * is used in context of these functions.
					 * 
					 * Also closing the handle. Finalize is ok with a bad handle
					 * that is bad because it is stale and this actually clears
					 * the handle from internal agent structures.  In any case
					 * we ignore the return value from Finalize here.
					 */
					saImmOiFinalize(clms_cb->immOiHandle);
					clms_cb->immOiHandle = 0;

					/* Initiate IMM reinitializtion in the background */
					clm_imm_reinit_bg(clms_cb);


				} else if (error != SA_AIS_OK) {
					LOG_ER("saImmOiDispatch FAILED: %u", error);
					break;
				}
			}
		}
	}

 done:
	LOG_ER("Failed, exiting...");
	TRACE_LEAVE();
	exit(1);
}

/**
* This dumps the CLMS CB
*/
void clms_cb_dump(void)
{
	CLMS_CLUSTER_NODE *node = NULL;
	CLMS_CLIENT_INFO *client = NULL;
	uns32 client_id = 0;
	SaNameT nodename;

	TRACE("\n***************************************************************************************");

	TRACE("My DB Snapshot \n");
	TRACE("Number of member nodes = %d\n", osaf_cluster->num_nodes);
	TRACE("Async update count = %d\n", clms_cb->async_upd_cnt);
	TRACE("CLMS_CB last client id %u", clms_cb->last_client_id);

	TRACE("Dump nodedb");
	memset(&nodename, '\0', sizeof(SaNameT));

	while ((node = clms_node_getnext_by_name(&nodename)) != NULL) {
		memcpy(&nodename, &node->node_name, sizeof(SaNameT));
		TRACE("Dump Runtime data of the node: %s", node->node_name.value);
		TRACE("Membership status %d", node->member);
		TRACE("Node Id %u", node->node_id);
		TRACE("Init_view %llu", node->init_view);
		TRACE("Admin_state %d", node->admin_state);
		TRACE("Change %d", node->change);
		TRACE("nodeup %d", node->nodeup);
		TRACE("stat_change %d", node->stat_change);
		TRACE("Admin_op %d", node->admin_op);
#ifdef ENABLE_AIS_PLM
		TRACE("EE Readiness State %d", node->ee_red_state);
#endif
	}

	TRACE("Dump Client data");
	while ((client = clms_client_getnext_by_id(client_id)) != NULL) {
		client_id = client->client_id;
		TRACE("Client_id %u", client->client_id);
		TRACE("MDS Dest %llx", client->mds_dest);
		TRACE("Track flags %d", client->track_flags);
	}
	TRACE("\n***********************************************************************************");
}

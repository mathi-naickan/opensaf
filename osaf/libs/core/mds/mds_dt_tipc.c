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

******************************************************************************
*/
#include "mds_dt.h"
#include "mds_log.h"
#include "ncssysf_def.h"
#include "ncssysf_tsk.h"
#include "ncssysf_mem.h"

#include <stdlib.h>
#include <sched.h>
#include <sys/poll.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "mds_dt_tipc.h"

/*
    tipc_id will be <NODE_ID,RANDOM NUMBER>
*/

/* Added for the subscription changes */

/*
 * In the default addressing scheme TIPC addresses will be 1.1.31, 1.1.47.
 * The slot ID is shifted 4 bits up and subslot ID is added in the 4 LSB.
 * When use of subslot ID is disabled (set MDS_USE_SUBSLOT_ID=0 in CFLAGS), the
 * TIPC addresses will be 1.1.1, 1.1.2, etc.
 */

/* Following is defined for use by MDS in TIPC 2.0 as TIPC 2.0 supports only network order */
bool mds_use_network_order = 0;

#define NTOHL(x) (mds_use_network_order?ntohl(x):x)
#define HTONL(x) (mds_use_network_order?htonl(x):x)

static uint32_t mdtm_tipc_check_for_endianness(void);

uint32_t mdtm_tipc_init(NODE_ID node_id, uint32_t *mds_tipc_ref);
uint32_t mdtm_tipc_destroy(void);

static uint32_t mdtm_create_rcv_task(int mdtm_hdle);
static uint32_t mdtm_destroy_rcv_task(void);

static uint32_t mdtm_process_recv_events(void);
static uint32_t mdtm_process_discovery_events(uint32_t flag, struct tipc_event event);

uint32_t mds_mdtm_svc_install_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE policy,
				MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
uint32_t mds_mdtm_svc_uninstall_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				  V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE policy,
				  MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
uint32_t mds_mdtm_svc_subscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				  MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);
uint32_t mds_mdtm_svc_unsubscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, 
				NCSMDS_SCOPE_TYPE install_scope,MDS_SUBTN_REF_VAL subtn_ref_val);
uint32_t mds_mdtm_vdest_install_tipc(MDS_VDEST_ID vdest_id);
uint32_t mds_mdtm_vdest_uninstall_tipc(MDS_VDEST_ID vdest_id);
uint32_t mds_mdtm_vdest_subscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val);
uint32_t mds_mdtm_vdest_unsubscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val);
uint32_t mds_mdtm_tx_hdl_register_tipc(MDS_DEST adest);
uint32_t mds_mdtm_tx_hdl_unregister_tipc(MDS_DEST adest);

uint32_t mds_mdtm_send_tipc(MDTM_SEND_REQ *req);

/* Tipc actual send, can be made as Macro even*/
static uint32_t mdtm_sendto(uint8_t *buffer, uint16_t buff_len, struct tipc_portid tipc_id);

uint32_t mdtm_frag_and_send(MDTM_SEND_REQ *req, uint32_t seq_num, struct tipc_portid id);

static uint32_t mdtm_add_mds_hdr(uint8_t *buffer, MDTM_SEND_REQ *req);

uint16_t mds_checksum(uint32_t length, uint8_t buff[]);

uint32_t mds_mdtm_node_subscribe_tipc(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);
uint32_t mds_mdtm_node_unsubscribe_tipc(MDS_SUBTN_REF_VAL subtn_ref_val);

typedef struct mdtm_tipc_cb {
	int Dsock;
	int BSRsock;

	void *mdtm_hdle_task;
	int hdle_mdtm;
	uint64_t adest;

	SYSF_MBX tmr_mbx;
	int tmr_fd;
	uint32_t node_id;

} MDTM_TIPC_CB;

MDTM_TIPC_CB tipc_cb;

static uint32_t mdtm_tipc_own_node(int fd);

struct sockaddr_tipc topsrv;

#define FOREVER ~0

static MDS_SUBTN_REF_VAL handle;
static uint16_t num_subscriptions;

uint32_t mdtm_global_frag_num;
/*********************************************************

  Function NAME: mdtm_tipc_init

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mdtm_tipc_init(NODE_ID nodeid, uint32_t *mds_tipc_ref)
{
	uint32_t tipc_node_id = 0;
	int flags;

	NCS_PATRICIA_PARAMS pat_tree_params;

	struct sockaddr_tipc addr;
	socklen_t sz = sizeof(addr);

	memset(&tipc_cb, 0, sizeof(tipc_cb));

	/* Added to assist the shutdown bug */
	mdtm_ref_hdl_list_hdr = NULL;
	num_subscriptions = 0;
	handle = 0;
	mdtm_global_frag_num = 0;

	/* REASSEMBLY TREE */
	memset(&pat_tree_params, 0, sizeof(pat_tree_params));
	pat_tree_params.key_size = sizeof(MDTM_REASSEMBLY_KEY);
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&mdtm_reassembly_list, &pat_tree_params)) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC ncs_patricia_tree_init failed MDTM_INIT\n");
		return NCSCC_RC_FAILURE;
	}

	/* Create the sockets required for Binding, Send, receive and Discovery */

	tipc_cb.Dsock = socket(AF_TIPC, SOCK_SEQPACKET, 0);
	if (tipc_cb.Dsock < 0) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Dsock Socket creation failed in MDTM_INIT err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	tipc_cb.BSRsock = socket(AF_TIPC, SOCK_RDM, 0);
	if (tipc_cb.BSRsock < 0) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC BSRsock Socket creation failed in MDTM_INIT err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	flags = fcntl(tipc_cb.Dsock, F_GETFD, 0);
	if ((flags < 0) || (flags > 1)) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Unable to get the CLOEXEC Flag on Dsock err :%s", strerror(errno));
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	} else {
		if (fcntl(tipc_cb.Dsock, F_SETFD, (flags | FD_CLOEXEC)) == (-1)) {
			syslog(LOG_ERR, "MDS:MDTM:TIPC Unable to set the CLOEXEC Flag on Dsock err :%s", strerror(errno));
			close(tipc_cb.Dsock);
			close(tipc_cb.BSRsock);
			return NCSCC_RC_FAILURE;
		}
	}

	flags = fcntl(tipc_cb.BSRsock, F_GETFD, 0);
	if ((flags < 0) || (flags > 1)) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Unable to get the CLOEXEC Flag on BSRsock err :%s", strerror(errno));
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	} else {
		if (fcntl(tipc_cb.BSRsock, F_SETFD, (flags | FD_CLOEXEC)) == (-1)) {
			syslog(LOG_ERR, "MDS:MDTM:TIPC Unable to set the CLOEXEC Flag on BSRsock err :%s", strerror(errno));
			close(tipc_cb.Dsock);
			close(tipc_cb.BSRsock);
			return NCSCC_RC_FAILURE;
		}
	}
	/* End Fix */

	/* Code for getting the self tipc random number */
	memset(&addr, 0, sizeof(addr));
	if (0 > getsockname(tipc_cb.BSRsock, (struct sockaddr *)&addr, &sz)) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Failed to get the BSR Sockname  err :%s", strerror(errno));
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	}
	*mds_tipc_ref = addr.addr.id.ref;

	tipc_cb.adest = ((uint64_t)(nodeid)) << 32;
	tipc_cb.adest |= addr.addr.id.ref;
	tipc_cb.node_id = nodeid;

	tipc_node_id = mdtm_tipc_own_node(tipc_cb.BSRsock);	/* This gets the tipc ownaddress */

	if (tipc_node_id == 0) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Zero tipc_node_id ");
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	}

	mds_use_network_order = mdtm_tipc_check_for_endianness();

	if (mds_use_network_order == NCSCC_RC_FAILURE) {
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	}

	/* Connect to the Topology Server */
	memset(&topsrv, 0, sizeof(topsrv));
	topsrv.family = AF_TIPC;
	topsrv.addrtype = TIPC_ADDR_NAME;
	topsrv.addr.name.name.type = TIPC_TOP_SRV;
	topsrv.addr.name.name.instance = TIPC_TOP_SRV;

	if (0 > connect(tipc_cb.Dsock, (struct sockaddr *)&topsrv, sizeof(topsrv))) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Failed to connect to topology server  err :%s", strerror(errno));
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	}

	/* Code for Tmr Mailbox Creation used for Tmr Msg Retrival */

	if (m_NCS_IPC_CREATE(&tipc_cb.tmr_mbx) != NCSCC_RC_SUCCESS) {
		/* Mail box creation failed */
		syslog(LOG_ERR, "MDS:MDTM:TIPC Tmr Mailbox Creation failed:\n");
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		return NCSCC_RC_FAILURE;
	} else {

		NCS_SEL_OBJ obj;

		/* Code added for attaching the mailbox, to eliminate the DBG Print at sysf_ipc.c: 640 */
		if (NCSCC_RC_SUCCESS != m_NCS_IPC_ATTACH(&tipc_cb.tmr_mbx)) {
			m_NCS_IPC_RELEASE(&tipc_cb.tmr_mbx, NULL);
			syslog(LOG_ERR, "MDS:MDTM:TIPC Tmr Mailbox  Attach failed:\n");
			close(tipc_cb.Dsock);
			close(tipc_cb.BSRsock);
			return NCSCC_RC_FAILURE;
		}

		obj = m_NCS_IPC_GET_SEL_OBJ(&tipc_cb.tmr_mbx);

		/* retreive the corresponding fd for mailbox and fill it in cb */
		tipc_cb.tmr_fd = m_GET_FD_FROM_SEL_OBJ(obj);	/* extract and fill value needs to be extracted */
		mdtm_attach_mbx(tipc_cb.tmr_mbx);
		mdtm_set_transport(MDTM_TX_TYPE_TIPC);
	}

	/* Create a task to receive the events and data */
	if (mdtm_create_rcv_task(tipc_cb.hdle_mdtm) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Receive Task Creation Failed in MDTM_INIT\n");
		close(tipc_cb.Dsock);
		close(tipc_cb.BSRsock);
		m_NCS_IPC_RELEASE(&tipc_cb.tmr_mbx, NULL);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_tipc_check_for_endianness

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  0 - host order
            1 - network order
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t mdtm_tipc_check_for_endianness(void)
{
	struct sockaddr_tipc srv;
	int d_fd;
	struct tipc_event event;

	struct tipc_subscr subs = { {0, 0, ~0},
	~0,
	TIPC_SUB_PORTS,
	{2, 2, 2, 2, 2, 2, 2, 2}
	};

	/* Connect to the Topology Server */
	d_fd = socket(AF_TIPC, SOCK_SEQPACKET, 0);
	if (d_fd < 0) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC D Socket creation failed in mdtm_tipc_check_for_endianness err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	memset(&srv, 0, sizeof(srv));
	srv.family = AF_TIPC;
	srv.addrtype = TIPC_ADDR_NAME;
	srv.addr.name.name.type = TIPC_TOP_SRV;
	srv.addr.name.name.instance = TIPC_TOP_SRV;

	if (0 > connect(d_fd, (struct sockaddr *)&srv, sizeof(srv))) {
		syslog(LOG_ERR, "MDS:MDTM:TIPC Failed to connect to topology server in mdtm_check_for_endianness err :%s", strerror(errno));
		close(d_fd);
		return NCSCC_RC_FAILURE;
	}

	if (send(d_fd, &subs, sizeof(subs), 0) != sizeof(subs)) {
		syslog(LOG_ERR, "failed to send subscription err :%s", strerror(errno));
		close(d_fd);
		return NCSCC_RC_FAILURE;
	}

	while (recv(d_fd, &event, sizeof(event), 0) == sizeof(event)) {
		close(d_fd);
		return 0;
	}
	close(d_fd);

	return 1;
}

/*********************************************************

  Function NAME: mdtm_tipc_destroy

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t mdtm_tipc_destroy(void)
{
	MDTM_REF_HDL_LIST *temp_ref_hdl_list_entry = NULL;
	MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;
	MDTM_REASSEMBLY_KEY reassembly_key;

	/* close sockets first */
	close(tipc_cb.BSRsock);
	close(tipc_cb.Dsock);

	/* Destroy receiving task */
	if (mdtm_destroy_rcv_task() != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: Receive Task Destruction Failed in MDTM_INIT\n");
	}
	/* Destroy mailbox */
	m_NCS_IPC_DETACH(&tipc_cb.tmr_mbx, (NCS_IPC_CB)mdtm_mailbox_mbx_cleanup, NULL);
	m_NCS_IPC_RELEASE(&tipc_cb.tmr_mbx, (NCS_IPC_CB)mdtm_mailbox_mbx_cleanup);

	/* Clear reference hdl list */
	while (mdtm_ref_hdl_list_hdr != NULL) {
		/* Store temporary the pointer of entry to be deleted */
		temp_ref_hdl_list_entry = mdtm_ref_hdl_list_hdr;
		/* point to next entry */
		mdtm_ref_hdl_list_hdr = mdtm_ref_hdl_list_hdr->next;
		/* Free the entry */
		m_MMGR_FREE_HDL_LIST(temp_ref_hdl_list_entry);
	}

	/* Delete the pat tree for the reassembly */
	reassem_queue = (MDTM_REASSEMBLY_QUEUE *)ncs_patricia_tree_getnext(&mdtm_reassembly_list, (uint8_t *)NULL);

	while (NULL != reassem_queue) {
		/* stop timer and free memory */
		m_NCS_TMR_STOP(reassem_queue->tmr);
		m_NCS_TMR_DESTROY(reassem_queue->tmr);

		/* Free tmr_info */
		m_MMGR_FREE_TMR_INFO(reassem_queue->tmr_info);

		/* Destroy Handle */
		ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, reassem_queue->tmr_hdl);

		reassem_queue->tmr_info = NULL;

		/* Free memory Allocated to this msg and MDTM_REASSEMBLY_QUEUE */
		mdtm_free_reassem_msg_mem(&reassem_queue->recv.msg);

		reassembly_key = reassem_queue->key;

		ncs_patricia_tree_del(&mdtm_reassembly_list, (NCS_PATRICIA_NODE *)reassem_queue);
		m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);

		reassem_queue = (MDTM_REASSEMBLY_QUEUE *)ncs_patricia_tree_getnext
		    (&mdtm_reassembly_list, (uint8_t *)&reassembly_key);
	}

	ncs_patricia_tree_destroy(&mdtm_reassembly_list);
	mdtm_ref_hdl_list_hdr = NULL;
	num_subscriptions = 0;
	handle = 0;
	mdtm_global_frag_num = 0;

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_tipc_own_node

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t mdtm_tipc_own_node(int fd)
{
	struct sockaddr_tipc addr;
	socklen_t sz = sizeof(addr);

	memset(&addr, 0, sizeof(addr));
	if (0 > getsockname(tipc_cb.Dsock, (struct sockaddr *)&addr, &sz)) {
		m_MDS_LOG_ERR("MDTM: Failed to get Own Node Address  err :%s", strerror(errno));
		return 0;
	}
	return addr.addr.id.node;
}

/*********************************************************

  Function NAME: mdtm_create_rcv_task

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

static uint32_t mdtm_create_rcv_task(int mdtm_hdle)
{
	/*
	   STEP 1: Create a recv task which will recv data and
	   captures the discovery events as well */

	int policy = SCHED_RR; /*root defaults */
	int max_prio = sched_get_priority_max(policy);
	int min_prio = sched_get_priority_min(policy);
	int prio_val = ((max_prio - min_prio) * 0.87); 
	
	if (m_NCS_TASK_CREATE((NCS_OS_CB)mdtm_process_recv_events,
			      (NCSCONTEXT)(long)mdtm_hdle,
			      (char *)"OSAF_MDS",
			      prio_val, policy, NCS_MDTM_STACKSIZE, &tipc_cb.mdtm_hdle_task) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: Task Creation-failed:\n");
		return NCSCC_RC_FAILURE;
	}

	/* Start the created task,
	 *   if start fails,
	 *        release the task by calling the NCS task release function*/
	if (m_NCS_TASK_START(tipc_cb.mdtm_hdle_task) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: Start of the Created Task-failed:\n");
		m_NCS_TASK_RELEASE(tipc_cb.mdtm_hdle_task);
		m_MDS_LOG_ERR("MDTM: START of created task failed");
		return NCSCC_RC_FAILURE;
	}
	/* return NCS success */
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_destroy_rcv_task

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t mdtm_destroy_rcv_task(void)
{
	if (m_NCS_TASK_STOP(tipc_cb.mdtm_hdle_task) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: Stop of the Created Task-failed:\n");
	}

	if (m_NCS_TASK_RELEASE(tipc_cb.mdtm_hdle_task) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: Stop of the Created Task-failed:\n");
	}

	return NCSCC_RC_SUCCESS;

}

/*********************************************************

  Function NAME: mdtm_process_recv_events

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t mdtm_process_recv_events(void)
{
	/*
	   STEP 1: Poll on the BSRsock and Dsock to get the events
	   if data is received process the received data
	   if discovery events are received , process the discovery events
	 */

	while (1) {
		unsigned int pollres;
		struct pollfd pfd[3];
		struct tipc_event event;

		pfd[0].fd = tipc_cb.Dsock;
		pfd[0].events = POLLIN;
		pfd[1].fd = tipc_cb.BSRsock;
		pfd[1].events = POLLIN;
		pfd[2].fd = tipc_cb.tmr_fd;
		pfd[2].events = POLLIN;

		pfd[0].revents = pfd[1].revents = pfd[2].revents = 0;

		pollres = poll(pfd, 3, MDTM_TIPC_POLL_TIMEOUT);

		if (pollres > 0) {	/* Check for EINTR and discard */
			memset(&event, 0, sizeof(event));
			mds_mutex_lock();
			if (pfd[0].revents == POLLIN) {
				if (recv(tipc_cb.Dsock, &event, sizeof(event), 0) != sizeof(event)) {
					m_MDS_LOG_ERR("Unable to capture the recd event .. Continuing  err :%s", strerror(errno));
					mds_mutex_unlock();
					continue;
				} else {
					if (NTOHL(event.event) == TIPC_PUBLISHED) {

						m_MDS_LOG_INFO("MDTM: Published: ");
						m_MDS_LOG_INFO("MDTM:  <%u,%u,%u> port id <0x%08x:%u>\n",
							       NTOHL(event.s.seq.type), NTOHL(event.found_lower),
							       NTOHL(event.found_upper), NTOHL(event.port.node),
							       NTOHL(event.port.ref));

						if ( NCSCC_RC_SUCCESS != mdtm_process_discovery_events(TIPC_PUBLISHED, 
								event)) {
							m_MDS_LOG_INFO("MDTM: Published Event processing status: F");
						}
					} else if (NTOHL(event.event) == TIPC_WITHDRAWN) {
						m_MDS_LOG_INFO("MDTM: Withdrawn: ");
						m_MDS_LOG_INFO("MDTM:  <%u,%u,%u> port id <0x%08x:%u>\n",
							       NTOHL(event.s.seq.type), NTOHL(event.found_lower),
							       NTOHL(event.found_upper), NTOHL(event.port.node),
							       NTOHL(event.port.ref));

						if ( NCSCC_RC_SUCCESS != mdtm_process_discovery_events(TIPC_WITHDRAWN, 
								event)) {
							m_MDS_LOG_INFO("MDTM: Withdrawn event processing status: F");
						}
					} else if (NTOHL(event.event) == TIPC_SUBSCR_TIMEOUT) {
						/* As the timeout passed in infinite, No need to check for the Timeout */
						m_MDS_LOG_ERR("MDTM: Timeou Event");
						m_MDS_LOG_INFO("MDTM: Timeou Event");
						m_MDS_LOG_INFO("MDTM:  <%u,%u,%u> port id <0x%08x:%u>\n",
							       NTOHL(event.s.seq.type), NTOHL(event.found_lower),
							       NTOHL(event.found_upper), NTOHL(event.port.node),
							       NTOHL(event.port.ref));
					} else {
						m_MDS_LOG_ERR("MDTM: Unknown Event");
						/* This should never come */
						assert(0);
					}
				}
			} else if (pfd[0].revents & POLLHUP) {	/* This value is returned when the number of subscriptions made cross the tipc max_subscr limit, so no more connection to the tipc topserver is present(viz no more up/down events), so abort and exit the process */
				m_MDS_LOG_CRITICAL
				    ("MDTM: POLLHUP returned on Discovery Socket, No. of subscriptions=%d",
				     num_subscriptions);
				abort();	/* This means, the process is use less as
						   it has lost the connectivity with the topology server
						   and will not be able to receive any UP/DOWN events */

			} else if (pfd[1].revents & POLLIN) {

				/* Data Received */

				uint8_t inbuf[MDTM_RECV_BUFFER_SIZE];
				uint8_t *data;	/* Used for decoding */
				uint16_t recd_bytes = 0;
#ifdef MDS_CHECKSUM_ENABLE_FLAG
				uint16_t old_checksum = 0;
				uint16_t new_checksum = 0;
#endif
				struct sockaddr_tipc client_addr;
				socklen_t alen = sizeof(client_addr);

				uint16_t recd_buf_len = 0;
				m_MDS_LOG_INFO("MDTM: Data received: Processing data ");

				recd_bytes = recvfrom(tipc_cb.BSRsock, inbuf, sizeof(inbuf), 0,
						      (struct sockaddr *)&client_addr, &alen);
				if (recd_bytes == 0) {	/* As we had disabled the feature of receving the bounced messages, recd_bytes==0 indicates a fatal condition so abort */
					m_MDS_LOG_CRITICAL
					    ("MDTM: recd bytes=0 on receive sock, fatal condition. Exiting the process  err :%s",strerror(errno) );
					abort();
				}
				data = inbuf;

				recd_buf_len = ncs_decode_16bit(&data);

				if (pfd[1].revents & POLLERR) {
					m_MDS_LOG_ERR("MDTM: Error Recd:tipc_id=<0x%08x:%u>:errno=0x%08x",
						      client_addr.addr.id.node, client_addr.addr.id.ref, errno);
				} else if (recd_buf_len == recd_bytes) {
					uint64_t tipc_id = 0;
					uint32_t buff_dump = 0;
					tipc_id = ((uint64_t)client_addr.addr.id.node) << 32;	/* TIPC_ID=<NODE,REF> */
					tipc_id |= client_addr.addr.id.ref;

#ifdef MDS_CHECKSUM_ENABLE_FLAG
					if (inbuf[2] == 1) {
						old_checksum = ((uint16_t)inbuf[3] << 8 | inbuf[4]);
						inbuf[3] = 0;
						inbuf[4] = 0;
						new_checksum = mds_checksum(recd_bytes, inbuf);

						if (old_checksum != new_checksum) {
							m_MDS_LOG_ERR
							    ("CHECKSUM-MISMATCH:recvd_on_sock=%d, TIPC-ID=0x%016llx, ADEST=<%08x,%u>",
							     recd_bytes, tipc_id,
							     m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(client_addr.addr.
												     id.node),
							     client_addr.addr.id.ref);
							mds_buff_dump(inbuf, recd_bytes, 100);
							abort();
							mds_mutex_unlock();
							continue;
						}
						mdtm_process_recv_data(&inbuf[5], recd_bytes - 5, tipc_id, &buff_dump);
						if (buff_dump) {
							m_MDS_LOG_ERR
							    ("RECV_DATA_PROCESS:recvd_on_sock=%d, TIPC-ID=0x%016llx, ADEST=<%08x,%u>",
							     recd_bytes, tipc_id,
							     m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(client_addr.addr.
												     id.node),
							     client_addr.addr.id.ref);
							mds_buff_dump(inbuf, recd_bytes, 100);
						}
					} else {
						mdtm_process_recv_data(&inbuf[5], recd_bytes - 5, tipc_id, &buff_dump);
					}
#else
					mdtm_process_recv_data(&inbuf[2], recd_bytes - 2, tipc_id, &buff_dump);
#endif
				} else {
					uint64_t tipc_id;
					tipc_id = ((uint64_t)client_addr.addr.id.node) << 32;	/* TIPC_ID=<NODE,REF> */
					tipc_id |= client_addr.addr.id.ref;

					/* Log message that we are dropping the data */
					m_MDS_LOG_ERR
					    ("LEN-MISMATCH:recvd_on_sock=%d, size_in_mds_hdr=%d,  TIPC-ID=0x%016llx, ADEST=<%08x,%u>",
					     recd_bytes, recd_buf_len, tipc_id,
					     m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(client_addr.addr.id.node),
					     client_addr.addr.id.ref);
					mds_buff_dump(inbuf, recd_bytes, 100);
				}
			} else if (pfd[2].revents == POLLIN) {
				m_MDS_LOG_INFO("MDTM: Processing Timer mailbox events\n");

				/* Check if destroy-event has been processed */
				if (mds_tmr_mailbox_processing() == NCSCC_RC_DISABLED) {
					/* Quit ASAP. We have acknowledge that MDS thread can be destroyed. 
					   Note that the destroying thread is waiting for the MDS_UNLOCK, before
					   proceeding with pthread-cancel and pthread-join */
					mds_mutex_unlock();

					/* N O T E : No further system calls etc. This is to ensure that the
					   pthread-cancel & pthread-join, do not get blocked. */
					return NCSCC_RC_SUCCESS;	/* Thread quit */
				}
			}
			mds_mutex_unlock();
		}		/* if pollres */
	}			/* while */
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_process_discovery_events

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

static uint32_t mdtm_process_discovery_events(uint32_t discovery_event, struct tipc_event event)
{
	/*
	   STEP 1: Extract the type of  event name such as VDEST, SVC, PCON, NODE or Process.
	   STEP 2: Depending upon the discovery_event call the respective "xx_up" or  "xx_down" functions

	   if(discovery_event==TIPC_PUBLISHED)
	   {
	   if(MDS_TIPC_NAME_TYPE == SVC_INST_TYPE)
	   {
	   STEP 1: If an entry with TX_HDL = TIPC_ID doesnt exist in ADEST_INFO_TABLE
	   then add an entry.

	   STEP 2: Call mds_mcm_svc_up(pwe_id, svc_id, role, scope, vdest, MDS_DEST adest)
	   }
	   else if (MDS_TIPC_NAME_TYPE == PCON_ID_DETAILS )
	   {

	   STEP 1: Retrieve ADEST, PCON_ID and TIPC_ID from Publish event

	   STEP 2: Update ADEST_INFO-TABLE with the parameters
	   }
	   else if (MDS_TIPC_NAME_TYPE = VDEST_INST_TYPE)
	   {
	   STEP 1: If with ID VDEST_ ID is present in LOCAL-VDEST table
	   then call mds_mcm_vdest_up(vdest_id)
	   }
	   * SIM ONLY *
	   else if (MDS_TIPC_NAME_TYPE == NODE_ID_DETAILS)
	   {
	   STEP 1: Retrieve NODE_ID and store in ADEST-INFO Table
	   }
	   }
	   else if (discovery_event==TIPC_WITHDRAWN)
	   {
	   if (MDS_TIPC_NAME_TYPE == SVC_INST_TYPE )
	   {
	   STEP 1: If an entry with TX_HDL = TIPC_ID exists in ADEST_INFO_TABLE.
	   Call mds_mcm_svc_down(pwe_id, svc_id, role, scope, vdest, MDS_DEST adest)
	   }
	   else if (MDS_TIPC_NAME_TYPE == PCON_ID_DETAILS )
	   {
	   Remove Entry from the ADEST-INFO-TABLE matching the ADEST,PCON and TIPC_ID
	   }
	   else if (MDS_TIPC_NAME_TYPE = VDEST_INST_TYPE)
	   {
	   * DISCARD : As remote VDEST need to know only Multiple Active
	   VDEST coming up in system *
	   }
	   * SIM ONLY *
	   else if (MDS_TIPC_NAME_TYPE = NODE_ID_DETAILS)
	   {
	   Remove Entry from the ADEST-INFO-TABLE matching the ADEST, NODE_ID and TIPC_ID
	   }
	   }
	 */
	uint32_t inst_type = 0, type = 0, lower = 0, node = 0, ref = 0;

	MDS_DEST adest = 0;
	MDS_SUBTN_REF_VAL subtn_ref_val;

	type = NTOHL(event.s.seq.type);
	lower = NTOHL(event.found_lower);
	node = NTOHL(event.port.node);
	ref = NTOHL(event.port.ref);
	subtn_ref_val = *((uint64_t *)(event.s.usr_handle));

	inst_type = type & 0x00ff0000;	/* To get which type event
					   either SVC,VDEST, PCON, NODE OR process */
	switch (inst_type) {
	case MDS_SVC_INST_TYPE:
		{
			PW_ENV_ID pwe_id;
			MDS_SVC_ID svc_id;
			V_DEST_RL role;
			NCSMDS_SCOPE_TYPE scope;
			MDS_VDEST_ID vdest;
			NCS_VDEST_TYPE policy = 0;
			MDS_SVC_HDL svc_hdl;

			MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver;
			MDS_SVC_ARCHWORD_TYPE archword_type;

			uint32_t node_status = 0;

			svc_id = (uint16_t)(type & MDS_EVENT_MASK_FOR_SVCID);
			vdest = (MDS_VDEST_ID)(lower);
			archword_type =
			    (MDS_SVC_ARCHWORD_TYPE)((lower & MDS_ARCHWORD_MASK) >> (LEN_4_BYTES -
										    MDS_ARCHWORD_BITS_LEN));
			svc_sub_part_ver =
			    (MDS_SVC_PVT_SUB_PART_VER)((lower & MDS_VER_MASK) >> (LEN_4_BYTES -
										  MDS_VER_BITS_LEN -
										  MDS_ARCHWORD_BITS_LEN));
			pwe_id = (type >> MDS_EVENT_SHIFT_FOR_PWE) & MDS_EVENT_MASK_FOR_PWE;
			policy =
			    (lower & MDS_POLICY_MASK) >> (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN -
							  MDS_VER_BITS_LEN - VDEST_POLICY_LEN);
			role =
			    (lower & MDS_ROLE_MASK) >> (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN -
							MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN);
			scope =
			    (lower & MDS_SCOPE_MASK) >> (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN -
							 MDS_VER_BITS_LEN - VDEST_POLICY_LEN -
							 ACT_STBY_LEN - MDS_SCOPE_LEN);

			m_MDS_LOG_INFO("MDTM: Received SVC event");

			if (NCSCC_RC_SUCCESS != mdtm_get_from_ref_tbl(subtn_ref_val, &svc_hdl)) {
				m_MDS_LOG_INFO("MDTM: No entry in ref tbl so dropping the recd event");
				return NCSCC_RC_FAILURE;
			}

			if (role == 0)
				role = V_DEST_RL_ACTIVE;
			else
				role = V_DEST_RL_STANDBY;

			if (policy == 0)
				policy = NCS_VDEST_TYPE_MxN;
			else
				policy = NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN;

			if (scope == 0)
				scope = NCSMDS_SCOPE_NONE;
			else if (scope == 1)
				scope = NCSMDS_SCOPE_INTRANODE;
			else if (scope == 3)
				scope = NCSMDS_SCOPE_INTRACHASSIS;
			else {
				m_MDS_LOG_ERR("MDTM: SVC Scope Not supported");
				return NCSCC_RC_FAILURE;
			}

			node_status = m_MDS_CHECK_TIPC_NODE_ID_RANGE(node);

			if (NCSCC_RC_SUCCESS == node_status) {
				adest = ((((uint64_t)(m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(node))) << 32) | ref);
			} else {
				m_MDS_LOG_ERR
				    ("MDTM: Dropping  the svc event for SVC id = %d, subscribed by SVC id = %d as the TIPC NODEid is not in the prescribed range=0x%08x, SVC Event type=%d",
				     svc_id, m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), node, discovery_event);
				return NCSCC_RC_FAILURE;
			}

			if (TIPC_PUBLISHED == discovery_event) {
				m_MDS_LOG_NOTIFY
				    ("MDTM: svc up event for SVCid =%d, subscri. by SVCid =%d pwe_id=%d adest=%016llx",
				     svc_id, m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), pwe_id, adest);

				if (NCSCC_RC_SUCCESS != mds_mcm_svc_up(pwe_id, svc_id, role, scope,
								       vdest, policy, adest, 0, svc_hdl, subtn_ref_val,
								       svc_sub_part_ver, archword_type)) {
					m_MDS_LOG_ERR
					    ("SVC-UP Event processsing failed for SVC id = %d, subscribed by SVC id = %d",
					     svc_id, m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl));
					return NCSCC_RC_FAILURE;
				}
				return NCSCC_RC_SUCCESS;
			} else if (TIPC_WITHDRAWN == discovery_event) {
				m_MDS_LOG_NOTIFY
				    ("MDTM: svc down event for SVCid =%d, subscri. by SVCid =%d pwe_id=%d adest=%016llx",
				     svc_id, m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), pwe_id, adest);

				if (NCSCC_RC_SUCCESS != mds_mcm_svc_down(pwe_id, svc_id, role, scope,
									 vdest, policy, adest, 0, svc_hdl,
									 subtn_ref_val, svc_sub_part_ver,
									 archword_type)) {
					m_MDS_LOG_ERR
					    ("MDTM: SVC-DOWN Event processsing failed for SVC id = %d, subscribed by SVC id = %d\n",
					     svc_id, m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl));
					return NCSCC_RC_FAILURE;
				}
				return NCSCC_RC_SUCCESS;
			} else {
				m_MDS_LOG_INFO
				    ("MDTM: TIPC EVENT UNSUPPORTED for SVC (other than Publish and Withdraw)\n");
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case MDS_VDEST_INST_TYPE:
		{
			MDS_VDEST_ID vdest_id;
			uint32_t node_status = 0;

			vdest_id = (uint16_t)lower;

			node_status = m_MDS_CHECK_TIPC_NODE_ID_RANGE(node);

			if (NCSCC_RC_SUCCESS == node_status) {
				adest = ((((uint64_t)(m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(node))) << 32) | ref);
			} else {
				m_MDS_LOG_ERR
				    ("MDTM: Dropping  the vdest event, vdest id = %d, as the TIPC NODEid is not in the prescribed range=0x%08x,  Event type=%d",
				     vdest_id, node, discovery_event);
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDTM: Received VDEST event");

			if (TIPC_PUBLISHED == discovery_event) {
				m_MDS_LOG_INFO("MDTM: Raising the VDEST UP event for VDEST id = %d", vdest_id);
				if (NCSCC_RC_SUCCESS != mds_mcm_vdest_up(vdest_id, adest)) {
					m_MDS_LOG_ERR("MDTM: VDEST UP event processing failed for VDEST id = %d",
						      vdest_id);
					return NCSCC_RC_FAILURE;
				}
				/* Return from here */
				return NCSCC_RC_SUCCESS;
			} else if (TIPC_WITHDRAWN == discovery_event) {
				m_MDS_LOG_INFO("MDTM: Raising the VDEST DOWN event for VDEST id = %d", vdest_id);
				if (NCSCC_RC_SUCCESS != mds_mcm_vdest_down(vdest_id, adest)) {
					m_MDS_LOG_ERR("MDTM: VDEST DOWN event processing failed");
					return NCSCC_RC_FAILURE;
				}
				return NCSCC_RC_SUCCESS;
			} else {
				m_MDS_LOG_INFO
				    ("MDTM: TIPC EVENT UNSUPPORTED for VDEST(other than Publish and Withdraw)\n");
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case MDS_NODE_INST_TYPE:
		{
			MDS_SVC_HDL svc_hdl;
			uint32_t node_status = 0;
			NODE_ID node_id = 0;

			m_MDS_LOG_INFO("MDTM: Received NODE event");

			node_status = m_MDS_CHECK_TIPC_NODE_ID_RANGE(node);

			if (NCSCC_RC_SUCCESS == node_status) {
				node_id = ((NODE_ID)(m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(node)));

			} else {
				m_MDS_LOG_ERR
				    ("MDTM: Dropping  the node event,as the TIPC NODEid is not in the prescribed range=0x%08x,  Event type=%d",
				     node, discovery_event);
				return NCSCC_RC_FAILURE;
			}

			if (NCSCC_RC_SUCCESS != mdtm_get_from_ref_tbl(subtn_ref_val, &svc_hdl)) {
				m_MDS_LOG_INFO("MDTM: No entry in ref tbl so dropping the recd event");
				return NCSCC_RC_FAILURE;
			}

			if (TIPC_PUBLISHED == discovery_event) {
				m_MDS_LOG_INFO("MDTM: Raising the NODE UP event for NODE id = %d", node_id);
				return mds_mcm_node_up(svc_hdl, node_id);
			} else if (TIPC_WITHDRAWN == discovery_event) {
				m_MDS_LOG_INFO("MDTM: Raising the NODE DOWN event for NODE id = %d", node_id);
				return mds_mcm_node_down(svc_hdl, node_id);
			} else {
				m_MDS_LOG_INFO
				    ("MDTM: TIPC EVENT UNSUPPORTED for Node (other than Publish and Withdraw)\n");
				return NCSCC_RC_FAILURE;
			}

		}
		break;

	default:
		{
			m_MDS_LOG_ERR
			    ("MDTM: TIPC EVENT UNSUPPORTED (default). If this case comes this should assert as there no other events being processed");
			return NCSCC_RC_FAILURE;
		}
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_free_reassem_msg_mem

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
		/* Presently doing nothing */

/*********************************************************

  Function NAME: mdtm_process_recv_data

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
	/*
	   Get the MDS Header from the data received
	   if Destination SVC exists
	   if the message is a fragmented one
	   reassemble  the recd data(is done based on the received data <Sequence number, TIPC_ID>)
	   An entry for the matching TIPC_ID is serached in the MDS_ADEST_LIST

	   if found
	   reasesemble is done on the MDS_ADEST_LIST
	   if not found
	   an entry is created and reasesemble is done on the MDS_ADEST_LIST
	   A timer is also started on the reassembly queue (to prevent large timegaps between the
	   fragemented packets)
	   Send the data to the upper
	   (Check the role of Destination Service
	   for some send types before giving the data to upper layer)
	 */

	/* If the data is recd over here, it means its a fragmented or non-fragmented pkt) */

	/* Added for seq number check */

		/* We got a fragmented pkt, reassemble */
		/* Check in reasssebly queue whether any pkts are present */

		/* Checking in reassembly queue */

				/* Check whether we recd the last pkt */
					/* Last frag in the message recd */

					/* Free memory Allocated to this msg and MDTM_REASSEMBLY_QUEUE */

					/* Destroy Handle */

					/* stop timer and free memory */

					/* Delete entry from MDTM_REASSEMBLY_QUEUE */

				/* Check SVC_hdl role here */
					/* fUNTION WILL RETURN success when svc is in active or quiesced */

								/* Free the queued data depending on the data type */

								/* Destroy Handle */
								/* stop timer and free memory */

								/* Delete this entry from Global reassembly queue */

				/* Enqueue the data at the End depending on the data type */

				/* Reassemble and send to upper layer if last pkt */
					/* Last frag in the message recd */
					/* Total Data reassembled */

					/* Destroy Handle */

					/* stop timer and free memory */

					/* Depending on the msg type if flat or full enc apply dec space, for setting the uba to decode by user */
					/* for direct buff and cpy encoding modes we do nothig */

					/* Now delete this entry from Global reassembly queue */

					/* Incrementing for next frag */
				/* fragment recd is not next fragment */

					/* Last frag in the message recd */

					/* Free memory Allocated to UB and MDTM_REASSEMBLY_QUEUE */

					/* Destroy Handle */

					/* stop timer and free memory */

					/* Delete entry from MDTM_REASSEMBLY_QUEUE */

			/* Some stale message, Log and Drop */

/*********************************************************

  Function NAME: mdtm_process_recv_message_common

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

		/* We receive buffer pointer starting from MDS HDR only */

		/* Added for seqnum check */

		/* Length Checks */

		/* Check whether total lenght of message is not less than MDS header length */
		/* Check whether mds header length received is not less than mds header length of this instance */

		/* Search whether the Destination exists or not , SVC,PWE,VDEST */

		/* Allocate the memory for reassem_queue */

			/* do nothing */

			/* NOTE: Version in ACK message is ignored */
				/* Size of Payload data in ACK message should be zero, If not its an error */
			/* only for non ack cases */

			/* Drop message if version is not 1 and return */
			/*if(reassem_queue->recv.msg_fmt_ver!=1) 
			   {
			   m_MDS_LOG_ERR("MDTM: Unable to process the recd message due to wrong version=%d",reassem_queue->recv.msg_fmt_ver);
			   m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);
			   return NCSCC_RC_FAILURE;
			   } */

			/* Depending on msg type if flat or full enc apply dec space, for setting the uba to decode by user */
			/* for direct buff and cpy encoding modes we do nothig */

		/* Call upper layer */

		/* Free Memory allocated to this structure */

		/*
		   Message seq no. (32-bit) debug assist) | MoreFrags(1-bit)|
		   Fragment Number(15-bit) | Fragment Size(16-bit)
		 */

		/* Added for seqnum check */

		/* Length Checks */

		/* Check whether total lenght of message is not less than or equal to MDS header length and MDTM frag header length */
		/* Check whether mds header length received is not less than mds header length of this instance */

		/* Search whether the Destination exists or not , SVC,PWE,VDEST */

			/* do nothing */

		/*start the timer */

/*********************************************************

  Function NAME: mdtm_fill_data

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
		/* We will never reach here */
		/* Nothing done here */

/*********************************************************

  Function NAME: mdtm_check_reassem_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
	/*
	   STEP 1: Check whether an entry is present with the seq_num and id,
	   If present
	   return the Pointer
	   else
	   return Null
	 */

/*********************************************************

  Function NAME: mdtm_add_reassemble_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
	/*
	   STEP 1: create an entry in the reassemble queue with parameters as seq_num and id,
	   return the Pointer to the reassembly queue
	 */

	/* Allocate Memory for reassem_queue */

/*********************************************************

  Function NAME: mdtm_del_reassemble_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
	/*
	   STEP 1: Check whether an entry is present with the seq_num and id,
	   If present
	   delete the node
	   return success
	   else
	   return failure
	 */

/*********************************************************

  Function NAME: mds_mdtm_svc_install_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t mds_mdtm_svc_install_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE policy,
				MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	/*
	   STEP 1: Bind to socket BSRSock with Tipc name sequence
	   TIPC Name:
	   <MDS-prefix, SVC_INST_TYPE, svcid>
	   TIPC Range:
	   <pwe,scope,ROLE=role ,vdest> to
	   <pwe,scope,ROLE=role ,vdest>
	 */
	struct sockaddr_tipc server_addr;
	uint32_t server_type = 0, server_inst = 0;
	MDS_SVC_ARCHWORD_TYPE archword = MDS_SELF_ARCHWORD;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	if (svc_id == NCSMDS_SVC_ID_AVND) {
		int imp = 0;
		imp = TIPC_HIGH_IMPORTANCE;

		if (setsockopt(tipc_cb.BSRsock, SOL_TIPC, TIPC_IMPORTANCE, &imp, sizeof(imp)) != 0) {
			m_MDS_LOG_ERR("MDTM: Can't set socket option TIPC_IMP err :%s\n", strerror(errno));
			assert(0);
		} else {
			m_MDS_LOG_INFO("MDTM: Successfully set socket option TIPC_IMP, SVC_ID=%d", svc_id);
		}
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.family = AF_TIPC;
	server_addr.addrtype = TIPC_ADDR_NAMESEQ;

	server_type = server_type | MDS_TIPC_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;
	server_inst |= (uint32_t)((archword) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN));	/* Upper  4  bits */
	server_inst |= (uint32_t)((mds_svc_pvt_ver) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN));	/* next 8  Bits */

	if (policy == NCS_VDEST_TYPE_MxN) {
		policy = 0;
		policy = 0 & 0x1;
	} else {
		policy = 0;
		policy = 1 & 0x1;
	}

	server_inst |= (uint32_t)((policy & 0x1) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN));	/* Next 1 bit */

	if (role == V_DEST_RL_ACTIVE) {
		role = 0;
	} else
		role = 1;

	server_inst |= (uint32_t)((role & 0x1) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN));	/* Next 1 bit */

	if (install_scope == NCSMDS_SCOPE_NONE) {
		server_addr.scope = TIPC_CLUSTER_SCOPE;
		install_scope = 0;
	} else if (install_scope == NCSMDS_SCOPE_INTRANODE) {
		server_addr.scope = TIPC_NODE_SCOPE;
		install_scope = 1;
	} else if (install_scope == NCSMDS_SCOPE_INTRACHASSIS) {
		server_addr.scope = TIPC_CLUSTER_SCOPE;
		install_scope = 3;
	}

	server_inst |= (uint32_t)((install_scope & 0x3) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN - MDS_SCOPE_LEN));	/* Next 2  bit */
	server_inst |= vdest_id;

	server_addr.addr.nameseq.type = server_type;
	server_addr.addr.nameseq.lower = server_inst;
	server_addr.addr.nameseq.upper = server_inst;

	m_MDS_LOG_INFO("MDTM: install_tipc : <%u,%u,%u>", server_type, server_inst, server_inst);

	if (0 != bind(tipc_cb.BSRsock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		m_MDS_LOG_ERR("MDTM: SVC-INSTALL Failure err :%s\n", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	m_MDS_LOG_NOTIFY("MDTM: install_tipc : svc id=%d, vdest=%d", svc_id, vdest_id);
	m_MDS_LOG_INFO("MDTM: SVC-INSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_svc_uninstall_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_svc_uninstall_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				  V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE policy,
				  MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	/*
	   STEP 1: If MDEST=VDEST
	   NEWROLE(of VDEST)=Backup/Quiesced
	   Else
	   NEWROLE =Active

	   STEP 2: Unbind to socket BSRSock with Tipc name sequence
	   TIPC Name:
	   <MDS-prefix, SVC_INST_TYPE, svcid>
	   TIPC Range:
	   <pwe,scope,NEWROLE,vdest>
	   to
	   <pwe,scope,NEWROLE,vdest>

	 */
	uint32_t server_inst = 0, server_type = 0;
	MDS_SVC_ARCHWORD_TYPE archword = MDS_SELF_ARCHWORD;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	struct sockaddr_tipc server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_type = server_type | MDS_TIPC_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;
	server_inst |= (uint32_t)((archword) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN));	/* Upper 4 Bits */
	server_inst |= (uint32_t)((mds_svc_pvt_ver) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN));	/* next 8 Bits */

	if (policy == NCS_VDEST_TYPE_MxN) {
		policy = 0;
	} else {
		policy = 1;
	}

	server_inst |= (uint32_t)((policy & 0x1) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN));	/* Next 1 bit */

	if (role == V_DEST_RL_ACTIVE) {
		role = 0;
	} else
		role = 1;

	server_inst |= (uint32_t)((role & 0x1) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN));	/* Next 1 bit */

	if (install_scope == NCSMDS_SCOPE_NONE) {
		install_scope = 0;
		server_addr.scope = -TIPC_CLUSTER_SCOPE;
	} else if (install_scope == NCSMDS_SCOPE_INTRANODE) {
		install_scope = 1;
		server_addr.scope = -TIPC_NODE_SCOPE;
	} else if (install_scope == NCSMDS_SCOPE_INTRACHASSIS) {
		install_scope = 3;
		server_addr.scope = -TIPC_CLUSTER_SCOPE;
	}

	server_inst |= (uint32_t)((install_scope & 0x3) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN - MDS_SCOPE_LEN));	/* Next 2  bit */

	server_inst |= vdest_id;

	m_MDS_LOG_INFO("MDTM: uninstall_tipc : <%u,%u,%u>", server_type, server_inst, server_inst);

	server_addr.family = AF_TIPC;
	server_addr.addrtype = TIPC_ADDR_NAMESEQ;
	server_addr.addr.nameseq.type = server_type;
	server_addr.addr.nameseq.lower = server_inst;
	server_addr.addr.nameseq.upper = server_inst;

	if (0 != bind(tipc_cb.BSRsock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		m_MDS_LOG_ERR("MDTM: SVC-UNINSTALL Failure err :%s \n", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	m_MDS_LOG_NOTIFY("MDTM: uninstall_tipc : svc id=%d,vdest id=%d", svc_id, vdest_id);
	m_MDS_LOG_INFO("MDTM: SVC-UNINSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_svc_subscribe_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_svc_subscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				  MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	/*
	   STEP 1: Subscribe to socket DSock with TIPC name sequence
	   TIPC Name:
	   <MDS-prefix, SVC_INST, svcid>
	   TIPC Range:
	   <pwe, scope,ROLE=0, 0x0000>
	   to
	   < pwe, scope, ROLE=1, 0xffff >
	   STEP 2: Fill the ref val

	 */
	uint32_t server_type = 0, status = 0;
	struct tipc_subscr subscr;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	if (num_subscriptions > MAX_SUBSCRIPTIONS) {
		m_MDS_LOG_ERR("MDTM: SYSTEM CRITICAL Crossing =%d subscriptions\n", num_subscriptions);
		if (num_subscriptions > MAX_SUBSCRIPTIONS_RETURN_ERROR) {
			m_MDS_LOG_ERR
			    ("MDTM: SYSTEM has crossed the max =%d subscriptions , Returning failure to the user",
			     MAX_SUBSCRIPTIONS_RETURN_ERROR);
			return NCSCC_RC_FAILURE;
		}
	}

	server_type = server_type | MDS_TIPC_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;

	memset(&subscr, 0, sizeof(subscr));
	subscr.seq.type = HTONL(server_type);
	subscr.seq.lower = HTONL(0x00000000);
	subscr.seq.upper = HTONL(0xffffffff);
	subscr.timeout = HTONL(FOREVER);
	subscr.filter = HTONL(TIPC_SUB_PORTS);
	*subtn_ref_val = 0;
	*subtn_ref_val = ++handle;
	*((uint64_t *)subscr.usr_handle) = *subtn_ref_val;

	if (send(tipc_cb.Dsock, &subscr, sizeof(subscr), 0) != sizeof(subscr)) {
		m_MDS_LOG_ERR("MDTM: SVC-SUBSCRIBE Failure err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	status = mdtm_add_to_ref_tbl(svc_hdl, *subtn_ref_val);

	++num_subscriptions;
	m_MDS_LOG_INFO("MDTM: SVC-SUBSCRIBE Success\n");

	return status;
}

/*********************************************************

  Function NAME: mds_mdtm_node_subscribe_tipc

  DESCRIPTION: Subscription to Node UP and DOWN events

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_node_subscribe_tipc(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	struct tipc_subscr net_subscr;
	uint32_t status = NCSCC_RC_SUCCESS;

	m_MDS_LOG_INFO("MDTM: In mds_mdtm_node_subscribe_tipc\n");
	memset(&net_subscr, 0, sizeof(net_subscr));
	net_subscr.seq.type = HTONL(0);
	net_subscr.seq.lower = HTONL(0);
	net_subscr.seq.upper = HTONL(~0);
	net_subscr.timeout = HTONL(FOREVER);
	net_subscr.filter = HTONL(TIPC_SUB_PORTS);
	*subtn_ref_val = 0;
	*subtn_ref_val = ++handle;
	*((uint64_t *)net_subscr.usr_handle) = *subtn_ref_val;

	if (send(tipc_cb.Dsock, &net_subscr, sizeof(net_subscr), 0) != sizeof(net_subscr)) {
		syslog(LOG_ERR, "failed to send network subscription err :%s", strerror(errno));
		m_MDS_LOG_ERR("MDTM: NODE-SUBSCRIBE Failure\n");
		return NCSCC_RC_FAILURE;
	}

	status = mdtm_add_to_ref_tbl(svc_hdl, *subtn_ref_val);
	++num_subscriptions;
	m_MDS_LOG_INFO("MDTM: NODE-SUBSCRIBE Success\n");

	return status;
}

/*********************************************************

  Function NAME: mds_mdtm_node_unsubscribe_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t mds_mdtm_node_unsubscribe_tipc(MDS_SUBTN_REF_VAL subtn_ref_val)
{
#ifdef TIPC_SUB_CANCEL
	struct tipc_subscr net_subscr;

	memset(&net_subscr, 0, sizeof(net_subscr));
	net_subscr.seq.type = HTONL(0);
	net_subscr.seq.lower = HTONL(0);
	net_subscr.seq.upper = HTONL(~0);
	net_subscr.timeout = HTONL(FOREVER);
	net_subscr.filter = HTONL(TIPC_SUB_CANCEL | TIPC_SUB_PORTS);
	*((uint64_t *)net_subscr.usr_handle) = subtn_ref_val;

	if (send(tipc_cb.Dsock, &net_subscr, sizeof(net_subscr), 0) != sizeof(net_subscr)) {
		perror("failed to send network subscription");
		m_MDS_LOG_ERR("MDTM: NODE-UNSUBSCRIBE Failure\n");
		return NCSCC_RC_FAILURE;
	}
	--num_subscriptions;
#endif
	m_MDS_LOG_INFO("MDTM: In mds_mdtm_node_unsubscribe_tipc\n");
	/* Presently TIPC doesnt supports the unsubscribe */
	mdtm_del_from_ref_tbl(subtn_ref_val);
	m_MDS_LOG_INFO("MDTM: NODE-UNSUBSCRIBE Success\n");
	return NCSCC_RC_SUCCESS;

}

/*********************************************************

  Function NAME: mds_mdtm_svc_unsubscribe_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t mds_mdtm_svc_unsubscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, 
			NCSMDS_SCOPE_TYPE install_scope, MDS_SUBTN_REF_VAL subtn_ref_val)
{
	/*
	   STEP 1: Get ref_val and call the TIPC unsubscribe with the ref_val
	 */
#ifdef TIPC_SUB_CANCEL
	uint32_t server_type = 0;
	struct tipc_subscr subscr;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	server_type = server_type | MDS_TIPC_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;

	memset(&subscr, 0, sizeof(subscr));
	subscr.seq.type = HTONL(server_type);
	subscr.seq.lower = HTONL(0x00000000);
	subscr.seq.upper = HTONL(0xffffffff);
	subscr.timeout = HTONL(FOREVER);
	subscr.filter = HTONL(TIPC_SUB_CANCEL | TIPC_SUB_PORTS);
	*((uint64_t *)subscr.usr_handle) = subtn_ref_val;

	if (send(tipc_cb.Dsock, &subscr, sizeof(subscr), 0) != sizeof(subscr)) {
		m_MDS_LOG_ERR("MDTM: SVC-UNSUBSCRIBE Failure\n");
		return NCSCC_RC_FAILURE;
	}
	--num_subscriptions;
#endif
	mdtm_del_from_ref_tbl(subtn_ref_val);

	/* Presently 1.5 doesnt supports the unsubscribe */
	m_MDS_LOG_INFO("MDTM: SVC-UNSUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;

}

/*********************************************************
  Function NAME: mds_mdtm_vdest_install_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_vdest_install_tipc(MDS_VDEST_ID vdest_id)
{
	/*
	   STEP 1: Bind to socket BSRSock with Tipc name sequence
	   TIPC Name:
	   <MDS-prefix, VDEST_INST_TYPE, 0>
	   TIPC Range:
	   <0,ROLE=0,POLICY=0,VDEST_ID > to
	   <0,ROLE=0,POLICY=0,VDEST_ID >
	 */
	struct sockaddr_tipc server_addr;
	uint32_t server_type = 0, server_inst = 0;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.family = AF_TIPC;

	server_addr.addrtype = TIPC_ADDR_NAMESEQ;

	server_type = server_type | MDS_TIPC_PREFIX | MDS_VDEST_INST_TYPE;
	server_inst |= vdest_id;

	server_addr.addr.nameseq.type = server_type;
	server_addr.addr.nameseq.lower = server_inst;
	server_addr.addr.nameseq.upper = server_inst;
	server_addr.scope = TIPC_CLUSTER_SCOPE;

	if (0 != bind(tipc_cb.BSRsock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		m_MDS_LOG_ERR("MDTM: VDEST-INSTALL Failure err :%s\n", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	m_MDS_LOG_INFO("MDTM: VDEST-INSTALL Success\n");
	return NCSCC_RC_SUCCESS;

}

/*********************************************************

  Function NAME: mds_mdtm_vdest_uninstall_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_vdest_uninstall_tipc(MDS_VDEST_ID vdest_id)
{
	/*
	   STEP 1: Unbind to socket BSRSock with Tipc name sequence
	   TIPC Name:
	   <MDS-prefix, VDEST_INST_TYPE, 0>
	   TIPC Range:
	   <0,ROLE=0,POLICY=0,VDEST_ID > to
	   <0,ROLE=0,POLICY=0,VDEST_ID >
	 */

	uint32_t server_inst = 0, server_type = 0;
	struct sockaddr_tipc server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_type = server_type | MDS_TIPC_PREFIX | MDS_VDEST_INST_TYPE;
	server_inst |= vdest_id;

	server_addr.family = AF_TIPC;
	server_addr.addrtype = TIPC_ADDR_NAMESEQ;
	server_addr.addr.nameseq.type = server_type;
	server_addr.addr.nameseq.lower = server_inst;
	server_addr.addr.nameseq.upper = server_inst;
	server_addr.scope = -TIPC_CLUSTER_SCOPE;
	if (0 != bind(tipc_cb.BSRsock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		m_MDS_LOG_ERR("MDTM: VDEST-UNINSTALL Failure err :%s\n", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	m_MDS_LOG_INFO("MDTM: VDEST-UNINSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_subscribe_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_vdest_subscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	/*
	   STEP 1: Subscribe to socket DSock with Tipc name sequence
	   TIPC Name:
	   <MDS-prefix, VDEST_INST_TYPE, 0>
	   TIPC Range:
	   <0,ROLE=0,POLICY=0,VDEST_ID > to
	   <0,ROLE=0,POLICY=0,VDEST_ID >
	 */

	uint32_t inst = 0, server_type = 0;
	struct tipc_subscr subscr;

	if (num_subscriptions > MAX_SUBSCRIPTIONS) {
		m_MDS_LOG_ERR("MDTM: SYSTEM CRITICAL Crossing =%d subscriptions\n", num_subscriptions);
		if (num_subscriptions > MAX_SUBSCRIPTIONS_RETURN_ERROR) {
			m_MDS_LOG_ERR
			    ("MDTM: SYSTEM has crossed the max =%d subscriptions , Returning failure to the user",
			     MAX_SUBSCRIPTIONS_RETURN_ERROR);
			return NCSCC_RC_FAILURE;
		}
	}

	server_type = server_type | MDS_TIPC_PREFIX | MDS_VDEST_INST_TYPE;
	inst |= vdest_id;
	memset(&subscr, 0, sizeof(subscr));
	subscr.seq.type = HTONL(server_type);
	subscr.seq.lower = HTONL(inst);
	subscr.seq.upper = HTONL(inst);
	subscr.timeout = HTONL(FOREVER);
	subscr.filter = HTONL(TIPC_SUB_PORTS);
	*subtn_ref_val = 0;
	*subtn_ref_val = ++handle;
	*((uint64_t *)subscr.usr_handle) = *subtn_ref_val;

	if (send(tipc_cb.Dsock, &subscr, sizeof(subscr), 0) != sizeof(subscr)) {
		m_MDS_LOG_ERR("MDTM: VDEST-SUBSCRIBE Failure\n");
		return NCSCC_RC_FAILURE;
	}
	++num_subscriptions;

	m_MDS_LOG_INFO("MDTM: VDEST-SUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_unsubscribe_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mds_mdtm_vdest_unsubscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val)
{
	/*
	   STEP 1: Get ref_val and call the TIPC unsubscribe with the ref_val
	 */
	/* Fix me, Presently unsupported */
	/* Presently 1.5 doesnt supports the unsubscribe */
#ifdef TIPC_SUB_CANCEL
	uint32_t inst = 0, server_type = 0;
	struct tipc_subscr subscr;

	server_type = server_type | MDS_TIPC_PREFIX | MDS_VDEST_INST_TYPE;
	inst |= vdest_id;
	memset(&subscr, 0, sizeof(subscr));
	subscr.seq.type = HTONL(server_type);
	subscr.seq.lower = HTONL(inst);
	subscr.seq.upper = HTONL(inst);
	subscr.timeout = HTONL(FOREVER);
	subscr.filter = HTONL(TIPC_SUB_CANCEL | TIPC_SUB_PORTS);
	*((uint64_t *)subscr.usr_handle) = subtn_ref_val;

	if (send(tipc_cb.Dsock, &subscr, sizeof(subscr), 0) != sizeof(subscr)) {
		m_MDS_LOG_ERR("MDTM: VDEST-UNSUBSCRIBE Failure\n");
		return NCSCC_RC_FAILURE;
	}
	--num_subscriptions;
#endif
	m_MDS_LOG_INFO("MDTM: VDEST-UNSUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_tx_hdl_register_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Tx Register (For incrementing the use count) */
uint32_t mds_mdtm_tx_hdl_register_tipc(MDS_DEST adest)
{
	/*
	   STEP 1: Check whether there is any TIPC_ID corresponding
	   to the passed ADEST in the ADEST table
	   if present
	   increment the use count in the ADEST table
	   else
	   return failure
	 */
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_tx_hdl_unregister_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Tx Unregister (For decrementing the use count) */
uint32_t mds_mdtm_tx_hdl_unregister_tipc(MDS_DEST adest)
{
	/*
	   STEP 1: Check whether there is any TIPC_ID corresponding to the passed ADEST in the
	   ADEST table
	   if present
	   decrement the use count in the ADEST table
	   if the use count is zero
	   delete the Entry in the ADEST table
	   else
	   return failure
	 */

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mdtm_send_tipc

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

/* Send messages to the destination */

uint32_t mds_mdtm_send_tipc(MDTM_SEND_REQ *req)
{
	/*
	   STEP 1: Get the TIPC_ID from the ADEST present in the recd structure
	   STEP 2: Get the total length of the message to be sent,
	   if len is greater than a single frag size,
	   fragment the message
	   Add fragment hdr
	   convert the msg into flat buffer
	   send message
	 */
	uint32_t status = 0;

	if (req->to == DESTINATION_SAME_PROCESS) {
		MDS_DATA_RECV recv;
		memset(&recv, 0, sizeof(recv));

		recv.dest_svc_hdl =
		    (MDS_SVC_HDL)m_MDS_GET_SVC_HDL_FROM_PWE_ID_VDEST_ID_AND_SVC_ID(req->dest_pwe_id, req->dest_vdest_id,
										   req->dest_svc_id);
		recv.src_svc_id = req->src_svc_id;
		recv.src_pwe_id = req->src_pwe_id;
		recv.src_vdest = req->src_vdest_id;
		recv.exchange_id = req->xch_id;
		recv.src_adest = tipc_cb.adest;
		recv.snd_type = req->snd_type;
		recv.msg = req->msg;
		recv.pri = req->pri;
		recv.msg_fmt_ver = req->msg_fmt_ver;
		recv.src_svc_sub_part_ver = req->src_svc_sub_part_ver;

		/* This is exclusively for the Bcast ENC and ENC_FLAT case */
		if (recv.msg.encoding == MDS_ENC_TYPE_FULL) {
			ncs_dec_init_space(&recv.msg.data.fullenc_uba, recv.msg.data.fullenc_uba.start);
			recv.msg_arch_word = req->msg_arch_word;
		} else if (recv.msg.encoding == MDS_ENC_TYPE_FLAT) {
			/* This case will not arise, but just to be on safe side */
			ncs_dec_init_space(&recv.msg.data.flat_uba, recv.msg.data.flat_uba.start);
		} else {
			/* Do nothing for the DIrect buff and Copy case */
		}

		status = mds_mcm_ll_data_rcv(&recv);

		return status;

	} else {
		struct tipc_portid tipc_id;
		USRBUF *usrbuf;

		uint32_t frag_seq_num = 0, node_status = 0;

		node_status = m_MDS_CHECK_NCS_NODE_ID_RANGE(m_MDS_GET_NODE_ID_FROM_ADEST(req->adest));

		if (NCSCC_RC_SUCCESS == node_status) {
			tipc_id.node =
			    m_MDS_GET_TIPC_NODE_ID_FROM_NCS_NODE_ID(m_MDS_GET_NODE_ID_FROM_ADEST(req->adest));
			tipc_id.ref = (uint32_t)(req->adest);
		} else {
			if (req->snd_type != MDS_SENDTYPE_ACK) {	/* This check is becoz in ack cases we are only sending the hdr and no data part is being send, so no message free , fix me */
				mdtm_free_reassem_msg_mem(&req->msg);
			}
			return NCSCC_RC_FAILURE;
		}

		frag_seq_num = ++mdtm_global_frag_num;

		/* Only for the ack and not for any other message */
		if (req->snd_type == MDS_SENDTYPE_ACK || req->snd_type == MDS_SENDTYPE_RACK) {
			uint8_t len = SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN;
			uint8_t buffer_ack[len];

			/* Add mds_hdr */
			if (NCSCC_RC_SUCCESS != mdtm_add_mds_hdr(buffer_ack, req)) {
				return NCSCC_RC_FAILURE;
			}

			/* Add frag_hdr */
			if (NCSCC_RC_SUCCESS != mdtm_add_frag_hdr(buffer_ack, len, frag_seq_num, 0)) {
				return NCSCC_RC_FAILURE;
			}

			m_MDS_LOG_DBG("MDTM:Sending message with Service Seqno=%d, TO Dest_Tipc_id=<0x%08x:%u> ",
				      req->svc_seq_num, tipc_id.node, tipc_id.ref);
			return mdtm_sendto(buffer_ack, len, tipc_id);
		}

		if (MDS_ENC_TYPE_FLAT == req->msg.encoding) {
			usrbuf = req->msg.data.flat_uba.start;
		} else if (MDS_ENC_TYPE_FULL == req->msg.encoding) {
			usrbuf = req->msg.data.fullenc_uba.start;
		} else {
			usrbuf = NULL;	/* This is because, usrbuf is used only in the above two cases and if it has come here,
					   it means, it is a direct send. Direct send will not use the USRBUF */
		}

		switch (req->msg.encoding) {
		case MDS_ENC_TYPE_CPY:
			/* will not present here */
			break;

		case MDS_ENC_TYPE_FLAT:
		case MDS_ENC_TYPE_FULL:
			{
				uint32_t len = 0;
				len = m_MMGR_LINK_DATA_LEN(usrbuf);	/* Getting total len */

				m_MDS_LOG_INFO("MDTM: User Sending Data lenght=%d Fr_svc=%d to_svc=%d\n", len,
					       req->src_svc_id, req->dest_svc_id);

				if (len > MDTM_NORMAL_MSG_FRAG_SIZE) {
					/* Packet needs to be fragmented and send */
					status = mdtm_frag_and_send(req, frag_seq_num, tipc_id);
					return status;

				} else {
					uint8_t *p8;
					uint8_t body[len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN];

					p8 = (uint8_t *)m_MMGR_DATA_AT_START(usrbuf, len, (char *)
									  &body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN]);

					if (p8 != &body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN])
						memcpy(&body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN], p8, len);

					if (NCSCC_RC_SUCCESS != mdtm_add_mds_hdr(body, req)) {
						m_MDS_LOG_ERR("MDTM: Unable to add the mds Hdr to the send msg\n");
						m_MMGR_FREE_BUFR_LIST(usrbuf);
						return NCSCC_RC_FAILURE;
					}

					if (NCSCC_RC_SUCCESS !=
					    mdtm_add_frag_hdr(body, (len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN),
							      frag_seq_num, 0)) {
						m_MDS_LOG_ERR("MDTM: Unable to add the frag Hdr to the send msg\n");
						m_MMGR_FREE_BUFR_LIST(usrbuf);
						return NCSCC_RC_FAILURE;
					}

					m_MDS_LOG_DBG
					    ("MDTM:Sending message with Service Seqno=%d, TO Dest_Tipc_id=<0x%08x:%u> ",
					     req->svc_seq_num, tipc_id.node, tipc_id.ref);

					if (NCSCC_RC_SUCCESS !=
					    mdtm_sendto(body, (len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN), tipc_id)) {
						m_MDS_LOG_ERR("MDTM: Unable to send the msg thru TIPC\n");
						m_MMGR_FREE_BUFR_LIST(usrbuf);
						return NCSCC_RC_FAILURE;
					}
					m_MMGR_FREE_BUFR_LIST(usrbuf);
					return NCSCC_RC_SUCCESS;
				}
			}
			break;

		case MDS_ENC_TYPE_DIRECT_BUFF:
			{
				if (req->msg.data.buff_info.len >
				    (MDTM_MAX_DIRECT_BUFF_SIZE - SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN)) {
					m_MDS_LOG_CRITICAL
					    ("MDTM: Passed pkt len is more than the single send direct buff\n");
					mds_free_direct_buff(req->msg.data.buff_info.buff);
					return NCSCC_RC_FAILURE;
				}

				m_MDS_LOG_INFO("MDTM: User Sending Data len=%d Fr_svc=%d to_svc=%d\n",
					       req->msg.data.buff_info.len, req->src_svc_id, req->dest_svc_id);

				uint8_t body[req->msg.data.buff_info.len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN];

				if (NCSCC_RC_SUCCESS != mdtm_add_mds_hdr(body, req)) {
					m_MDS_LOG_ERR("MDTM: Unable to add the mds Hdr to the send msg\n");
					mds_free_direct_buff(req->msg.data.buff_info.buff);
					return NCSCC_RC_FAILURE;
				}
				if (NCSCC_RC_SUCCESS !=
				    mdtm_add_frag_hdr(body,
						      req->msg.data.buff_info.len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN,
						      frag_seq_num, 0)) {
					m_MDS_LOG_ERR("MDTM: Unable to add the frag Hdr to the send msg\n");
					mds_free_direct_buff(req->msg.data.buff_info.buff);
					return NCSCC_RC_FAILURE;
				}
				memcpy(&body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN], req->msg.data.buff_info.buff,
				       req->msg.data.buff_info.len);

				if (NCSCC_RC_SUCCESS !=
				    mdtm_sendto(body,
						(req->msg.data.buff_info.len + SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN),
						tipc_id)) {
					m_MDS_LOG_ERR("MDTM: Unable to send the msg thru TIPC\n");
					mds_free_direct_buff(req->msg.data.buff_info.buff);
					return NCSCC_RC_FAILURE;
				}

				/* If Direct Send is bcast it will be done at bcast function */
				if (req->snd_type == MDS_SENDTYPE_BCAST || req->snd_type == MDS_SENDTYPE_RBCAST) {
					/* Dont free Here */
				} else
					mds_free_direct_buff(req->msg.data.buff_info.buff);

				return NCSCC_RC_SUCCESS;
			}
			break;

		default:
			m_MDS_LOG_ERR("MDTM: Encoding type not supported\n");
			return NCSCC_RC_SUCCESS;
			break;
		}
		return NCSCC_RC_SUCCESS;
	}
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: mdtm_frag_and_send

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
#define MDTM_MAX_SEND_PKT_SIZE   (MDTM_NORMAL_MSG_FRAG_SIZE+SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN)	/* Includes the 30 header bytes(2+8+20) */

#ifdef MDS_CHECKSUM_ENABLE_FLAG
#define MDTM_FRAG_HDR_PLUS_LEN_2   13
#else
#define MDTM_FRAG_HDR_PLUS_LEN_2   10
#endif

uint32_t mdtm_frag_and_send(MDTM_SEND_REQ *req, uint32_t seq_num, struct tipc_portid id)
{
	USRBUF *usrbuf;
	uint32_t len = 0;
	uint16_t len_buf = 0;
	uint8_t *p8;
	uint16_t i = 1;
	uint16_t frag_val = 0;

	switch (req->msg.encoding) {
	case MDS_ENC_TYPE_FULL:
		usrbuf = req->msg.data.fullenc_uba.start;
		break;

	case MDS_ENC_TYPE_FLAT:
		usrbuf = req->msg.data.flat_uba.start;
		break;

	default:
		return NCSCC_RC_SUCCESS;
	}

	len = m_MMGR_LINK_DATA_LEN(usrbuf);	/* Getting total len */

	if (len > (32767 * MDTM_NORMAL_MSG_FRAG_SIZE)) {	/* We have 15 bits for frag number so 2( pow 15) -1=32767 */
		m_MDS_LOG_CRITICAL
		    ("MDTM: App. is trying to send data more than MDTM Can fragment and send, Max size is =%d\n",
		     32767 * MDTM_NORMAL_MSG_FRAG_SIZE);
		m_MMGR_FREE_BUFR_LIST(usrbuf);
		return NCSCC_RC_FAILURE;
	}

	while (len != 0) {
		if (len > MDTM_NORMAL_MSG_FRAG_SIZE) {
			if (i == 1) {
				len_buf = MDTM_MAX_SEND_PKT_SIZE;
				frag_val = MORE_FRAG_BIT | i;
			} else {
				if ((len + MDTM_FRAG_HDR_PLUS_LEN_2) > MDTM_MAX_SEND_PKT_SIZE) {
					len_buf = MDTM_MAX_SEND_PKT_SIZE;
					frag_val = MORE_FRAG_BIT | i;
				} else {
					len_buf = len + MDTM_FRAG_HDR_PLUS_LEN_2;
					frag_val = NO_FRAG_BIT | i;
				}
			}
		} else {
			len_buf = len + MDTM_FRAG_HDR_PLUS_LEN_2;
			frag_val = NO_FRAG_BIT | i;
		}
		{
			uint8_t body[len_buf];
			if (i == 1) {
				p8 = (uint8_t *)m_MMGR_DATA_AT_START(usrbuf,
								  (len_buf - SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN),
								  (char *)&body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN]);

				if (p8 != &body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN])
					memcpy(&body[SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN], p8, len_buf);

				if (NCSCC_RC_SUCCESS != mdtm_add_mds_hdr(body, req)) {
					m_MDS_LOG_ERR("MDTM: frg MDS hdr addition failed\n");
					m_MMGR_FREE_BUFR_LIST(usrbuf);
					return NCSCC_RC_FAILURE;
				}

				if (NCSCC_RC_SUCCESS != mdtm_add_frag_hdr(body, len_buf, seq_num, frag_val)) {
					m_MDS_LOG_ERR("MDTM: Frag hdr addition failed\n");
					m_MMGR_FREE_BUFR_LIST(usrbuf);
					return NCSCC_RC_FAILURE;
				}
				m_MDS_LOG_DBG
				    ("MDTM:Sending message with Service Seqno=%d, Fragment Seqnum=%d, frag_num=%d, TO Dest_Tipc_id=<0x%08x:%u>",
				     req->svc_seq_num, seq_num, frag_val, id.node, id.ref);
				mdtm_sendto(body, len_buf, id);
				m_MMGR_REMOVE_FROM_START(&usrbuf, len_buf - SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN);
				len = len - (len_buf - SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN);
			} else {
				p8 = (uint8_t *)m_MMGR_DATA_AT_START(usrbuf, len_buf - MDTM_FRAG_HDR_PLUS_LEN_2,
								  (char *)&body[MDTM_FRAG_HDR_PLUS_LEN_2]);
				if (p8 != &body[MDTM_FRAG_HDR_PLUS_LEN_2])
					memcpy(&body[MDTM_FRAG_HDR_PLUS_LEN_2], p8, len_buf - MDTM_FRAG_HDR_PLUS_LEN_2);

				if (NCSCC_RC_SUCCESS != mdtm_add_frag_hdr(body, len_buf, seq_num, frag_val)) {
					m_MDS_LOG_ERR("MDTM: Frag hde addition failed\n");
					m_MMGR_FREE_BUFR_LIST(usrbuf);
					return NCSCC_RC_FAILURE;
				}
				m_MDS_LOG_DBG
				    ("MDTM:Sending message with Service Seqno=%d, Fragment Seqnum=%d, frag_num=%d, TO Dest_Tipc_id=<0x%08x:%u>",
				     req->svc_seq_num, seq_num, frag_val, id.node, id.ref);
				mdtm_sendto(body, len_buf, id);
				m_MMGR_REMOVE_FROM_START(&usrbuf, (len_buf - MDTM_FRAG_HDR_PLUS_LEN_2));
				len = len - (len_buf - MDTM_FRAG_HDR_PLUS_LEN_2);
				if (len == 0)
					break;
			}
		}
		i++;
		frag_val = 0;
	}
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_add_frag_hdr

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uint32_t mdtm_add_frag_hdr(uint8_t *buf_ptr, uint16_t len, uint32_t seq_num, uint16_t frag_byte)
{
	/* Add the FRAG HDR to the Buffer */
	uint8_t *data;
	data = buf_ptr;
	ncs_encode_16bit(&data, len);
#ifdef MDS_CHECKSUM_ENABLE_FLAG
	ncs_encode_8bit(&data, 0);
	ncs_encode_16bit(&data, 0);
#endif
	ncs_encode_32bit(&data, seq_num);
	ncs_encode_16bit(&data, frag_byte);
#ifdef MDS_CHECKSUM_ENABLE_FLAG
	ncs_encode_16bit(&data, len - MDTM_FRAG_HDR_LEN - 5);	/* 2 bytes for keeping len, to cross check at the receiver end */
#else
	ncs_encode_16bit(&data, len - MDTM_FRAG_HDR_LEN - 2);	/* 2 bytes for keeping len, to cross check at the receiver end */
#endif
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_sendto

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

static uint32_t mdtm_sendto(uint8_t *buffer, uint16_t buff_len, struct tipc_portid id)
{
	/* Can be made as macro even */
	struct sockaddr_tipc server_addr;
	int send_len = 0;
#ifdef MDS_CHECKSUM_ENABLE_FLAG
	uint16_t checksum = 0;
#endif

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.family = AF_TIPC;
	server_addr.addrtype = TIPC_ADDR_ID;
	server_addr.addr.id = id;

	m_MDS_LOG_INFO("MDTM: TIPC Sending Len=%d\n", buff_len);

#ifdef MDS_CHECKSUM_ENABLE_FLAG
	if (gl_mds_checksum == 1) {
		buffer[2] = 1;
		buffer[3] = 0;
		buffer[4] = 0;
		checksum = mds_checksum(buff_len, buffer);
		buffer[3] = checksum >> 8;
		buffer[4] = checksum;
	}
#endif

	send_len = sendto(tipc_cb.BSRsock, buffer, buff_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (send_len == buff_len) {
		m_MDS_LOG_INFO("MDTM: Successfully sent message");
		return NCSCC_RC_SUCCESS;
	} else {
		m_MDS_LOG_ERR("MDTM: Failed to send message err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 *
 * Function Name: mdtm_add_mds_hdr
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

static uint32_t mdtm_add_mds_hdr(uint8_t *buffer, MDTM_SEND_REQ *req)
{
	uint8_t prot_ver = 0, enc_snd_type = 0;
#ifdef MDS_CHECKSUM_ENABLE_FLAG
	uint8_t zero_8 = 0;
#endif

	uint16_t zero_16 = 0;
	uint32_t zero_32 = 0;

	uint32_t xch_id = 0;

	uint8_t *ptr;
	ptr = buffer;

	prot_ver |= MDS_PROT | MDS_VERSION | ((uint8_t)(req->pri - 1));
	enc_snd_type = (req->msg.encoding << 6);
	enc_snd_type |= (uint8_t)req->snd_type;

	/* Message length */
	ncs_encode_16bit(&ptr, zero_16);

#ifdef MDS_CHECKSUM_ENABLE_FLAG
	/* FRAG HDR */
	ncs_encode_8bit(&ptr, zero_8);	/* for checksum flag */
	ncs_encode_16bit(&ptr, zero_16);	/* for checksum */
#endif

	ncs_encode_32bit(&ptr, zero_32);
	ncs_encode_16bit(&ptr, zero_16);
	ncs_encode_16bit(&ptr, zero_16);

	/* MDS HDR */
	ncs_encode_8bit(&ptr, prot_ver);
	ncs_encode_16bit(&ptr, (uint16_t)MDS_HDR_LEN);	/* Will be updated if any additional options are being added at the end */
	ncs_encode_32bit(&ptr, req->svc_seq_num);
	ncs_encode_8bit(&ptr, enc_snd_type);
	ncs_encode_16bit(&ptr, req->src_pwe_id);
	ncs_encode_16bit(&ptr, req->src_vdest_id);
	ncs_encode_16bit(&ptr, req->src_svc_id);
	ncs_encode_16bit(&ptr, req->dest_vdest_id);
	ncs_encode_16bit(&ptr, req->dest_svc_id);

	switch (req->snd_type) {
	case MDS_SENDTYPE_SNDRSP:
	case MDS_SENDTYPE_SNDRACK:
	case MDS_SENDTYPE_SNDACK:
	case MDS_SENDTYPE_RSP:
	case MDS_SENDTYPE_REDRSP:
	case MDS_SENDTYPE_REDRACK:
	case MDS_SENDTYPE_REDACK:
	case MDS_SENDTYPE_RRSP:
	case MDS_SENDTYPE_ACK:
	case MDS_SENDTYPE_RACK:
		xch_id = req->xch_id;
		break;

	default:
		xch_id = 0;
		break;
	}

	ncs_encode_32bit(&ptr, xch_id);
	ncs_encode_16bit(&ptr, req->msg_fmt_ver);	/* New field */

	return NCSCC_RC_SUCCESS;
}


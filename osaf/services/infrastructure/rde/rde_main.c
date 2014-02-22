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
 * Author(s): Ericsson AB
 *            Wind River Systems
 *
 */

#include <configmake.h>
#include <stdlib.h>
#include <poll.h>
#include <libgen.h>

#include <logtrace.h>
#include <mds_papi.h>
#include <nid_api.h>
#include <daemon.h>
#include <nid_api.h>

#include "rde_cb.h"

#define RDA_MAX_CLIENTS 32

static const char *role_string[] =
{
	"Undefined role",
	"ACTIVE",
	"STANDBY",
	"QUIESCED",
	"QUIESCING",
	"Invalid"
};

enum {
	FD_TERM = 0,
	FD_AMF = 1,
	FD_MBX,
	FD_RDA_SERVER,
	FD_CLIENT_START
};

NCS_NODE_ID rde_my_node_id;
static NCS_NODE_ID peer_node_id;

const char *rde_msg_name[] = {
	"-",
	"RDE_MSG_PEER_UP(1)",
	"RDE_MSG_PEER_DOWN(2)",
	"RDE_MSG_PEER_INFO_REQ(3)",
	"RDE_MSG_PEER_INFO_RESP(4)",
};

/* note: default value mentioned in $pkgsysconfdir/rde.conf, change in both places */
static int discover_peer_timeout = 2000;
static RDE_CONTROL_BLOCK _rde_cb;
static RDE_CONTROL_BLOCK *rde_cb = &_rde_cb;
static NCS_SEL_OBJ usr1_sel_obj;


RDE_CONTROL_BLOCK *rde_get_control_block(void)
{
	return rde_cb;
}

/**
 * USR1 signal is used when AMF wants instantiate us as a
 * component. Wake up the main thread so it can register with
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

uint32_t rde_set_role(PCS_RDA_ROLE role)
{
	LOG_NO("RDE role set to %s", role_string[role]);

	rde_cb->ha_role = role;

	/* Send new role to all RDA client */
	rde_rda_send_role(rde_cb->ha_role);

	return NCSCC_RC_SUCCESS;
}

static int fd_to_client_ixd(int fd)
{
	int i;
	RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;

	for (i = 0; i < rde_rda_cb->client_count; i++)
		if (fd == rde_rda_cb->clients[i].fd)
			break;

	assert(i < MAX_RDA_CLIENTS);

	return i;
}

static void handle_mbx_event(void)
{
	struct rde_msg *msg;

	TRACE_ENTER();

	msg = (struct rde_msg*)ncs_ipc_non_blk_recv(&rde_cb->mbx);

	switch (msg->type) {
	case RDE_MSG_PEER_INFO_REQ: {
		struct rde_msg peer_info_req;
		TRACE("Received %s", rde_msg_name[msg->type]);
		peer_info_req.type = RDE_MSG_PEER_INFO_RESP;
		peer_info_req.info.peer_info.ha_role = rde_cb->ha_role;
		rde_mds_send(&peer_info_req, msg->fr_dest);
		break;
	}
	case RDE_MSG_PEER_UP:
		TRACE("Received %s", rde_msg_name[msg->type]);
		peer_node_id = msg->fr_node_id;
		break;
	case RDE_MSG_PEER_DOWN:
		TRACE("Received %s", rde_msg_name[msg->type]);
		peer_node_id = 0;
		break;
	default:
		LOG_ER("%s: discarding unknown message type %u", __FUNCTION__, msg->type);
		break;
	}

	free(msg);

	TRACE_LEAVE();
}

static uint32_t discover_peer(int mbx_fd)
{
	struct pollfd fds[1];
	struct rde_msg *msg;
	int ret;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	fds[0].fd = mbx_fd;
	fds[0].events = POLLIN;

	while (1) {
		ret = poll(fds, 1, discover_peer_timeout);

		if (ret == -1) {
			if (errno == EINTR)
				continue;
			
			LOG_ER("poll failed - %s", strerror(errno));
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if (ret == 0) {
			TRACE("Peer discovery timeout");
			goto done;
		}

		if (ret == 1) {
			msg = (struct rde_msg*)ncs_ipc_non_blk_recv(&rde_cb->mbx);

			switch (msg->type) {
			case RDE_MSG_PEER_UP: {
				struct rde_msg peer_info_req;

				peer_node_id = msg->fr_node_id;
				TRACE("Received %s", rde_msg_name[msg->type]);

				/* Send request for peer information */
				peer_info_req.type = RDE_MSG_PEER_INFO_REQ;
				peer_info_req.info.peer_info.ha_role = rde_cb->ha_role;
				rde_mds_send(&peer_info_req, msg->fr_dest);
				goto done;
			}
			default:
				LOG_ER("%s: discarding unknown message type %u", __FUNCTION__, msg->type);
				break;
			}

		} else
			assert(0);
	}
done:
	TRACE_LEAVE();
	return rc;
}

static uint32_t determine_role(int mbx_fd)
{
	struct pollfd fds[1];
	struct rde_msg *msg;
	int ret;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (peer_node_id == 0) {
		LOG_NO("No peer available => Setting Active role for this node");
		rde_cb->ha_role = PCS_RDA_ACTIVE;
		goto done;
	}

	fds[0].fd = mbx_fd;
	fds[0].events = POLLIN;

	while (1) {
		ret = poll(fds, 1, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;
			
			LOG_ER("poll failed - %s", strerror(errno));
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		assert(ret == 1);

		msg = (struct rde_msg*)ncs_ipc_non_blk_recv(&rde_cb->mbx);

		switch (msg->type) {
		case RDE_MSG_PEER_UP:
			TRACE("Received straggler up msg, ignoring");
			assert(peer_node_id);
			break;
		case RDE_MSG_PEER_DOWN:
			TRACE("Received %s", rde_msg_name[msg->type]);
			LOG_NO("peer rde@%x down waiting for response => Setting Active role", peer_node_id);
			rde_cb->ha_role = PCS_RDA_ACTIVE;
			peer_node_id = 0;
			goto done;
		case RDE_MSG_PEER_INFO_REQ: {
			struct rde_msg peer_info_req;
			TRACE("Received %s", rde_msg_name[msg->type]);
			peer_info_req.type = RDE_MSG_PEER_INFO_RESP;
			peer_info_req.info.peer_info.ha_role = rde_cb->ha_role;
			rde_mds_send(&peer_info_req, msg->fr_dest);
			break;
		}
		case RDE_MSG_PEER_INFO_RESP:
			TRACE("Received %s", rde_msg_name[msg->type]);
			switch (msg->info.peer_info.ha_role) {
			case PCS_RDA_UNDEFINED:
				TRACE("my=%x, peer=%x", rde_my_node_id, msg->fr_node_id);
				if (rde_my_node_id < msg->fr_node_id) {
					rde_cb->ha_role = PCS_RDA_ACTIVE;
					LOG_NO("Peer rde@%x has no state, my nodeid is less => Setting Active role", msg->fr_node_id);
				} else if (rde_my_node_id > msg->fr_node_id) {
					rde_cb->ha_role = PCS_RDA_STANDBY;
					LOG_NO("Peer rde@%x has no state, my nodeid is greater => Setting Standby role", msg->fr_node_id);
				} else
					assert(0);
				goto done;
			case PCS_RDA_ACTIVE:
				rde_cb->ha_role = PCS_RDA_STANDBY;
				LOG_NO("Peer rde@%x has active state => Assigning Standby role to this node", msg->fr_node_id);
				goto done;
			case PCS_RDA_STANDBY:
				LOG_NO("Peer rde@%x has standby state => possible fail over, waiting...", msg->fr_node_id);
				sleep(1);
				
				/* Send request for peer information */
				struct rde_msg peer_info_req;
				peer_info_req.type = RDE_MSG_PEER_INFO_REQ;
				peer_info_req.info.peer_info.ha_role = rde_cb->ha_role;
				rde_mds_send(&peer_info_req, msg->fr_dest);
				break;
			default:
				LOG_NO("rde@%x has unsupported state, panic!", msg->fr_node_id);
				assert(0);
			}
			break;
		default:
			LOG_ER("%s: discarding unknown message type %u", __FUNCTION__, msg->type);
			break;
		}
	} /* while (1) */

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Initialize the RDE server.
 * 
 * @return int, 0=OK
 */
static int initialize_rde(void)
{
	RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;
	int rc = NCSCC_RC_FAILURE;
	char *val;

	/* Determine how this process was started, by NID or AMF */
	if (getenv("SA_AMF_COMPONENT_NAME") == NULL)
		rde_cb->rde_amf_cb.nid_started = true;

	if ((val = getenv("RDE_DISCOVER_PEER_TIMEOUT")) != NULL)
		discover_peer_timeout = strtoul(val, NULL, 0);

	TRACE("discover_peer_timeout=%d", discover_peer_timeout);

	if ((rc = ncs_core_agents_startup()) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_core_agents_startup FAILED");
		goto init_failed;
	}

	if (rde_cb->rde_amf_cb.nid_started &&
		(rc = ncs_sel_obj_create(&usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create FAILED");
		goto init_failed;
	}

	if ((rc = ncs_ipc_create(&rde_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_ipc_create FAILED");
		goto init_failed;
	}

	if ((rc = ncs_ipc_attach(&rde_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_ipc_attach FAILED");
		goto init_failed;
	}

	rde_my_node_id = ncs_get_node_id();

	if ((rc = rde_rda_open(RDE_RDA_SOCK_NAME, rde_rda_cb)) != NCSCC_RC_SUCCESS)
		goto init_failed;

	if (rde_cb->rde_amf_cb.nid_started &&
		signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 FAILED: %s", strerror(errno));
		goto init_failed;
	}

	if (rde_mds_register(rde_cb) != NCSCC_RC_SUCCESS)
		goto init_failed;

	rc = NCSCC_RC_SUCCESS;

 init_failed:
	return rc;
}

int main(int argc, char *argv[])
{
	uint32_t rc;
	nfds_t nfds = 4;
	struct pollfd fds[nfds + RDA_MAX_CLIENTS];
	int i, ret;
	NCS_SEL_OBJ mbx_sel_obj;
	RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;
	int term_fd;

	daemonize(argc, argv);

	if (initialize_rde() != NCSCC_RC_SUCCESS)
		goto init_failed;

	mbx_sel_obj = ncs_ipc_get_sel_obj(&rde_cb->mbx);

	if ((rc = discover_peer(mbx_sel_obj.rmv_obj)) == NCSCC_RC_FAILURE)
		goto init_failed;

	if ((rc = determine_role(mbx_sel_obj.rmv_obj)) == NCSCC_RC_FAILURE)
		goto init_failed;

	/* If AMF started register immediately */
	if (!rde_cb->rde_amf_cb.nid_started &&
		(rc = rde_amf_init(&rde_cb->rde_amf_cb)) != NCSCC_RC_SUCCESS) {
		goto init_failed;
	}

	if (rde_cb->rde_amf_cb.nid_started &&
		nid_notify("RDE", rc, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		goto done;
	}

	daemon_sigterm_install(&term_fd);

	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;

	/* USR1/AMF fd */
	fds[FD_AMF].fd = rde_cb->rde_amf_cb.nid_started ?
		usr1_sel_obj.rmv_obj : rde_cb->rde_amf_cb.amf_fd;
	fds[FD_AMF].events = POLLIN;

	/* Mailbox */
	fds[FD_MBX].fd = mbx_sel_obj.rmv_obj;
	fds[FD_MBX].events = POLLIN;

	/* RDA server socket */
	fds[FD_RDA_SERVER].fd = rde_cb->rde_rda_cb.fd;
	fds[FD_RDA_SERVER].events = POLLIN;

	while (1) {
		ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;
			
			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (fds[FD_AMF].revents & POLLIN) {
			if (rde_cb->rde_amf_cb.amf_hdl != 0) {
				SaAisErrorT error;
				TRACE("AMF event rec");
				if ((error = saAmfDispatch(rde_cb->rde_amf_cb.amf_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
					LOG_ER("saAmfDispatch failed: %u", error);
					goto done;
				}
			} else {
				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_destroy(usr1_sel_obj);
				
				if (rde_amf_init(&rde_cb->rde_amf_cb) != NCSCC_RC_SUCCESS)
					goto done;
				
				fds[FD_AMF].fd = rde_cb->rde_amf_cb.amf_fd;
			}
		}

		if (fds[FD_MBX].revents & POLLIN)
			handle_mbx_event();

		if (fds[FD_RDA_SERVER].revents & POLLIN) {
			int newsockfd;

			newsockfd = accept(rde_rda_cb->fd, (struct sockaddr *)NULL, NULL);
			if (newsockfd < 0) {
				LOG_ER("accept FAILED %s", strerror(errno));
				goto done;
			}

			/* Add the new client fd to client-list	*/
			rde_rda_cb->clients[rde_rda_cb->client_count].is_async = false;
			rde_rda_cb->clients[rde_rda_cb->client_count].fd = newsockfd;
			rde_rda_cb->client_count++;

			/* Update poll fd selection */
			fds[nfds].fd = newsockfd;
			fds[nfds].events = POLLIN;
			nfds++;

			TRACE("accepted new client, fd=%d, idx=%d, nfds=%lu", newsockfd, rde_rda_cb->client_count, nfds);
		}

		for (i = FD_CLIENT_START; i < nfds; i++) {
			if (fds[i].revents & POLLIN) {
				int client_disconnected = 0;
				TRACE("received msg on fd %u", fds[i].fd);
				rde_rda_client_process_msg(rde_rda_cb, fd_to_client_ixd(fds[i].fd), &client_disconnected);
				if (client_disconnected) {
					/* reinitialize the fd array & nfds */
					nfds = FD_CLIENT_START;
					for (i = 0; i < rde_rda_cb->client_count; i++, nfds++) {
						fds[i + FD_CLIENT_START].fd = rde_rda_cb->clients[i].fd;
						fds[i + FD_CLIENT_START].events = POLLIN;
					}
					TRACE("client disconnected, fd array reinitialized, nfds=%lu", nfds);
					break;
				}
			}
		}
	}

 init_failed:
	if (rde_cb->rde_amf_cb.nid_started &&
		nid_notify("RDE", NCSCC_RC_FAILURE, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		rc = NCSCC_RC_FAILURE;
	}

 done:
	syslog(LOG_ERR, "Exiting...");
	exit(1);
}


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

#include <limits.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include "base/daemon.h"
#include "base/logtrace.h"
#include "base/osaf_poll.h"
#include "mds/mds_papi.h"
#include "base/ncs_main_papi.h"
#include "nid/agent/nid_api.h"
#include "amf/saf/saAmf.h"
#include "rde/rded/rde_cb.h"
#include "rde/rded/role.h"

#define RDA_MAX_CLIENTS 32

enum {
  FD_TERM = 0,
  FD_AMF = 1,
  FD_MBX,
  FD_RDA_SERVER,
  FD_CLIENT_START
};

static void SendPeerInfoReq(MDS_DEST mds_dest);
static void SendPeerInfoResp(MDS_DEST mds_dest);
static void CheckForSplitBrain(const rde_msg* msg);

const char *rde_msg_name[] = { "-", "RDE_MSG_PEER_UP(1)",
    "RDE_MSG_PEER_DOWN(2)", "RDE_MSG_PEER_INFO_REQ(3)",
    "RDE_MSG_PEER_INFO_RESP(4)", };

static RDE_CONTROL_BLOCK _rde_cb;
static RDE_CONTROL_BLOCK *rde_cb = &_rde_cb;
static NCS_SEL_OBJ usr1_sel_obj;
static NODE_ID own_node_id;
static Role* role;

RDE_CONTROL_BLOCK *rde_get_control_block() {
  return rde_cb;
}

/**
 * USR1 signal is used when AMF wants instantiate us as a
 * component. Wake up the main thread so it can register with
 * AMF.
 *
 * @param i_sig_num
 */
static void sigusr1_handler(int sig) {
  (void) sig;
  signal(SIGUSR1, SIG_IGN);
  ncs_sel_obj_ind(&usr1_sel_obj);
}

static int fd_to_client_ixd(int fd) {
  int i;
  RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;

  for (i = 0; i < rde_rda_cb->client_count; i++)
    if (fd == rde_rda_cb->clients[i].fd)
      break;

  assert(i < MAX_RDA_CLIENTS);

  return i;
}

static void handle_mbx_event() {
  rde_msg *msg;

  TRACE_ENTER();

  msg = reinterpret_cast<rde_msg*>(ncs_ipc_non_blk_recv(&rde_cb->mbx));
  TRACE("Received %s from node 0x%x with state %s. My state is %s",
        rde_msg_name[msg->type], msg->fr_node_id,
        Role::to_string(msg->info.peer_info.ha_role),
        Role::to_string(role->role()));

  switch (msg->type) {
    case RDE_MSG_PEER_INFO_REQ: {
      LOG_NO("Got peer info request from node 0x%x with role %s",
             msg->fr_node_id, Role::to_string(msg->info.peer_info.ha_role));
      CheckForSplitBrain(msg);
      SendPeerInfoResp(msg->fr_dest);
      break;
    }
    case RDE_MSG_PEER_INFO_RESP: {
      LOG_NO("Got peer info response from node 0x%x with role %s",
             msg->fr_node_id, Role::to_string(msg->info.peer_info.ha_role));
      CheckForSplitBrain(msg);
      role->SetPeerState(msg->info.peer_info.ha_role, msg->fr_node_id);
      break;
    }
    case RDE_MSG_PEER_UP: {
      if (msg->fr_node_id != own_node_id) {
        LOG_NO("Peer up on node 0x%x", msg->fr_node_id);
        SendPeerInfoReq(msg->fr_dest);
      }
      break;
    }
    case RDE_MSG_PEER_DOWN:
      LOG_NO("Peer down on node 0x%x", msg->fr_node_id);
      break;
    default:
      LOG_ER("%s: discarding unknown message type %u", __FUNCTION__,
             msg->type);
      break;
  }

  free(msg);

  TRACE_LEAVE();
}

static void CheckForSplitBrain(const rde_msg* msg) {
  PCS_RDA_ROLE own_role = role->role();
  PCS_RDA_ROLE other_role = msg->info.peer_info.ha_role;
  if (own_role == PCS_RDA_ACTIVE && other_role == PCS_RDA_ACTIVE) {
    opensaf_reboot(0, nullptr, "Split-brain detected");
  }
}

static void SendPeerInfoReq(MDS_DEST mds_dest) {
  rde_msg peer_info_req;
  peer_info_req.type = RDE_MSG_PEER_INFO_REQ;
  peer_info_req.info.peer_info.ha_role = role->role();
  rde_mds_send(&peer_info_req, mds_dest);
}

static void SendPeerInfoResp(MDS_DEST mds_dest) {
  rde_msg peer_info_req;
  peer_info_req.type = RDE_MSG_PEER_INFO_RESP;
  peer_info_req.info.peer_info.ha_role = role->role();
  rde_mds_send(&peer_info_req, mds_dest);
}

/**
 * Initialize the RDE server.
 *
 * @return int, 0=OK
 */
static int initialize_rde() {
  RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;
  int rc = NCSCC_RC_FAILURE;

  if ((rc = rde_rda_open(RDE_RDA_SOCK_NAME, rde_rda_cb)) != NCSCC_RC_SUCCESS) {
    goto init_failed;
  }

  /* Determine how this process was started, by NID or AMF */
  if (getenv("SA_AMF_COMPONENT_NAME") == nullptr)
    rde_cb->rde_amf_cb.nid_started = true;

  if ((rc = ncs_core_agents_startup()) != NCSCC_RC_SUCCESS) {
    LOG_ER("ncs_core_agents_startup FAILED");
    goto init_failed;
  }

  own_node_id = ncs_get_node_id();
  role = new Role(own_node_id);
  rde_rda_cb->role = role;
  rde_cb->rde_amf_cb.role = role;

  if (rde_cb->rde_amf_cb.nid_started
      && (rc = ncs_sel_obj_create(&usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
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

  if (rde_cb->rde_amf_cb.nid_started
      && signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
    LOG_ER("signal USR1 FAILED: %s", strerror(errno));
    goto init_failed;
  }

  rc = NCSCC_RC_SUCCESS;

  init_failed: return rc;
}

int main(int argc, char *argv[]) {
  nfds_t nfds = FD_CLIENT_START;
  pollfd fds[FD_CLIENT_START + RDA_MAX_CLIENTS];
  int ret;
  NCS_SEL_OBJ mbx_sel_obj;
  RDE_RDA_CB *rde_rda_cb = &rde_cb->rde_rda_cb;
  int term_fd;

  opensaf_reboot_prepare();

  daemonize(argc, argv);

  if (initialize_rde() != NCSCC_RC_SUCCESS)
    goto init_failed;

  mbx_sel_obj = ncs_ipc_get_sel_obj(&rde_cb->mbx);

  /* If AMF started register immediately */
  if (!rde_cb->rde_amf_cb.nid_started
      && rde_amf_init(&rde_cb->rde_amf_cb) != NCSCC_RC_SUCCESS) {
    goto init_failed;
  }

  daemon_sigterm_install(&term_fd);

  fds[FD_TERM].fd = term_fd;
  fds[FD_TERM].events = POLLIN;

  /* USR1/AMF fd */
  fds[FD_AMF].fd =
      rde_cb->rde_amf_cb.nid_started ?
          usr1_sel_obj.rmv_obj : rde_cb->rde_amf_cb.amf_fd;
  fds[FD_AMF].events = POLLIN;

  /* Mailbox */
  fds[FD_MBX].fd = mbx_sel_obj.rmv_obj;
  fds[FD_MBX].events = POLLIN;

  /* RDA server socket */
  fds[FD_RDA_SERVER].fd = rde_cb->rde_rda_cb.fd;
  fds[FD_RDA_SERVER].events = POLLIN;

  if (rde_cb->rde_amf_cb.nid_started) {
    TRACE("NID started");
    if (nid_notify("RDE", NCSCC_RC_SUCCESS, nullptr) != NCSCC_RC_SUCCESS) {
      LOG_ER("nid_notify failed");
      goto init_failed;
    }
  } else {
    TRACE("Not NID started");
  }

  while (1) {
    nfds_t fds_to_poll =
        role->role() != PCS_RDA_UNDEFINED ? nfds : FD_CLIENT_START;
    ret = osaf_poll(fds, fds_to_poll, 0);
    if (ret == 0) {
      timespec ts;
      timespec* timeout = role->Poll(&ts);
      fds_to_poll = role->role() != PCS_RDA_UNDEFINED ? nfds : FD_CLIENT_START;
      ret = osaf_ppoll(fds, fds_to_poll, timeout, nullptr);
    }

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
        if ((error = saAmfDispatch(rde_cb->rde_amf_cb.amf_hdl,
                                   SA_DISPATCH_ALL)) != SA_AIS_OK) {
          LOG_ER("saAmfDispatch failed: %u", error);
          goto done;
        }
      } else {
        TRACE("SIGUSR1 event rec");
        ncs_sel_obj_destroy(&usr1_sel_obj);

        if (rde_amf_init(&rde_cb->rde_amf_cb) != NCSCC_RC_SUCCESS)
          goto done;

        fds[FD_AMF].fd = rde_cb->rde_amf_cb.amf_fd;
      }
    }

    if (fds[FD_MBX].revents & POLLIN)
      handle_mbx_event();

    if (fds[FD_RDA_SERVER].revents & POLLIN) {
      int newsockfd;

      newsockfd = accept(rde_rda_cb->fd, nullptr, nullptr);
      if (newsockfd < 0) {
        LOG_ER("accept FAILED %s", strerror(errno));
        goto done;
      }

      /* Add the new client fd to client-list */
      rde_rda_cb->clients[rde_rda_cb->client_count].is_async = false;
      rde_rda_cb->clients[rde_rda_cb->client_count].fd = newsockfd;
      rde_rda_cb->client_count++;

      /* Update poll fd selection */
      fds[nfds].fd = newsockfd;
      fds[nfds].events = POLLIN;
      nfds++;

      TRACE("accepted new client, fd=%d, idx=%d, nfds=%lu", newsockfd,
            rde_rda_cb->client_count, nfds);
    }

    for (nfds_t i = FD_CLIENT_START;
        role->role() != PCS_RDA_UNDEFINED && i < fds_to_poll; i++) {
      if (fds[i].revents & POLLIN) {
        int client_disconnected = 0;
        TRACE("received msg on fd %u", fds[i].fd);
        rde_rda_client_process_msg(rde_rda_cb, fd_to_client_ixd(fds[i].fd),
                                   &client_disconnected);
        if (client_disconnected) {
          /* reinitialize the fd array & nfds */
          nfds = FD_CLIENT_START;
          for (int j = 0; j < rde_rda_cb->client_count; j++, nfds++) {
            fds[j + FD_CLIENT_START].fd = rde_rda_cb->clients[j].fd;
            fds[j + FD_CLIENT_START].events = POLLIN;
          }
          TRACE("client disconnected, fd array reinitialized, nfds=%lu", nfds);
          break;
        }
      }
    }
  }

  init_failed: if (rde_cb->rde_amf_cb.nid_started
      && nid_notify("RDE", NCSCC_RC_FAILURE, nullptr) != NCSCC_RC_SUCCESS) {
    LOG_ER("nid_notify failed");
  }

  done: syslog(LOG_ERR, "Exiting...");
  exit(1);
}


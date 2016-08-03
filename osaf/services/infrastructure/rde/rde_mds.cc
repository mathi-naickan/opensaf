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
 * Author(s): Ericsson AB
 *
 */

#include <thread>
#include <chrono>
#include <logtrace.h>
#include <mds_papi.h>
#include <ncsencdec_pub.h>

#include <rde_cb.h>

#define RDE_MDS_PVT_SUBPART_VERSION 1

static MDS_HDL mds_hdl;

static uint32_t msg_encode(MDS_CALLBACK_ENC_INFO *enc_info) {
  struct rde_msg *msg;
  NCS_UBAID *uba;
  uint8_t *data;

  enc_info->o_msg_fmt_ver = 1;
  uba = enc_info->io_uba;
  msg = (struct rde_msg*) enc_info->i_msg;

  data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
  assert(data);
  ncs_encode_32bit(&data, msg->type);
  ncs_enc_claim_space(uba, sizeof(uint32_t));

  switch (msg->type) {
    case RDE_MSG_PEER_INFO_REQ:
    case RDE_MSG_PEER_INFO_RESP:
      data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
      assert(data);
      ncs_encode_32bit(&data, msg->info.peer_info.ha_role);
      ncs_enc_claim_space(uba, sizeof(uint32_t));
      break;

    default:
      assert(0);
      break;
  }

  return NCSCC_RC_SUCCESS;
}

static uint32_t msg_decode(MDS_CALLBACK_DEC_INFO *dec_info) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  struct rde_msg *msg;
  NCS_UBAID *uba;
  uint8_t *data;
  uint8_t data_buff[256];

  if (dec_info->i_fr_svc_id != NCSMDS_SVC_ID_RDE) {
    rc = NCSCC_RC_FAILURE;
    goto done;
  }

  if (dec_info->i_msg_fmt_ver != 1) {
    rc = NCSCC_RC_FAILURE;
    goto done;
  }

  msg = static_cast<rde_msg*>(malloc(sizeof(*msg)));
  assert(msg);

  dec_info->o_msg = msg;
  uba = dec_info->io_uba;

  data = ncs_dec_flatten_space(uba, data_buff, sizeof(uint32_t));
  assert(data);
  msg->type = static_cast<RDE_MSG_TYPE>(ncs_decode_32bit(&data));
  ncs_dec_skip_space(uba, sizeof(uint32_t));

  switch (msg->type) {
    case RDE_MSG_PEER_INFO_REQ:
    case RDE_MSG_PEER_INFO_RESP:
      data = ncs_dec_flatten_space(uba, data_buff, sizeof(uint32_t));
      assert(data);
      msg->info.peer_info.ha_role = static_cast<PCS_RDA_ROLE>(ncs_decode_32bit(
          &data));
      ncs_dec_skip_space(uba, sizeof(uint32_t));
      break;

    default:
      assert(0);
      break;
  }

  done: return rc;
}

static int mbx_send(RDE_MSG_TYPE type, MDS_DEST fr_dest, NODE_ID fr_node_id) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  struct rde_msg *msg = static_cast<rde_msg*>(calloc(1,
                                                     sizeof(struct rde_msg)));
  RDE_CONTROL_BLOCK *cb = rde_get_control_block();

  msg->type = type;
  msg->fr_dest = fr_dest;
  msg->fr_node_id = fr_node_id;

  if (ncs_ipc_send(&cb->mbx, (NCS_IPC_MSG*) msg,
                   NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
    LOG_ER("ncs_ipc_send FAILED");
    free(msg);
    rc = NCSCC_RC_FAILURE;
  }

  return rc;
}

static uint32_t mds_callback(struct ncsmds_callback_info *info) {
  struct rde_msg *msg;
  uint32_t rc = NCSCC_RC_SUCCESS;
  RDE_CONTROL_BLOCK *cb = rde_get_control_block();

  switch (info->i_op) {
    case MDS_CALLBACK_COPY:
      rc = NCSCC_RC_FAILURE;
      break;
    case MDS_CALLBACK_ENC:
      rc = msg_encode(&info->info.enc);
      break;
    case MDS_CALLBACK_DEC:
      rc = msg_decode(&info->info.dec);
      break;
    case MDS_CALLBACK_ENC_FLAT:
      break;
    case MDS_CALLBACK_DEC_FLAT:
      break;
    case MDS_CALLBACK_RECEIVE:
      msg = (struct rde_msg*) info->info.receive.i_msg;
      msg->fr_dest = info->info.receive.i_fr_dest;
      msg->fr_node_id = info->info.receive.i_node_id;
      if (ncs_ipc_send(&cb->mbx, (NCS_IPC_MSG*) info->info.receive.i_msg,
                       NCS_IPC_PRIORITY_NORMAL) != NCSCC_RC_SUCCESS) {
        LOG_ER("ncs_ipc_send FAILED");
        free(msg);
        rc = NCSCC_RC_FAILURE;
        goto done;
      }
      break;
    case MDS_CALLBACK_SVC_EVENT:
      if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
        TRACE("MDS DOWN dest: %" PRIx64 ", node ID: %x, svc_id: %d",
              info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id,
              info->info.svc_evt.i_svc_id);
        rc = mbx_send(RDE_MSG_PEER_DOWN, info->info.svc_evt.i_dest,
                      info->info.svc_evt.i_node_id);
      } else if (info->info.svc_evt.i_change == NCSMDS_UP) {
        TRACE("MDS UP dest: %" PRIx64 ", node ID: %x, svc_id: %d",
              info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id,
              info->info.svc_evt.i_svc_id);
        rc = mbx_send(RDE_MSG_PEER_UP, info->info.svc_evt.i_dest,
                      info->info.svc_evt.i_node_id);
      } else {
        TRACE("MDS %u dest: %" PRIx64 ", node ID: %x, svc_id: %d",
              info->info.svc_evt.i_change, info->info.svc_evt.i_dest,
              info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);
      }
      break;
    case MDS_CALLBACK_QUIESCED_ACK:
      break;
    case MDS_CALLBACK_DIRECT_RECEIVE:
      break;
    default:
      break;
  }
  done: return rc;
}

uint32_t rde_mds_register() {
  NCSADA_INFO ada_info;
  NCSMDS_INFO svc_info;
  MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_RDE };
  MDS_DEST mds_adest;

  TRACE_ENTER();

  ada_info.req = NCSADA_GET_HDLS;
  if (ncsada_api(&ada_info) != NCSCC_RC_SUCCESS) {
    LOG_ER("%s: NCSADA_GET_HDLS Failed", __FUNCTION__);
    return NCSCC_RC_FAILURE;
  }

  mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;
  mds_adest = ada_info.info.adest_get_hdls.o_adest;

  svc_info.i_mds_hdl = mds_hdl;
  svc_info.i_svc_id = NCSMDS_SVC_ID_RDE;
  svc_info.i_op = MDS_INSTALL;

  svc_info.info.svc_install.i_yr_svc_hdl = 0;
  svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE; /* node
                                                                    specific */
  svc_info.info.svc_install.i_svc_cb = mds_callback; /* callback */
  svc_info.info.svc_install.i_mds_q_ownership = false;
  svc_info.info.svc_install.i_mds_svc_pvt_ver = RDE_MDS_PVT_SUBPART_VERSION;

  if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
    LOG_ER("%s: MDS Install Failed", __FUNCTION__);
    return NCSCC_RC_FAILURE;
  }

  memset(&svc_info, 0, sizeof(NCSMDS_INFO));
  svc_info.i_mds_hdl = mds_hdl;
  svc_info.i_svc_id = NCSMDS_SVC_ID_RDE;
  svc_info.i_op = MDS_RED_SUBSCRIBE;
  svc_info.info.svc_subscribe.i_num_svcs = 1;
  svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
  svc_info.info.svc_subscribe.i_svc_ids = svc_id;

  if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
    LOG_ER("MDS Subscribe for redundancy Failed");
    return NCSCC_RC_FAILURE;
  }

  TRACE_LEAVE2("NodeId:%x, mds_adest:%" PRIx64, ncs_get_node_id(), mds_adest);

  return NCSCC_RC_SUCCESS;
}

uint32_t rde_mds_unregister() {
  NCSMDS_INFO mds_info;
  uint32_t rc = NCSCC_RC_SUCCESS;
  TRACE_ENTER();

  /* Un-install your service into MDS.
   No need to cancel the services that are subscribed */
  memset(&mds_info, 0, sizeof(NCSMDS_INFO));

  mds_info.i_mds_hdl = mds_hdl;
  mds_info.i_svc_id = NCSMDS_SVC_ID_RDE;
  mds_info.i_op = MDS_UNINSTALL;

  rc = ncsmds_api(&mds_info);
  if (rc != NCSCC_RC_SUCCESS) {
    LOG_WA("MDS Unregister Failed");
  }

  TRACE_LEAVE2("retval = %u", rc);
  return rc;
}

uint32_t rde_mds_send(struct rde_msg *msg, MDS_DEST to_dest) {
  NCSMDS_INFO info;
  uint32_t rc;

  for (int i = 0; i != 3; ++i) {
    TRACE("Sending %s to %" PRIx64, rde_msg_name[msg->type], to_dest);
    memset(&info, 0, sizeof(info));

    info.i_mds_hdl = mds_hdl;
    info.i_op = MDS_SEND;
    info.i_svc_id = NCSMDS_SVC_ID_RDE;

    info.info.svc_send.i_msg = msg;
    info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
    info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
    info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_RDE;
    info.info.svc_send.info.snd.i_to_dest = to_dest;

    rc = ncsmds_api(&info);
    if (NCSCC_RC_FAILURE == rc) {
      LOG_WA("Failed to send %s to %" PRIx64, rde_msg_name[msg->type],
             to_dest);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else {
      break;
    }
  }

  return rc;
}

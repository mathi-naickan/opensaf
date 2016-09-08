/*      -*- OpenSAF -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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
 * Author(s): Oracle
 *
 */
#include "osaf/services/saf/logsv/lgs/lgs.h"
#include "osaf/services/saf/logsv/lgs/lgs_clm.h"

static bool clm_initialized;
static void *clm_node_db = NULL;       /* used for C++ STL map */
typedef std::map<NODE_ID, lgs_clm_node_t *> ClmNodeMap;

/**
 * @brief Checks if LGSV has already initialized with CLM service.
 *
 * @return true/false.
 */
static bool is_clm_init() {
  return (((lgs_cb->clm_hdl != 0)
           && (clm_initialized == true)) ? true : false);
}

/**
 * Name     : lgs_clm_node_map_init
 * Description   : This routine is used to initialize the clm_node_map
 * Arguments     : lgs_cb - pointer to the lgs Control Block
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * Notes    : None
 */
uint32_t lgs_clm_node_map_init(lgs_cb_t *lgs_cb) {
  TRACE_ENTER();
  if (clm_node_db) {
    TRACE("CLM Node map already exists");
    return NCSCC_RC_FAILURE;
  }

  clm_node_db = new ClmNodeMap;
  TRACE_LEAVE();
  return NCSCC_RC_SUCCESS;
}


/**
 * Name     : lgs_clm_node_find
 * Description   : This routine finds the clm_node .
 * Arguments     : clm_node_id - CLM Node id
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * Notes    : None
 */
static uint32_t lgs_clm_node_find(NODE_ID clm_node_id) {
  TRACE_ENTER();
  uint32_t rc;

  ClmNodeMap *clmNodeMap(reinterpret_cast<ClmNodeMap *>
                         (clm_node_db));

  if (clmNodeMap) {
    ClmNodeMap::iterator it(clmNodeMap->find(clm_node_id));

    if (it != clmNodeMap->end()) {
      rc = NCSCC_RC_SUCCESS;
    } else {
      rc = NCSCC_RC_FAILURE;
      TRACE("clm_node_id not exist DB failed : %x", clm_node_id);
    }
  } else {
    TRACE("clmNodeMap DB not exist failed : %x", clm_node_id);
    rc = NCSCC_RC_FAILURE;
    //    osafassert(false);
  }

  TRACE_LEAVE();
  return rc;
}

/**
 * Name     : lgs_clm_node_add
 * Description   : This routine adds the new node to clm_node_map.
 * Arguments     : clm_node_id - CLM Node id
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * Notes    : None
 */
static uint32_t lgs_clm_node_add(NODE_ID clm_node_id) {
  TRACE_ENTER();
  uint32_t rc = NCSCC_RC_SUCCESS;
  lgs_clm_node_t *clm_node;

  clm_node = new lgs_clm_node_t();

  clm_node->clm_node_id = clm_node_id;

  ClmNodeMap *clmNodeMap(reinterpret_cast<ClmNodeMap *>
                         (clm_node_db));

  if (clmNodeMap) {
    std::pair<ClmNodeMap::iterator, bool> p(clmNodeMap->insert(
        std::make_pair(clm_node->clm_node_id, clm_node)));

    if (!p.second) {
      TRACE("unable to add clm node info map - the id %x already existed",
            clm_node->clm_node_id);
      rc = NCSCC_RC_FAILURE;
    }
  } else {
    TRACE("can't find local sec map in lgs_clm_node_add");
    rc = NCSCC_RC_FAILURE;
  }
  TRACE_LEAVE();
  return rc;
}

/**
 * Name     : lgs_clm_node_del
 * Description   : Function to Delete the clm_node.
 * Arguments     : clm_node_id - CLM Node id
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * Notes    : None.
 */
static uint32_t lgs_clm_node_del(NODE_ID clm_node_id) {
  TRACE_ENTER();
  uint32_t rc;
  lgs_clm_node_t *clm_node;

  ClmNodeMap *clmNodeMap(reinterpret_cast<ClmNodeMap *>
                         (clm_node_db));

  if (clmNodeMap) {
    auto it = (clmNodeMap->find(clm_node_id));

    if (it != clmNodeMap->end()) {
      clm_node = it->second;
      clmNodeMap->erase(it);
      delete clm_node;
      rc = NCSCC_RC_SUCCESS;
    } else {
      TRACE("clm_node_id delete  failed : %x", clm_node_id);
      rc = NCSCC_RC_FAILURE;
    }
  } else {
    TRACE("clm_node_id delete to map not exist failed : %x", clm_node_id);
    rc = NCSCC_RC_FAILURE;
  }

  TRACE_LEAVE();
  return rc;
}

/**
 * @brief  Send Membership status of node to a lib on that node.
 *
 * @param SaClmClusterChangesT (CLM status of node)
 * @param client_id
 * @param mdsDest of client
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
static uint32_t send_clm_node_status_lib(SaClmClusterChangesT clusterChange,
                                         unsigned int client_id, MDS_DEST mdsDest) {
  uint32_t rc;
  NCSMDS_INFO mds_info = {0};
  lgsv_msg_t msg;
  TRACE_ENTER();
  TRACE_3("change:%u, client_id: %u", clusterChange, client_id);

  memset(&msg, 0, sizeof(lgsv_msg_t));
  msg.type = LGSV_LGS_CBK_MSG;
  msg.info.cbk_info.type = LGSV_CLM_NODE_STATUS_CALLBACK;
  msg.info.cbk_info.lgs_client_id = client_id;
  msg.info.cbk_info.inv = 0;
  msg.info.cbk_info.clm_node_status_cbk.clm_node_status = clusterChange;

  mds_info.i_mds_hdl = lgs_cb->mds_hdl;
  mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
  mds_info.i_op = MDS_SEND;
  mds_info.info.svc_send.i_msg = &msg;
  mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_LGA;
  mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
  mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
  mds_info.info.svc_send.info.snd.i_to_dest = mdsDest;
  rc = ncsmds_api(&mds_info);
  if (rc != NCSCC_RC_SUCCESS)
    LOG_NO("Failed (%u) to send of WRITE ack to: %" PRIx64, rc, mdsDest);

  TRACE_LEAVE();
  return rc;
}

/**
 * @brief  Sends CLM membership status of the node to all the clients
 *         on the node except A11 clients.
 * @param  clusterChange (CLM membership status of node).
 * @param  NCS clm_node_id.
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
static uint32_t send_cluster_membership_msg_to_clients(
    SaClmClusterChangesT clusterChange, NODE_ID clm_node_id) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  log_client_t *rp = NULL;
  uint32_t client_id_net;

  TRACE_ENTER();
  TRACE_3("clm_node_id: %x, change:%u", clm_node_id, clusterChange);

  rp = reinterpret_cast<log_client_t *>
      (ncs_patricia_tree_getnext(&lgs_cb->client_tree, NULL));

  while (rp != NULL) {
    /** Store the client_id_net for get Next  */
    client_id_net = rp->client_id_net;
    NODE_ID tmp_clm_node_id = m_LGS_GET_NODE_ID_FROM_ADEST(rp->mds_dest);
    //  Do not send to A11 client. Send only to specific Node
    if (tmp_clm_node_id == clm_node_id)
      rc = send_clm_node_status_lib(clusterChange, rp->client_id,
                                    rp->mds_dest);

    rp = reinterpret_cast<log_client_t *>(ncs_patricia_tree_getnext(
        &lgs_cb->client_tree, reinterpret_cast<uint8_t *>(&client_id_net)));
  }

  TRACE_LEAVE();
  return rc;
}

static uint32_t send_clm_node_status_change(SaClmClusterChangesT clusterChange,
                                            NODE_ID clm_node_id) {
  return (send_cluster_membership_msg_to_clients(clusterChange, clm_node_id));
}

/**
 * @brief  Checks CLM membership status of a client.
 *         A.02.01 clients are always CLM member.
 * @param  Client MDS_DEST
 * @param  Client saf version.
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
bool is_client_clm_member(NODE_ID clm_node_id, SaVersionT *ver) {
  //  Before CLM init all clients are clm member.
  if (is_clm_init() == false)
    return true;

  TRACE("client Version: %d.%d.%d", ver->releaseCode,
        ver->majorVersion, ver->minorVersion);
  //  CLM integration is supported from A.02.02.
  //  So old clients A.02.01 are always clm member.
  if ((ver->releaseCode == LOG_RELEASE_CODE_0) &&
      (ver->majorVersion == LOG_MAJOR_VERSION_0) &&
      (ver->minorVersion == LOG_MINOR_VERSION_0))
    return true;
  /*
        It means CLM initialization is successful and this is atleast a A.02.02 client.
        So check CLM membership status of client's node.
  */
  if (lgs_clm_node_find(clm_node_id) != NCSCC_RC_SUCCESS)
    return false;
  else
    return true;
}

/*
 * @brief  CLM callback for tracking node membership status.
 *     Depending upon the membership status (joining/leaving cluster)
 *     of a node, LGS will add or remove node from its data base.
 *     A node is added when it is part of the cluster and is removed
 *     when it leaves the cluster membership. An update of status is
 *     sent to the clients on that node. Based on this status LGA
 *     will decide return code for different API calls.
 *
 */
static void lgs_clm_track_cbk(
    const SaClmClusterNotificationBufferT_4 *notificationBuffer,
    SaUint32T numberOfMembers, SaInvocationT invocation,
    const SaNameT *rootCauseEntity,
    const SaNtfCorrelationIdsT *correlationIds,
    SaClmChangeStepT step, SaTimeT timeSupervision,
    SaAisErrorT error) {

  NODE_ID clm_node_id;
  SaClmClusterChangesT clusterChange;
  SaBoolT is_member;
  uint32_t i = 0;

  TRACE_ENTER2("'%llu' '%u' '%u'", invocation, step, error);
  if (error != SA_AIS_OK) {
    TRACE_1("Error received in ClmTrackCallback");
    goto done;
  }
  clm_initialized = true;

  for (i = 0; i < notificationBuffer->numberOfItems; i++) {
    switch (step) {
      case SA_CLM_CHANGE_COMPLETED:
        is_member = notificationBuffer->notification[i].clusterNode.member;
        clm_node_id = notificationBuffer->notification[i].clusterNode.nodeId;
        if (lgs_clm_node_find(clm_node_id) == NCSCC_RC_SUCCESS) {
          TRACE_1("'%x' is present in LGS db", clm_node_id);
          if (!is_member) {
            TRACE("CLM Node : %x Left the cluster", clm_node_id);
            clusterChange = SA_CLM_NODE_LEFT;
            if (lgs_clm_node_del(clm_node_id) == NCSCC_RC_SUCCESS) {
              if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE)
                send_clm_node_status_change(clusterChange, clm_node_id);
            }
          }
        } else {
          TRACE_1("'%x' is not present in LGS db", clm_node_id);
          if (is_member) {
            TRACE("CLM Node : %x Joined the cluster", clm_node_id);
            clusterChange = SA_CLM_NODE_JOINED;
            if (lgs_clm_node_add(clm_node_id) == NCSCC_RC_SUCCESS) {
              if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE)
                send_clm_node_status_change(clusterChange, clm_node_id);
            }
          }
        }
        break;
      default:
        break;
    }
  }
done:
  TRACE_LEAVE();
  return;
}

/* saClmClusterTrackCallback */
static const SaClmCallbacksT_4 clm_callbacks = {
  0,
  lgs_clm_track_cbk
};

/*
 * @brief   Registers with the CLM service (B.04.01).
 *
 * @return  SaAisErrorT
 */
void *lgs_clm_init_thread(void *cb) {
  static SaVersionT clmVersion = { 'B', 0x04, 0x01 };
  lgs_cb_t *_lgs_cb = reinterpret_cast<lgs_cb_t *> (cb);
  SaAisErrorT rc;
  TRACE_ENTER();
  rc = saClmInitialize_4(&_lgs_cb->clm_hdl, &clm_callbacks, &clmVersion);
  if (rc != SA_AIS_OK) {
    LOG_ER("saClmInitialize failed with error: %d", rc);
    TRACE_LEAVE();
    exit(EXIT_FAILURE);
  }
  rc = saClmSelectionObjectGet(_lgs_cb->clm_hdl, &lgs_cb->clmSelectionObject);
  if (rc != SA_AIS_OK) {
    LOG_ER("saClmSelectionObjectGet failed with error: %d", rc);
    TRACE_LEAVE();
    exit(EXIT_FAILURE);
  }

  /* TODO:subscribe for SA_TRACK_START_STEP also. */
  rc = saClmClusterTrack_4(_lgs_cb->clm_hdl,
                           (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL);
  if (rc != SA_AIS_OK) {
    LOG_ER("saClmClusterTrack failed with error: %d", rc);
    TRACE_LEAVE();
    exit(EXIT_FAILURE);
  }
  TRACE("CLM Initialization SUCCESS......");
  TRACE_LEAVE();
  return NULL;
}

/*
 * @brief  Creates a thread to initialize with CLM.
 */
void lgs_init_with_clm(void) {
  pthread_t thread;
  pthread_attr_t attr;
  TRACE_ENTER();

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  if (pthread_create(&thread, &attr, lgs_clm_init_thread, lgs_cb) != 0) {
    LOG_ER("pthread_create FAILED: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  pthread_attr_destroy(&attr);
  TRACE_LEAVE();
}

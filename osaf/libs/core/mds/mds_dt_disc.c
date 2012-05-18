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
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#include "mds_dt.h"
#include "mds_dt_tcp.h"
#include "mds_dt_tcp_disc.h"
#include <sys/types.h>
#include <unistd.h>


uint16_t mdtm_num_subscriptions;
MDS_SUBTN_REF_VAL mdtm_handle;
extern pid_t mdtm_pid;

extern MDTM_INTRANODE_UNSENT_MSGS *mds_mdtm_msg_unsent_hdr;
extern MDTM_INTRANODE_UNSENT_MSGS *mds_mdtm_msg_unsent_tail;

struct pollfd pfd[2];

/* Encode function declarations */
static void mds_mdtm_enc_svc_subscribe(MDS_MDTM_DTM_MSG * svc_subscribe, uint8_t *buff);
static void mds_mdtm_enc_svc_unsubscribe(MDS_MDTM_DTM_MSG * svc_unsubscribe, uint8_t *buff);
static void mds_mdtm_enc_svc_install(MDS_MDTM_DTM_MSG * svc_install, uint8_t *buff);
static void mds_mdtm_enc_svc_uninstall(MDS_MDTM_DTM_MSG * svc_uninstall, uint8_t *buff);
static void mds_mdtm_enc_vdest_install(MDS_MDTM_DTM_MSG * vdest_install, uint8_t *buff);
static void mds_mdtm_enc_vdest_uninstall(MDS_MDTM_DTM_MSG * vdest_uninstall, uint8_t *buff);
static void mds_mdtm_enc_vdest_subscribe(MDS_MDTM_DTM_MSG * vdest_subscribe, uint8_t *buff);
static void mds_mdtm_enc_node_subscribe(MDS_MDTM_DTM_MSG * node_subscribe, uint8_t *buff);
static void mds_mdtm_enc_node_unsubscribe(MDS_MDTM_DTM_MSG * node_unsubscribe, uint8_t *buff);

/**
 * Function contains the logic for service subscribe
 *
 * @param pwe_id svc_id install_scope
 * @param svc_hdl subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_svc_subscribe_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				 MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	uint32_t server_type = 0, status = 0;
	MDS_MDTM_DTM_MSG subscr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE];

	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE);

	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	if (mdtm_num_subscriptions > MAX_SUBSCRIPTIONS) {
		m_MDS_LOG_ERR("MDTM: SYSTEM CRITICAL Crossing =%d subscriptions\n", mdtm_num_subscriptions);
		if (mdtm_num_subscriptions > MAX_SUBSCRIPTIONS_RETURN_ERROR) {
			m_MDS_LOG_ERR
			    ("MDTM: SYSTEM has crossed the max =%d subscriptions , Returning failure to the user",
			     MAX_SUBSCRIPTIONS_RETURN_ERROR);
			return NCSCC_RC_FAILURE;
		}
	}

	server_type = server_type | MDS_TCP_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;

	memset(&subscr, 0, sizeof(MDS_MDTM_DTM_MSG));

	subscr.size = MDS_MDTM_DTM_SUBSCRIBE_SIZE;
	subscr.mds_version = MDS_SND_VERSION;
	subscr.mds_indentifire = MDS_IDENTIFIRE;
	subscr.type = MDS_MDTM_DTM_SUBSCRIBE_TYPE;
	subscr.info.subscribe.scope_type = (install_scope - 1);
	subscr.info.subscribe.server_type = server_type;
	subscr.info.subscribe.server_instance_lower = MDS_MDTM_LOWER_INSTANCE;
	subscr.info.subscribe.server_instance_upper = MDS_MDTM_UPPER_INSTANCE;
	subscr.info.subscribe.sub_ref_val = ++mdtm_handle;
	subscr.info.subscribe.node_id = tcp_cb->node_id;
	subscr.info.subscribe.process_id = mdtm_pid;

	*subtn_ref_val = subscr.info.subscribe.sub_ref_val;

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_svc_subscribe(&subscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE);

	status = mdtm_add_to_ref_tbl(svc_hdl, *subtn_ref_val);

	++mdtm_num_subscriptions;
	m_MDS_LOG_INFO("MDTM: SVC-SUBSCRIBE Success\n");

	return status;
}

/**
 * Function contains the logic for service unsubscribe
 *
 * @param subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_svc_unsubscribe_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope, 
		MDS_SUBTN_REF_VAL subtn_ref_val)
{

	MDS_MDTM_DTM_MSG unsubscr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE];
	/*
	   STEP 1: Get ref_val and call the TCP unsubscribe with the ref_val
	 */

	memset(&tcp_buffer, 0, MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE);

	memset(&unsubscr, 0, sizeof(MDS_MDTM_DTM_MSG));

	unsubscr.size = MDS_MDTM_DTM_UNSUBSCRIBE_SIZE;
	unsubscr.mds_version = MDS_SND_VERSION;
	unsubscr.mds_indentifire = MDS_IDENTIFIRE;
	unsubscr.type = MDS_MDTM_DTM_UNSUBSCRIBE_TYPE;
	unsubscr.info.unsubscribe.sub_ref_val = subtn_ref_val;
	unsubscr.info.unsubscribe.node_id = tcp_cb->node_id;
	unsubscr.info.unsubscribe.process_id = mdtm_pid;

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_svc_unsubscribe(&unsubscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE);

	mdtm_del_from_ref_tbl(subtn_ref_val);
	--mdtm_num_subscriptions;

	m_MDS_LOG_INFO("MDTM: SVC-UNSUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;
}

/**
 * Function contains the logic for service install
 *
 * @param pwe_id, svc_id, install_scope, role
 * @param vdest_id, vdest_policy, mds_svc_pvt_ver
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_svc_install_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
			       V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
			       MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	uint32_t server_type = 0, server_inst = 0;
	MDS_MDTM_DTM_MSG svc_install;
	NCS_VDEST_TYPE policy = vdest_policy;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE];
	MDS_SVC_ARCHWORD_TYPE archword = MDS_SELF_ARCHWORD;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	memset(&svc_install, 0, sizeof(MDS_MDTM_DTM_MSG));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE);

	server_type = server_type | MDS_TCP_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;
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
	install_scope = install_scope - 1;
	server_inst |= (uint32_t)((install_scope & 0x3) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN - MDS_SCOPE_LEN));	/* Next 2  bit */
	server_inst |= vdest_id;

	svc_install.size = MDS_MDTM_DTM_SVC_INSTALL_SIZE;
	svc_install.mds_version = MDS_SND_VERSION;
	svc_install.mds_indentifire = MDS_IDENTIFIRE;
	svc_install.type = MDS_MDTM_DTM_BIND_TYPE;
	svc_install.info.bind.install_scope = install_scope;
	svc_install.info.bind.server_type = server_type;
	svc_install.info.bind.server_instance_lower = server_inst;
	svc_install.info.bind.server_instance_upper = server_inst;
	svc_install.info.bind.node_id = tcp_cb->node_id;
	svc_install.info.bind.process_id = mdtm_pid;
	svc_install.info.bind.install_scope = install_scope;

	m_MDS_LOG_INFO("MDTM: install_tcp : <%u,%u,%u>", server_type, server_inst, server_inst);

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_svc_install(&svc_install, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE);

	m_MDS_LOG_INFO("MDTM: SVC-INSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/**
 * Function contains the logic for service uninstall
 *
 * @param pwe_id, svc_id, install_scope, role
 * @param vdest_id, vdest_policy, mds_svc_pvt_ver
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_svc_uninstall_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				 V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
				 MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	uint32_t server_inst = 0, server_type = 0;
	MDS_MDTM_DTM_MSG svc_uninstall;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE];
	NCS_VDEST_TYPE policy = vdest_policy;
	MDS_SVC_ARCHWORD_TYPE archword = MDS_SELF_ARCHWORD;
	pwe_id = pwe_id << MDS_EVENT_SHIFT_FOR_PWE;
	svc_id = svc_id & MDS_EVENT_MASK_FOR_SVCID;

	memset(&svc_uninstall, 0, sizeof(MDS_MDTM_DTM_MSG));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE);

	server_type = server_type | MDS_TCP_PREFIX | MDS_SVC_INST_TYPE | pwe_id | svc_id;
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

	install_scope = install_scope - 1;
	server_inst |= (uint32_t)((install_scope & 0x3) << (LEN_4_BYTES - MDS_ARCHWORD_BITS_LEN - MDS_VER_BITS_LEN - VDEST_POLICY_LEN - ACT_STBY_LEN - MDS_SCOPE_LEN));	/* Next 2  bit */

	server_inst |= vdest_id;

	svc_uninstall.size = MDS_MDTM_DTM_SVC_UNINSTALL_SIZE;
	svc_uninstall.mds_version = MDS_SND_VERSION;
	svc_uninstall.mds_indentifire = MDS_IDENTIFIRE;
	svc_uninstall.type = MDS_MDTM_DTM_UNBIND_TYPE;
	svc_uninstall.info.unbind.install_scope = install_scope;
	svc_uninstall.info.unbind.server_type = server_type;
	svc_uninstall.info.unbind.server_instance_lower = server_inst;
	svc_uninstall.info.unbind.server_instance_upper = server_inst;
	svc_uninstall.info.unbind.node_id = tcp_cb->node_id;
	svc_uninstall.info.unbind.process_id = mdtm_pid;
	svc_uninstall.info.unbind.install_scope = install_scope;

	m_MDS_LOG_INFO("MDTM: uninstall_tcp : <%u,%u,%u>", server_type, server_inst, server_inst);

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_svc_uninstall(&svc_uninstall, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE);

	m_MDS_LOG_INFO("MDTM: SVC-UNINSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/**
 * Function contains the logic for vdest install
 *
 * @param vdest_id
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_vdest_install_tcp(MDS_VDEST_ID vdest_id)
{

	MDS_MDTM_DTM_MSG server_addr;
	uint32_t server_type = 0, server_inst = 0;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE];

	server_type = server_type | MDS_TCP_PREFIX | MDS_VDEST_INST_TYPE;
	server_inst |= vdest_id;

	memset(&server_addr, 0, sizeof(MDS_MDTM_BIND_MSG));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE);

	server_addr.size = MDS_MDTM_DTM_SVC_INSTALL_SIZE;
	server_addr.mds_version = MDS_SND_VERSION;
	server_addr.mds_indentifire = MDS_IDENTIFIRE;
	server_addr.type = MDS_MDTM_DTM_BIND_TYPE;
	server_addr.info.bind.server_type = server_type;
	server_addr.info.bind.server_instance_lower = server_inst;
	server_addr.info.bind.server_instance_upper = server_inst;
	server_addr.info.bind.node_id = tcp_cb->node_id;
	server_addr.info.bind.process_id = mdtm_pid;
	server_addr.info.bind.install_scope = (NCSMDS_SCOPE_NONE - 1);

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_vdest_install(&server_addr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE);

	m_MDS_LOG_INFO("MDTM: VDEST-INSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/**
 * Function contains the logic for vdest uninstall
 *
 * @param vdest_id
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_vdest_uninstall_tcp(MDS_VDEST_ID vdest_id)
{
	uint32_t server_inst = 0, server_type = 0;
	MDS_MDTM_DTM_MSG server_addr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE];

	server_type = server_type | MDS_TCP_PREFIX | MDS_VDEST_INST_TYPE;
	server_inst |= vdest_id;

	memset(&server_addr, 0, sizeof(MDS_MDTM_BIND_MSG));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE);

	server_addr.size = MDS_MDTM_DTM_SVC_UNINSTALL_SIZE;
	server_addr.mds_version = MDS_SND_VERSION;
	server_addr.mds_indentifire = MDS_IDENTIFIRE;
	server_addr.type = MDS_MDTM_DTM_UNBIND_TYPE;
	server_addr.info.unbind.server_type = server_type;
	server_addr.info.unbind.server_instance_lower = server_inst;
	server_addr.info.unbind.server_instance_upper = server_inst;
	server_addr.info.unbind.node_id = tcp_cb->node_id;
	server_addr.info.unbind.process_id = mdtm_pid;
	server_addr.info.unbind.install_scope = (NCSMDS_SCOPE_NONE - 1);

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_vdest_uninstall(&server_addr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE);

	m_MDS_LOG_INFO("MDTM: VDEST-UNINSTALL Success\n");
	return NCSCC_RC_SUCCESS;
}

/**
 * Function contains the logic for vdest subscribe
 *
 * @param vdest_id, subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_vdest_subscribe_tcp(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	uint32_t inst = 0, server_type = 0;
	MDS_MDTM_DTM_MSG subscr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE];

	memset(&tcp_buffer, 0, MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE);

	if (mdtm_num_subscriptions > MAX_SUBSCRIPTIONS) {
		m_MDS_LOG_ERR("MDTM: SYSTEM CRITICAL Crossing =%d subscriptions\n", mdtm_num_subscriptions);
		if (mdtm_num_subscriptions > MAX_SUBSCRIPTIONS_RETURN_ERROR) {
			m_MDS_LOG_ERR
			    ("MDTM: SYSTEM has crossed the max =%d subscriptions , Returning failure to the user",
			     MAX_SUBSCRIPTIONS_RETURN_ERROR);
			return NCSCC_RC_FAILURE;
		}
	}

	server_type = server_type | MDS_TCP_PREFIX | MDS_VDEST_INST_TYPE;
	inst |= vdest_id;
	memset(&subscr, 0, sizeof(subscr));

	subscr.size = MDS_MDTM_DTM_SUBSCRIBE_SIZE;
	subscr.mds_version = MDS_SND_VERSION;
	subscr.mds_indentifire = MDS_IDENTIFIRE;
	subscr.type = MDS_MDTM_DTM_SUBSCRIBE_TYPE;
	subscr.info.subscribe.server_type = server_type;
	subscr.info.subscribe.server_instance_lower = inst;
	subscr.info.subscribe.server_instance_upper = inst;
	subscr.info.subscribe.sub_ref_val = ++mdtm_handle;
	subscr.info.subscribe.node_id = tcp_cb->node_id;
	subscr.info.subscribe.process_id = mdtm_pid;
	subscr.info.subscribe.scope_type = (NCSMDS_SCOPE_NONE - 1);

	*subtn_ref_val = subscr.info.subscribe.sub_ref_val;
	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_vdest_subscribe(&subscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE);

	++mdtm_num_subscriptions;

	m_MDS_LOG_INFO("MDTM: VDEST-SUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;
}

/**
 *
 * @param vdest_id, subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 *
 */
uint32_t mds_mdtm_vdest_unsubscribe_tcp(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val)
{
	MDS_MDTM_DTM_MSG unsubscr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE];
	/*
	   STEP 1: Get ref_val and call the TCP unsubscribe with the ref_val
	 */

	memset(&tcp_buffer, 0, MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE);

	memset(&unsubscr, 0, sizeof(MDS_MDTM_DTM_MSG));

	unsubscr.size = MDS_MDTM_DTM_UNSUBSCRIBE_SIZE;
	unsubscr.mds_version = MDS_SND_VERSION;
	unsubscr.mds_indentifire = MDS_IDENTIFIRE;
	unsubscr.type = MDS_MDTM_DTM_UNSUBSCRIBE_TYPE;
	unsubscr.info.unsubscribe.sub_ref_val = subtn_ref_val;
	unsubscr.info.unsubscribe.node_id = tcp_cb->node_id;
	unsubscr.info.unsubscribe.process_id = mdtm_pid;

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_svc_unsubscribe(&unsubscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE);

	mdtm_del_from_ref_tbl(subtn_ref_val);
	--mdtm_num_subscriptions;

	m_MDS_LOG_INFO("MDTM: vdest-UNSUBSCRIBE Success\n");

	return NCSCC_RC_SUCCESS;
}

/**
 * Currently not in use
 *
 * @param adest
 *
 * @return NCSCC_RC_SUCCESS
 *
 */
uint32_t mds_mdtm_tx_hdl_register_tcp(MDS_DEST adest)
{
	return NCSCC_RC_SUCCESS;
}

/**
 * Currently not in use
 *
 * @param adest
 *
 * @return NCSCC_RC_SUCCESS
 *
 */
uint32_t mds_mdtm_tx_hdl_unregister_tcp(MDS_DEST adest)
{
	return NCSCC_RC_SUCCESS;
}


/**
 * Function contains the logic to subscribe to the node
 *
 * @param subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_node_subscribe_tcp(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	MDS_MDTM_DTM_MSG node_subscr;
	uint32_t status = NCSCC_RC_SUCCESS;
	uint8_t tcp_buffer[MDS_MDTM_DTM_NODE_SUBSCRIBE_BUFFER_SIZE];

	m_MDS_LOG_INFO("MDTM: In mds_mdtm_node_subscribe_tcp\n");
	memset(&node_subscr, 0, sizeof(node_subscr));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_NODE_SUBSCRIBE_BUFFER_SIZE);

	node_subscr.size = MDS_MDTM_DTM_NODE_SUBSCRIBE_SIZE;
	node_subscr.mds_version = MDS_SND_VERSION;
	node_subscr.mds_indentifire = MDS_IDENTIFIRE;
	node_subscr.type = MDS_MDTM_DTM_NODE_SUBSCRIBE_TYPE;
	node_subscr.info.node_subscribe.sub_ref_val = ++mdtm_handle;
	node_subscr.info.node_subscribe.node_id = tcp_cb->node_id;
	node_subscr.info.node_subscribe.process_id = mdtm_pid;

	*subtn_ref_val = node_subscr.info.node_subscribe.sub_ref_val;

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_node_subscribe(&node_subscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_NODE_SUBSCRIBE_BUFFER_SIZE);

	status = mdtm_add_to_ref_tbl(svc_hdl, *subtn_ref_val);
	++mdtm_num_subscriptions;
	m_MDS_LOG_INFO("MDTM: NODE-SUBSCRIBE Success\n");

	return status;
}

/**
 * Function contains the logic to unsubscribe to the node
 *
 * @param subtn_ref_val
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t mds_mdtm_node_unsubscribe_tcp(MDS_SUBTN_REF_VAL subtn_ref_val)
{
	MDS_MDTM_DTM_MSG node_unsubscr;
	uint8_t tcp_buffer[MDS_MDTM_DTM_NODE_UNSUBSCRIBE_BUFFER_SIZE];

	m_MDS_LOG_INFO("MDTM: In mds_mdtm_node_subscribe_tcp\n");

	memset(&node_unsubscr, 0, sizeof(node_unsubscr));
	memset(&tcp_buffer, 0, MDS_MDTM_DTM_NODE_UNSUBSCRIBE_BUFFER_SIZE);

	node_unsubscr.size = MDS_MDTM_DTM_NODE_UNSUBSCRIBE_SIZE;
	node_unsubscr.mds_version = MDS_SND_VERSION;
	node_unsubscr.mds_indentifire = MDS_IDENTIFIRE;
	node_unsubscr.type = MDS_MDTM_DTM_NODE_SUBSCRIBE_TYPE;
	node_unsubscr.info.node_unsubscribe.sub_ref_val = subtn_ref_val;
	node_unsubscr.info.node_unsubscribe.node_id = tcp_cb->node_id;
	node_unsubscr.info.node_unsubscribe.process_id = mdtm_pid;

	/* Convert into the encoded tcp_buffer before send */
	mds_mdtm_enc_node_unsubscribe(&node_unsubscr, tcp_buffer);

	/* Add the message to unsent queue if messages are already there otherwise send the message directly */
	mds_mdtm_unsent_queue_add_send(tcp_buffer, MDS_MDTM_DTM_NODE_UNSUBSCRIBE_BUFFER_SIZE);

	m_MDS_LOG_INFO("MDTM: In mds_mdtm_node_unsubscribe_tcp\n");

	mdtm_del_from_ref_tbl(subtn_ref_val);
	--mdtm_num_subscriptions;
	m_MDS_LOG_INFO("MDTM: NODE-UNSUBSCRIBE Success\n");
	return NCSCC_RC_SUCCESS;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_svc_subscribe(MDS_MDTM_DTM_MSG * svc_subscribe, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, svc_subscribe->size);
	ncs_encode_32bit(&buff, svc_subscribe->mds_indentifire);
	ncs_encode_8bit(&buff, svc_subscribe->mds_version);
	ncs_encode_8bit(&buff, (uint8_t)svc_subscribe->type);
	ncs_encode_8bit(&buff, (uint8_t)svc_subscribe->info.subscribe.scope_type);
	ncs_encode_32bit(&buff, svc_subscribe->info.subscribe.server_type);
	ncs_encode_32bit(&buff, svc_subscribe->info.subscribe.server_instance_lower);
	ncs_encode_32bit(&buff, svc_subscribe->info.subscribe.server_instance_upper);
	ncs_encode_64bit(&buff, svc_subscribe->info.subscribe.sub_ref_val);
	ncs_encode_32bit(&buff, svc_subscribe->info.subscribe.node_id);
	ncs_encode_32bit(&buff, svc_subscribe->info.subscribe.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_svc_unsubscribe(MDS_MDTM_DTM_MSG * svc_unsubscribe, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, svc_unsubscribe->size);
	ncs_encode_32bit(&buff, svc_unsubscribe->mds_indentifire);
	ncs_encode_8bit(&buff, svc_unsubscribe->mds_version);
	ncs_encode_8bit(&buff, svc_unsubscribe->type);
	ncs_encode_64bit(&buff, svc_unsubscribe->info.unsubscribe.sub_ref_val);
	ncs_encode_32bit(&buff, svc_unsubscribe->info.unsubscribe.node_id);
	ncs_encode_32bit(&buff, svc_unsubscribe->info.unsubscribe.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_svc_install(MDS_MDTM_DTM_MSG * svc_install, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, svc_install->size);
	ncs_encode_32bit(&buff, svc_install->mds_indentifire);
	ncs_encode_8bit(&buff, svc_install->mds_version);
	ncs_encode_8bit(&buff, svc_install->type);
	ncs_encode_8bit(&buff, svc_install->info.bind.install_scope);
	ncs_encode_32bit(&buff, svc_install->info.bind.server_type);
	ncs_encode_32bit(&buff, svc_install->info.bind.server_instance_lower);
	ncs_encode_32bit(&buff, svc_install->info.bind.server_instance_upper);
	ncs_encode_32bit(&buff, svc_install->info.bind.node_id);
	ncs_encode_32bit(&buff, svc_install->info.bind.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_svc_uninstall(MDS_MDTM_DTM_MSG * svc_uninstall, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, svc_uninstall->size);
	ncs_encode_32bit(&buff, svc_uninstall->mds_indentifire);
	ncs_encode_8bit(&buff, svc_uninstall->mds_version);
	ncs_encode_8bit(&buff, svc_uninstall->type);
	ncs_encode_8bit(&buff, svc_uninstall->info.unbind.install_scope);
	ncs_encode_32bit(&buff, svc_uninstall->info.unbind.server_type);
	ncs_encode_32bit(&buff, svc_uninstall->info.unbind.server_instance_lower);
	ncs_encode_32bit(&buff, svc_uninstall->info.unbind.server_instance_upper);
	ncs_encode_32bit(&buff, svc_uninstall->info.unbind.node_id);
	ncs_encode_32bit(&buff, svc_uninstall->info.unbind.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_vdest_install(MDS_MDTM_DTM_MSG * vdest_install, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, vdest_install->size);
	ncs_encode_32bit(&buff, vdest_install->mds_indentifire);
	ncs_encode_8bit(&buff, vdest_install->mds_version);
	ncs_encode_8bit(&buff, vdest_install->type);
	ncs_encode_8bit(&buff, vdest_install->info.bind.install_scope);
	ncs_encode_32bit(&buff, vdest_install->info.bind.server_type);
	ncs_encode_32bit(&buff, vdest_install->info.bind.server_instance_lower);
	ncs_encode_32bit(&buff, vdest_install->info.bind.server_instance_upper);
	ncs_encode_32bit(&buff, vdest_install->info.bind.node_id);
	ncs_encode_32bit(&buff, vdest_install->info.bind.process_id);

	return;

}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_vdest_uninstall(MDS_MDTM_DTM_MSG * vdest_uninstall, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, vdest_uninstall->size);
	ncs_encode_32bit(&buff, vdest_uninstall->mds_indentifire);
	ncs_encode_8bit(&buff, vdest_uninstall->mds_version);
	ncs_encode_8bit(&buff, vdest_uninstall->type);
	ncs_encode_8bit(&buff, vdest_uninstall->info.unbind.install_scope);
	ncs_encode_32bit(&buff, vdest_uninstall->info.unbind.server_type);
	ncs_encode_32bit(&buff, vdest_uninstall->info.unbind.server_instance_lower);
	ncs_encode_32bit(&buff, vdest_uninstall->info.unbind.server_instance_upper);
	ncs_encode_32bit(&buff, vdest_uninstall->info.unbind.node_id);
	ncs_encode_32bit(&buff, vdest_uninstall->info.unbind.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_vdest_subscribe(MDS_MDTM_DTM_MSG * vdest_subscribe, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, vdest_subscribe->size);
	ncs_encode_32bit(&buff, vdest_subscribe->mds_indentifire);
	ncs_encode_8bit(&buff, vdest_subscribe->mds_version);
	ncs_encode_8bit(&buff, (uint8_t)vdest_subscribe->type);
	ncs_encode_8bit(&buff, (uint8_t)vdest_subscribe->info.subscribe.scope_type);
	ncs_encode_32bit(&buff, vdest_subscribe->info.subscribe.server_type);
	ncs_encode_32bit(&buff, vdest_subscribe->info.subscribe.server_instance_lower);
	ncs_encode_32bit(&buff, vdest_subscribe->info.subscribe.server_instance_upper);
	ncs_encode_64bit(&buff, vdest_subscribe->info.subscribe.sub_ref_val);
	ncs_encode_32bit(&buff, vdest_subscribe->info.subscribe.node_id);
	ncs_encode_32bit(&buff, vdest_subscribe->info.subscribe.process_id);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_node_subscribe(MDS_MDTM_DTM_MSG * node_subscribe, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, node_subscribe->size);
	ncs_encode_32bit(&buff, node_subscribe->mds_indentifire);
	ncs_encode_8bit(&buff, node_subscribe->mds_version);
	ncs_encode_8bit(&buff, node_subscribe->type);
	ncs_encode_32bit(&buff, node_subscribe->info.node_subscribe.node_id);
	ncs_encode_32bit(&buff, node_subscribe->info.node_subscribe.process_id);
	ncs_encode_64bit(&buff, node_subscribe->info.node_subscribe.sub_ref_val);

	return;
}

/**
 * Encode the send message
 *
 * @param Send buffer, Data packet
 *
 */
static void mds_mdtm_enc_node_unsubscribe(MDS_MDTM_DTM_MSG * node_unsubscribe, uint8_t *data)
{
	uint8_t *buff = data;
	ncs_encode_16bit(&buff, node_unsubscribe->size);
	ncs_encode_32bit(&buff, node_unsubscribe->mds_indentifire);
	ncs_encode_8bit(&buff, node_unsubscribe->mds_version);
	ncs_encode_8bit(&buff, node_unsubscribe->type);
	ncs_encode_32bit(&buff, node_unsubscribe->info.node_unsubscribe.node_id);
	ncs_encode_32bit(&buff, node_unsubscribe->info.node_unsubscribe.process_id);
	ncs_encode_64bit(&buff, node_unsubscribe->info.node_unsubscribe.sub_ref_val);

	return;
}

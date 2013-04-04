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

/* Notes
   -----
  We have two types of send
       1. Normal send in which the user provides MDS with a  pointer to the message of type NCS_CONTEXT.
           As we dont know the actual type of memory structure to the pointer being provided,
            MDS will not free this memory in all the failures and success cases of sends.
       2. Direct send in which the user provides MDS with a flat buffer pointer and length 
           of the buffer. Memory will be allocated to the flat buffer by using the MDS api
           m_MDS_ALLOC_DIRECT_BUFF(size) (where size is the number of bytes of memory to be
           allocated and is limited to 8000 bytes). As buffer pointer is allocated by MDS, in all the
           cases of success and failure of DIRECT sends, memory will be freed by the MDS and application
           should not free the memory.

*/

#include "mds_core.h"
#include "mds_papi.h"
#include "mds_log.h"
#include "ncssysf_mem.h"

uint32_t mds_mcm_global_exchange_id = 0;

#define SUCCESS 0
#define FAILURE 1

#define MDS_SENDTYPE_ACK     14
#define MDS_SENDTYPE_RACK    15

typedef enum mds_bcast_enum {

/* The following part is used only for the BCAST and Red BCAST sends only  and no where it is used */
	BCAST_ENC_FLAT = 2,
	BCAST_ENC = 4,

} MDS_BCAST_ENUM;

/* Following structure is only used in the bcast/red bcast for various ver types */
typedef struct mds_bcast_buff_list {

	struct mds_bcast_buff_list *next;
	MDS_BCAST_ENUM bcast_flag;
	USRBUF *bcast_enc_flat;
	USRBUF *bcast_enc;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;
	MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_part_ver;
	MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word;
} MDS_BCAST_BUFF_LIST;

#define m_MMGR_ALLOC_BCAST_BUFF_LIST  m_NCS_MEM_ALLOC(sizeof(MDS_BCAST_BUFF_LIST), \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_BCAST_BUFF_LIST)

#define m_MMGR_FREE_BCAST_BUFF_LIST(p)  m_NCS_MEM_FREE(p, \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_BCAST_BUFF_LIST)

typedef struct send_msg {
#define MSG_NCSCONTEXT   1
#define MSG_DIRECT_BUFF  2

	uint8_t msg_type;
	union {
		NCSCONTEXT msg;
		MDS_DIRECT_BUFF_INFO info;
	} data;

	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;	/* Used for only direct sends */
	MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_part_ver;	/* To add in MDS hdr */
	MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word;

	/* The following part is used only for the BCAST and Red BCAST sends only  and no where it is used */
	MDS_BCAST_BUFF_LIST *mds_bcast_list_hdr;
} SEND_MSG;

/* Functions for bcast list add, search and free all the list */
static uint32_t mds_mcm_add_bcast_list(SEND_MSG *msg, MDS_BCAST_ENUM bcast_enum, USRBUF *usr_buf,
				    MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_ver, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver,
				    MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word);

static uint32_t mds_mcm_search_bcast_list(SEND_MSG *msg, MDS_BCAST_ENUM bcast_enum,
				       MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_ver,
				       MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word, MDS_BCAST_BUFF_LIST **mds_mcm_bcast_ptr,
				       bool flag);

static uint32_t mds_mcm_del_all_bcast_list(SEND_MSG *msg);

#define MDS_MSG_SENT 3

uint32_t mds_send(NCSMDS_INFO *info);

static uint32_t mds_mcm_free_msg_memory(MDS_ENCODED_MSG msg);

static uint32_t mcm_pvt_normal_svc_snd(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_pvt_red_svc_snd(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				 NCSCONTEXT msg, MDS_DEST to_dest,
				 V_DEST_QA anchor,
				 MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_pvt_normal_svc_sndrsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				       NCSCONTEXT msg, MDS_DEST to_dest,
				       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_normal_svc_sndack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				       NCSCONTEXT msg, MDS_DEST to_dest,
				       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_normal_svc_sndrack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					NCSCONTEXT msg, MDS_DEST to_dest,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_normal_svc_snd_rsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					NCSCONTEXT msg, MDS_DEST to_dest,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_red_svc_sndrsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    V_DEST_QA anchor,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_red_svc_sndrack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				     NCSCONTEXT msg, MDS_DEST to_dest,
				     V_DEST_QA anchor,
				     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_red_svc_sndack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    V_DEST_QA anchor,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);
static uint32_t mcm_pvt_red_svc_snd_rsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				     NCSCONTEXT msg, MDS_DEST to_dest,
				     V_DEST_QA anchor,
				     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_pvt_normal_svc_bcast(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				      NCSCONTEXT msg,
				      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
				      NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_pvt_red_svc_bcast(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				   NCSCONTEXT msg,
				   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
				   NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri);

/* Direct Send*/
static uint32_t mcm_pvt_normal_svc_snd_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

static uint32_t mcm_pvt_red_svc_snd_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					V_DEST_QA anchor,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

static uint32_t mcm_pvt_normal_svc_sndrsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_normal_svc_sndack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_normal_svc_sndrack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_normal_svc_snd_rsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_red_svc_sndrsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   V_DEST_QA anchor,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_red_svc_sndrack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					    V_DEST_QA anchor,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_red_svc_sndack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   V_DEST_QA anchor,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);
static uint32_t mcm_pvt_red_svc_snd_rsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					    V_DEST_QA anchor,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

static uint32_t mcm_pvt_normal_svc_bcast_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					     MDS_DIRECT_BUFF buff, uint16_t len,
					     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					     NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					     MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

static uint32_t mcm_pvt_red_svc_bcast_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					  MDS_DIRECT_BUFF buff, uint16_t len,
					  MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					  NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					  MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

uint32_t mds_mcm_ll_data_rcv(MDS_DATA_RECV *recv);
static uint32_t mds_mcm_process_recv_snd_msg_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_snd(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_snd(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_bcast(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_bcast(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_sndrsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_sndrsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mds_mcm_process_recv_sndrack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_sndrack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_sndrack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mds_mcm_process_recv_sndack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_sndack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_sndack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mds_mcm_process_rcv_snd_rsp_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_snd_rsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_snd_rsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mds_mcm_process_recv_ack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_normal_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);
static uint32_t mcm_recv_red_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv);

static uint32_t mds_mcm_mailbox_post(MDS_SVC_INFO *dest_svc_cb, MDS_DATA_RECV *recv, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mds_mcm_do_decode_full_or_flat(MDS_SVC_INFO *svccb, NCSMDS_CALLBACK_INFO *cbinfo, MDS_DATA_RECV *recv_msg,
					    NCSCONTEXT orig_msg);
static uint32_t mds_mcm_send_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv, uint8_t flag, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mds_mcm_direct_send(NCSMDS_INFO *info);

static uint32_t mds_mcm_send(NCSMDS_INFO *info);

static uint32_t mcm_query_for_node_dest(MDS_DEST adest, uint8_t *to);

static uint32_t mcm_query_for_node_dest_on_archword(MDS_DEST adest, uint8_t *to, MDS_SVC_ARCHWORD_TYPE arch_word);

static uint32_t mcm_pvt_normal_snd_process_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       SEND_MSG msg, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, uint32_t xch_id);

static uint32_t mds_mcm_process_disc_queue_checks(MDS_SVC_INFO *svc_cb, MDS_SVC_ID dest_svc_id,
					       MDS_DEST i_dest, MDS_SEND_INFO *req, MDS_DEST *o_dest,
					       bool *timer_running, MDS_SUBSCRIPTION_RESULTS_INFO **tx_send_hdl);

static uint32_t mcm_process_await_active(MDS_SVC_INFO *svc_cb, MDS_SUBSCRIPTION_RESULTS_INFO *send_hdl,
				      MDS_SVC_ID to_svc_id, SEND_MSG *msg, MDS_VDEST_ID to_vdest,
				      uint32_t snd_type, uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_msg_direct_send_buff(uint8_t to, MDS_DIRECT_BUFF_INFO buff,
				      MDS_SVC_ID to_svc_id, MDS_SVC_INFO *svc_cb,
				      MDS_DEST adest, MDS_VDEST_ID to_vdest, uint32_t snd_type,
				      uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver);

static uint32_t mcm_msg_encode_full_or_flat_and_send(uint8_t to, SEND_MSG *to_msg,
						  MDS_SVC_ID to_svc_id, MDS_SVC_INFO *svc_cb,
						  MDS_DEST adest, MDS_VDEST_ID to_vdest, uint32_t snd_type,
						  uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mds_subtn_tbl_add_disc_queue(MDS_SUBSCRIPTION_INFO *sub_info, MDS_SEND_INFO *req,
					  MDS_VDEST_ID dest_vdest_id, MDS_DEST dest,
					  MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id, bool time_wait);

static uint32_t mcm_pvt_red_snd_process_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    SEND_MSG msg, MDS_DEST to_dest,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, uint32_t xch_id, V_DEST_QA anchor);

static uint32_t mds_mcm_process_disc_queue_checks_redundant(MDS_SVC_INFO *svc_cb, MDS_SVC_ID dest_svc_id,
							 MDS_VDEST_ID dest_vdest_id, V_DEST_QA anchor,
							 MDS_SEND_INFO *req);

static uint32_t mcm_pvt_create_sync_send_entry(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_SENDTYPES snd, uint32_t xch_id,
					    MDS_MCM_SYNC_SEND_QUEUE **sync_queue, NCSCONTEXT sent_msg);

static uint32_t mcm_pvt_process_sndrack_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    SEND_MSG msg, MDS_DEST to_dest,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, V_DEST_QA anchor);

static uint32_t mds_mcm_time_wait(NCS_SEL_OBJ sel_obj, uint32_t time);

static uint32_t mcm_pvt_get_sync_send_entry(MDS_SVC_INFO *svc_cb, MDS_DATA_RECV *recv,
					 MDS_MCM_SYNC_SEND_QUEUE **sync_queue);
static uint32_t mcm_pvt_del_sync_send_entry(MDS_PWE_HDL env_hdl, MDS_SVC_ID fr_svc_id, uint32_t xch_id,
					 MDS_SENDTYPES snd_type, MDS_DEST adest);

static uint32_t mds_mcm_raise_selection_obj_for_ack(MDS_SVC_INFO *svc_cb, MDS_DATA_RECV *recv);

uint32_t mds_retrieve(NCSMDS_INFO *info);

static uint32_t mcm_pvt_process_svc_bcast_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      SEND_MSG to_msg,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					      uint8_t flag /* For normal=0, red=1 */ );

static uint32_t mds_mcm_check_intranode(MDS_DEST adest);

#define MDS_GET_NODE_ID(p) m_MDS_GET_NODE_ID_FROM_ADEST(p)

/* New added functions after PL changes*/
static uint32_t mds_mcm_send_msg_enc(uint8_t to, MDS_SVC_INFO *svc_cb, SEND_MSG *to_msg,
				  MDS_SVC_ID to_svc_id, MDS_VDEST_ID dest_vdest_id,
				  MDS_SEND_INFO *req, uint32_t xch_id, MDS_DEST dest, MDS_SEND_PRIORITY_TYPE pri);

static uint32_t mcm_msg_cpy_send(uint8_t to, MDS_SVC_INFO *svc_cb, SEND_MSG *to_msg,
			      MDS_SVC_ID to_svc_id, MDS_VDEST_ID dest_vdest_id,
			      MDS_SEND_INFO *req, uint32_t xch_id, MDS_DEST dest, MDS_SEND_PRIORITY_TYPE pri);

uint32_t mds_await_active_tbl_add(MDS_SUBSCRIPTION_RESULTS_INFO *info, MDTM_SEND_REQ req);
uint32_t mds_await_active_tbl_del(MDS_AWAIT_ACTIVE_QUEUE *queue);
uint32_t mds_await_active_tbl_send(MDS_AWAIT_ACTIVE_QUEUE *queue, MDS_DEST adest, MDS_SVC_HDL svc_hdl);

/* For deleting a single node entry in await active, when timeout occurs before the noactive timer expires*/
static uint32_t mds_await_active_tbl_del_entry(MDS_PWE_HDL env_hdl, MDS_SVC_ID fr_svc_id, uint32_t xch_id,
					    MDS_SVC_ID to_svc_id, MDS_VDEST_ID vdest, MDS_SENDTYPES snd_type);

static uint32_t mds_subtn_tbl_del_disc_queue(MDS_SUBSCRIPTION_INFO *sub_info, NCS_SEL_OBJ obj);

static uint32_t mds_check_for_mds_existence(NCS_SEL_OBJ sel_obj, MDS_HDL env_hdl,
					 MDS_SVC_ID fr_svc_id, MDS_SVC_ID to_svc_id);

static uint32_t mds_validate_svc_cb(MDS_SVC_INFO *svc_cb, MDS_SVC_HDL svc_hdl, MDS_SVC_ID svc_id);

/* Move to header file*/

/* Log enhancements */
#define m_MDS_ERR_PRINT_ADEST(x)   m_MDS_LOG_ERR("MDS_SND_RCV: Adest=<0x%08x,%u>",m_MDS_GET_NODE_ID_FROM_ADEST(x), m_MDS_GET_PROCESS_ID_FROM_ADEST(x))
#define m_MDS_ERR_PRINT_ANCHOR(y)   m_MDS_LOG_ERR("MDS_SND_RCV: Anchor=<0x%08x,%u>",m_MDS_GET_NODE_ID_FROM_ADEST(y), m_MDS_GET_PROCESS_ID_FROM_ADEST(y))

/****************************************************************************
 *
 * Function Name: mds_send
 *
 * Purpose:       This function executes the send service requested by the
 *                service. This includes all forms of send, including bcast.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

uint32_t mds_send(NCSMDS_INFO *info)
{

	/* Extract the type of send first either send or direct send */
	uint32_t snd_type_major = 0;
	snd_type_major = info->i_op;

	switch (snd_type_major) {
	case MDS_SEND:
		return mds_mcm_send(info);
		break;

	case MDS_DIRECT_SEND:
		return mds_mcm_direct_send(info);
		break;

	default:
		m_MDS_LOG_ERR("MDS_SND_RCV: Send Type Not supported Neither send nor direct send \n");
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 *
 * Function Name: mds_mcm_direct_send
 *
 * Purpose:       This function executes the send service requested by the
 *                service. This includes all forms of send, including bcast.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

static uint32_t mds_mcm_direct_send(NCSMDS_INFO *info)
{
	MDS_SEND_INFO req;
	uint32_t status;

	memset(&req, 0, sizeof(req));
	if ((info->info.svc_direct_send.i_priority < MDS_SEND_PRIORITY_LOW) ||
	    (info->info.svc_direct_send.i_priority > MDS_SEND_PRIORITY_VERY_HIGH)) {
		if (info->info.svc_direct_send.i_direct_buff != NULL) {
			m_MDS_FREE_DIRECT_BUFF(info->info.svc_direct_send.i_direct_buff);
			info->info.svc_direct_send.i_direct_buff = NULL;
		}
		m_MDS_LOG_ERR("MDS_SND_RCV: Priority defined is not in range\n");
		return NCSCC_RC_FAILURE;
	}

	if (info->info.svc_direct_send.i_direct_buff == NULL) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Send Message Direct Buff is NULL\n");
		return NCSCC_RC_FAILURE;
	} else if (info->info.svc_direct_send.i_direct_buff_len > MDS_DIRECT_BUF_MAXSIZE) {
		mds_free_direct_buff(info->info.svc_direct_send.i_direct_buff);
		m_MDS_LOG_ERR("MDS_SND_RCV: Send Message Direct Buff Len is greater than SEND SIZE\n");
		return NCSCC_RC_FAILURE;
	}

	if ((info->info.svc_direct_send.i_to_svc == 0) || (info->i_svc_id == 0)) {
		m_MDS_FREE_DIRECT_BUFF(info->info.svc_direct_send.i_direct_buff);
		m_MDS_LOG_ERR("MDS_SND_RCV: Source or Dest service provided is Null, src_svc=%, dest_svc=%d \n",
			      info->i_svc_id, info->info.svc_direct_send.i_to_svc);
		return NCSCC_RC_FAILURE;
	}

	req.i_to_svc = info->info.svc_direct_send.i_to_svc;

	switch (info->info.svc_direct_send.i_sendtype) {
	case MDS_SENDTYPE_SND:
		req.i_sendtype = MDS_SENDTYPE_SND;
		status = mcm_pvt_normal_svc_snd_direct(info->i_mds_hdl, info->i_svc_id,
						       info->info.svc_direct_send.i_direct_buff,
						       info->info.svc_direct_send.i_direct_buff_len,
						       info->info.svc_direct_send.info.snd.i_to_dest,
						       info->info.svc_direct_send.i_to_svc,
						       &req,
						       info->info.svc_direct_send.i_priority,
						       info->info.svc_direct_send.i_msg_fmt_ver);
		break;

	case MDS_SENDTYPE_SNDRSP:
		req.i_sendtype = MDS_SENDTYPE_SNDRSP;
		req.info.sndrsp.i_time_to_wait = info->info.svc_direct_send.info.sndrsp.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndrsp_direct(info->i_mds_hdl, info->i_svc_id,
							  info->info.svc_direct_send.i_direct_buff,
							  info->info.svc_direct_send.i_direct_buff_len,
							  info->info.svc_direct_send.info.sndrsp.i_to_dest,
							  info->info.svc_direct_send.i_to_svc,
							  &req,
							  info->info.svc_direct_send.i_priority,
							  info->info.svc_direct_send.i_msg_fmt_ver);
		if (status == NCSCC_RC_SUCCESS) {
			info->info.svc_direct_send.info.sndrsp.buff = req.info.sndrsp.buff;
			info->info.svc_direct_send.info.sndrsp.len = req.info.sndrsp.len;
			info->info.svc_direct_send.info.sndrsp.o_msg_fmt_ver = req.info.sndrsp.o_msg_fmt_ver;
		}

		break;

	case MDS_SENDTYPE_SNDRACK:
		req.i_sendtype = MDS_SENDTYPE_SNDRACK;
		req.info.sndrack.i_msg_ctxt = info->info.svc_direct_send.info.sndrack.i_msg_ctxt;
		req.info.sndrack.i_time_to_wait = info->info.svc_direct_send.info.sndrack.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndrack_direct(info->i_mds_hdl, info->i_svc_id,
							   info->info.svc_direct_send.i_direct_buff,
							   info->info.svc_direct_send.i_direct_buff_len,
							   info->info.svc_direct_send.info.sndrack.i_sender_dest,
							   info->info.svc_direct_send.i_to_svc,
							   &req,
							   info->info.svc_direct_send.i_priority,
							   info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_SNDACK:
		req.i_sendtype = MDS_SENDTYPE_SNDACK;
		req.info.sndack.i_time_to_wait = info->info.svc_direct_send.info.sndack.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndack_direct(info->i_mds_hdl, info->i_svc_id,
							  info->info.svc_direct_send.i_direct_buff,
							  info->info.svc_direct_send.i_direct_buff_len,
							  info->info.svc_direct_send.info.sndack.i_to_dest,
							  info->info.svc_direct_send.i_to_svc,
							  &req,
							  info->info.svc_direct_send.i_priority,
							  info->info.svc_direct_send.i_msg_fmt_ver);
		break;

	case MDS_SENDTYPE_RSP:
		req.i_sendtype = MDS_SENDTYPE_RSP;
		req.info.rsp.i_msg_ctxt = info->info.svc_direct_send.info.rsp.i_msg_ctxt;
		status = mcm_pvt_normal_svc_snd_rsp_direct(info->i_mds_hdl, info->i_svc_id,
							   info->info.svc_direct_send.i_direct_buff,
							   info->info.svc_direct_send.i_direct_buff_len,
							   info->info.svc_direct_send.info.rsp.i_sender_dest,
							   info->info.svc_direct_send.i_to_svc,
							   &req,
							   info->info.svc_direct_send.i_priority,
							   info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_RED:
		req.i_sendtype = MDS_SENDTYPE_RED;
		req.info.red.i_to_anc = info->info.svc_direct_send.info.red.i_to_anc;
		req.info.red.i_to_vdest = info->info.svc_direct_send.info.red.i_to_vdest;

		status = mcm_pvt_red_svc_snd_direct(info->i_mds_hdl, info->i_svc_id,
						    info->info.svc_direct_send.i_direct_buff,
						    info->info.svc_direct_send.i_direct_buff_len,
						    info->info.svc_direct_send.info.red.i_to_vdest,
						    info->info.svc_direct_send.info.red.i_to_anc,
						    info->info.svc_direct_send.i_to_svc,
						    &req,
						    info->info.svc_direct_send.i_priority,
						    info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_REDRSP:
		req.i_sendtype = MDS_SENDTYPE_REDRSP;
		req.info.redrsp.i_time_to_wait = info->info.svc_direct_send.info.redrsp.i_time_to_wait;
		status = mcm_pvt_red_svc_sndrsp_direct(info->i_mds_hdl, info->i_svc_id,
						       info->info.svc_direct_send.i_direct_buff,
						       info->info.svc_direct_send.i_direct_buff_len,
						       info->info.svc_direct_send.info.redrsp.i_to_vdest,
						       info->info.svc_direct_send.info.redrsp.i_to_anc,
						       info->info.svc_direct_send.i_to_svc,
						       &req,
						       info->info.svc_direct_send.i_priority,
						       info->info.svc_direct_send.i_msg_fmt_ver);
		if (status == NCSCC_RC_SUCCESS) {
			info->info.svc_direct_send.info.redrsp.buff = req.info.redrsp.buff;
			info->info.svc_direct_send.info.redrsp.len = req.info.redrsp.len;
			info->info.svc_direct_send.info.redrsp.o_msg_fmt_ver = req.info.redrsp.o_msg_fmt_ver;
		}

		break;

	case MDS_SENDTYPE_REDRACK:
		req.i_sendtype = MDS_SENDTYPE_REDRACK;
		req.info.redrack.i_msg_ctxt = info->info.svc_direct_send.info.redrack.i_msg_ctxt;
		req.info.redrack.i_time_to_wait = info->info.svc_direct_send.info.redrack.i_time_to_wait;
		status = mcm_pvt_red_svc_sndrack_direct(info->i_mds_hdl, info->i_svc_id,
							info->info.svc_direct_send.i_direct_buff,
							info->info.svc_direct_send.i_direct_buff_len,
							info->info.svc_direct_send.info.redrack.i_to_vdest,
							info->info.svc_direct_send.info.redrack.i_to_anc,
							info->info.svc_direct_send.i_to_svc,
							&req,
							info->info.svc_direct_send.i_priority,
							info->info.svc_direct_send.i_msg_fmt_ver);
		break;

	case MDS_SENDTYPE_REDACK:
		req.i_sendtype = MDS_SENDTYPE_REDACK;
		req.info.redack.i_time_to_wait = info->info.svc_direct_send.info.redack.i_time_to_wait;
		status = mcm_pvt_red_svc_sndack_direct(info->i_mds_hdl, info->i_svc_id,
						       info->info.svc_direct_send.i_direct_buff,
						       info->info.svc_direct_send.i_direct_buff_len,
						       info->info.svc_direct_send.info.redack.i_to_vdest,
						       info->info.svc_direct_send.info.redack.i_to_anc,
						       info->info.svc_direct_send.i_to_svc,
						       &req,
						       info->info.svc_direct_send.i_priority,
						       info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_RRSP:
		req.i_sendtype = MDS_SENDTYPE_RRSP;
		req.info.rrsp.i_msg_ctxt = info->info.svc_direct_send.info.rrsp.i_msg_ctxt;
		status = mcm_pvt_red_svc_snd_rsp_direct(info->i_mds_hdl, info->i_svc_id,
							info->info.svc_direct_send.i_direct_buff,
							info->info.svc_direct_send.i_direct_buff_len,
							info->info.svc_direct_send.info.rrsp.i_to_dest,
							info->info.svc_direct_send.info.rrsp.i_to_anc,
							info->info.svc_direct_send.i_to_svc,
							&req,
							info->info.svc_direct_send.i_priority,
							info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_BCAST:
		req.i_sendtype = MDS_SENDTYPE_BCAST;
		status = mcm_pvt_normal_svc_bcast_direct(info->i_mds_hdl, info->i_svc_id,
							 info->info.svc_direct_send.i_direct_buff,
							 info->info.svc_direct_send.i_direct_buff_len,
							 info->info.svc_direct_send.i_to_svc,
							 &req,
							 info->info.svc_direct_send.info.bcast.i_bcast_scope,
							 info->info.svc_direct_send.i_priority,
							 info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	case MDS_SENDTYPE_RBCAST:
		req.i_sendtype = MDS_SENDTYPE_RBCAST;
		status = mcm_pvt_red_svc_bcast_direct(info->i_mds_hdl, info->i_svc_id,
						      info->info.svc_direct_send.i_direct_buff,
						      info->info.svc_direct_send.i_direct_buff_len,
						      info->info.svc_direct_send.i_to_svc,
						      &req,
						      info->info.svc_direct_send.info.rbcast.i_bcast_scope,
						      info->info.svc_direct_send.i_priority,
						      info->info.svc_direct_send.i_msg_fmt_ver);

		break;

	default:
		m_MDS_LOG_ERR("MDS_SND_RCV: Send type not supported");
		status = NCSCC_RC_FAILURE;
		break;
	}
	if (status == MDS_INT_RC_DIRECT_SEND_FAIL) {
		/* Free the MDS Direct Buff */
		m_MDS_FREE_DIRECT_BUFF(info->info.svc_direct_send.i_direct_buff);
		status = NCSCC_RC_FAILURE;
	}
	return status;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_send
 *
 * Purpose:       This function executes the send service requested by the
 *                service. This includes all forms of send, including bcast.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_send(NCSMDS_INFO *info)
{
	/*
	   STEP 1: Extract the SEND_TYPE
	   Each individual send function is called from this function based on the send_type
	 */
	uint32_t status = NCSCC_RC_SUCCESS;
	MDS_SEND_INFO req;

	memset(&req, 0, sizeof(req));
	if ((info->info.svc_send.i_priority < MDS_SEND_PRIORITY_LOW) ||
	    (info->info.svc_send.i_priority > MDS_SEND_PRIORITY_VERY_HIGH)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Priority defined is not in range");
		return NCSCC_RC_FAILURE;
	}

	if (info->info.svc_send.i_msg == NULL) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Message sent in Null");
		return NCSCC_RC_FAILURE;
	}

	req.i_to_svc = info->info.svc_send.i_to_svc;

	switch (info->info.svc_send.i_sendtype) {
	case MDS_SENDTYPE_SND:
		req.i_sendtype = MDS_SENDTYPE_SND;
		status = mcm_pvt_normal_svc_snd(info->i_mds_hdl, info->i_svc_id,
						info->info.svc_send.i_msg,
						info->info.svc_send.info.snd.i_to_dest,
						info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);
		break;

	case MDS_SENDTYPE_SNDRSP:
		req.i_sendtype = MDS_SENDTYPE_SNDRSP;
		req.info.sndrsp.i_time_to_wait = info->info.svc_send.info.sndrsp.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndrsp(info->i_mds_hdl, info->i_svc_id,
						   info->info.svc_send.i_msg,
						   info->info.svc_send.info.sndrsp.i_to_dest,
						   info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);
		if (status == NCSCC_RC_SUCCESS) {
			info->info.svc_send.info.sndrsp.o_rsp = req.info.sndrsp.o_rsp;
			info->info.svc_send.info.sndrsp.o_msg_fmt_ver = req.info.sndrsp.o_msg_fmt_ver;
		}

		break;

	case MDS_SENDTYPE_SNDRACK:
		req.i_sendtype = MDS_SENDTYPE_SNDRACK;
		req.info.sndrack.i_msg_ctxt = info->info.svc_send.info.sndrack.i_msg_ctxt;
		req.info.sndrack.i_time_to_wait = info->info.svc_send.info.sndrack.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndrack(info->i_mds_hdl, info->i_svc_id,
						    info->info.svc_send.i_msg,
						    info->info.svc_send.info.sndrack.i_sender_dest,
						    info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_SNDACK:
		req.i_sendtype = MDS_SENDTYPE_SNDACK;
		req.info.sndack.i_time_to_wait = info->info.svc_send.info.sndack.i_time_to_wait;
		status = mcm_pvt_normal_svc_sndack(info->i_mds_hdl, info->i_svc_id,
						   info->info.svc_send.i_msg,
						   info->info.svc_send.info.sndack.i_to_dest,
						   info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);
		break;

	case MDS_SENDTYPE_RSP:
		req.i_sendtype = MDS_SENDTYPE_RSP;
		req.info.rsp.i_msg_ctxt = info->info.svc_send.info.rsp.i_msg_ctxt;
		status = mcm_pvt_normal_svc_snd_rsp(info->i_mds_hdl, info->i_svc_id,
						    info->info.svc_send.i_msg,
						    info->info.svc_send.info.rsp.i_sender_dest,
						    info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_RED:
		req.i_sendtype = MDS_SENDTYPE_RED;
		req.info.red.i_to_anc = info->info.svc_send.info.red.i_to_anc;
		req.info.red.i_to_vdest = info->info.svc_send.info.red.i_to_vdest;

		status = mcm_pvt_red_svc_snd(info->i_mds_hdl, info->i_svc_id,
					     info->info.svc_send.i_msg,
					     info->info.svc_send.info.red.i_to_vdest,
					     info->info.svc_send.info.red.i_to_anc,
					     info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_REDRSP:
		req.i_sendtype = MDS_SENDTYPE_REDRSP;
		req.info.redrsp.i_time_to_wait = info->info.svc_send.info.redrsp.i_time_to_wait;
		status = mcm_pvt_red_svc_sndrsp(info->i_mds_hdl, info->i_svc_id,
						info->info.svc_send.i_msg,
						info->info.svc_send.info.redrsp.i_to_vdest,
						info->info.svc_send.info.redrsp.i_to_anc,
						info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);
		if (status == NCSCC_RC_SUCCESS) {
			info->info.svc_send.info.redrsp.o_rsp = req.info.redrsp.o_rsp;
			info->info.svc_send.info.redrsp.o_msg_fmt_ver = req.info.redrsp.o_msg_fmt_ver;
		}

		break;

	case MDS_SENDTYPE_REDRACK:
		req.i_sendtype = MDS_SENDTYPE_REDRACK;
		req.info.redrack.i_msg_ctxt = info->info.svc_send.info.redrack.i_msg_ctxt;
		req.info.redrack.i_time_to_wait = info->info.svc_send.info.redrack.i_time_to_wait;
		status = mcm_pvt_red_svc_sndrack(info->i_mds_hdl, info->i_svc_id,
						 info->info.svc_send.i_msg,
						 info->info.svc_send.info.redrack.i_to_vdest,
						 info->info.svc_send.info.redrack.i_to_anc,
						 info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);
		break;

	case MDS_SENDTYPE_REDACK:
		req.i_sendtype = MDS_SENDTYPE_REDACK;
		req.info.redack.i_time_to_wait = info->info.svc_send.info.redack.i_time_to_wait;
		status = mcm_pvt_red_svc_sndack(info->i_mds_hdl, info->i_svc_id,
						info->info.svc_send.i_msg,
						info->info.svc_send.info.redack.i_to_vdest,
						info->info.svc_send.info.redack.i_to_anc,
						info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_RRSP:
		req.i_sendtype = MDS_SENDTYPE_RRSP;
		req.info.rrsp.i_msg_ctxt = info->info.svc_send.info.rrsp.i_msg_ctxt;
		status = mcm_pvt_red_svc_snd_rsp(info->i_mds_hdl, info->i_svc_id,
						 info->info.svc_send.i_msg,
						 info->info.svc_send.info.rrsp.i_to_dest,
						 info->info.svc_send.info.rrsp.i_to_anc,
						 info->info.svc_send.i_to_svc, &req, info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_BCAST:
		req.i_sendtype = MDS_SENDTYPE_BCAST;
		status = mcm_pvt_normal_svc_bcast(info->i_mds_hdl, info->i_svc_id,
						  info->info.svc_send.i_msg,
						  info->info.svc_send.i_to_svc,
						  &req,
						  info->info.svc_send.info.bcast.i_bcast_scope,
						  info->info.svc_send.i_priority);

		break;

	case MDS_SENDTYPE_RBCAST:
		req.i_sendtype = MDS_SENDTYPE_RBCAST;
		status = mcm_pvt_red_svc_bcast(info->i_mds_hdl, info->i_svc_id,
					       info->info.svc_send.i_msg,
					       info->info.svc_send.i_to_svc,
					       &req,
					       info->info.svc_send.info.rbcast.i_bcast_scope,
					       info->info.svc_send.i_priority);

		break;

	default:
		m_MDS_LOG_ERR("MDS_SND_RCV: Sendtype not supported");
		status = NCSCC_RC_FAILURE;
		break;
	}
	return status;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_snd
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
     * Trivial validations already done?
    STEP 1: From input get Dest-SVC-ID, Destination <ADEST, VDEST>, PWE-ID

    STEP 2: Do a route lookup and match for active role

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist)
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                ** Subscription timer running (or just started) **
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            - if ((no matching entry exists in no-actives table||
                        (Destination role=Standby && NO_ACTIVE_TMR is not running/Expired))
                return route-not-found
            - if (not direct send)
                o Do full encode
            - if (NO_ACTIVE_TMR is running)
            {
                Enqueue <sel-obj>, <ready-to-go-msg> in
                await-active-table
                return Sucess;
            }
            else
                return route-not-found
        }
        else
        {
            ** reaching here implies route is available**
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
    *
*****************************************************************************/

static uint32_t mcm_pvt_normal_svc_snd(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	uint32_t xch_id = 0, status = 0;
	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mcm_pvt_normal_svc_snd\n");

	status = mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);

	if (status == NCSCC_RC_SUCCESS) {
		m_MDS_LOG_INFO("MDS_SND_RCV: Normal send Message sent successfully from svc_id= %d, to svc_id=%d\n",
			       fr_svc_id, to_svc_id);
		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_pvt_normal_svc_snd\n");
		return NCSCC_RC_SUCCESS;
	} else {
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal send Message sent Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_pvt_normal_svc_snd\n");
		return status;
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_snd_process_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_pvt_normal_snd_process_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       SEND_MSG to_msg, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, uint32_t xch_id)
{
	MDS_SVC_ID dest_svc_id, src_svc_id;
	MDS_VDEST_ID dest_vdest_id;
	MDS_DEST dest, ret_adest = 0;
	bool timer_running = 0;
	uint8_t to = 0;		/* Destination on same process, node or off node */
	NCSCONTEXT hdl;

	MDS_PWE_HDL pwe_hdl = (MDS_PWE_HDL)env_hdl;
	MDS_SVC_INFO *svc_cb = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *tx_send_hdl = NULL;	/* Subscription Result */

	uint32_t status = 0;

	MDS_SUBSCRIPTION_RESULTS_INFO *subs_result_hdl = NULL;
	V_DEST_RL role_ret = 0;	/* Used only to get the subscription result ptr */

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mcm_pvt_normal_snd_process_common\n");

	if (to_msg.msg_type == MSG_NCSCONTEXT) {
		if (to_msg.data.msg == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Null Message\n");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (to_msg.data.info.buff == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: NULL Message\n");
			return NCSCC_RC_FAILURE;
		}

	}

	src_svc_id = fr_svc_id;
	dest_svc_id = to_svc_id;

	/* Get SVC_cb */
	if (NCSCC_RC_SUCCESS != (mds_svc_tbl_get(pwe_hdl, src_svc_id, &hdl))) {
		m_MDS_LOG_ERR("MDS_SND_RCV: SVC not present=%d\n", src_svc_id);
		if (to_msg.msg_type == MSG_DIRECT_BUFF) {
			return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
		}
		return NCSCC_RC_FAILURE;
	}

	svc_cb = (MDS_SVC_INFO *)hdl;

	dest_svc_id = to_svc_id;

	dest_vdest_id = m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest);

	if (dest_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
		dest = to_dest;

		/* Query one type to get the tx_send_hdl(results) */
		if (NCSCC_RC_SUCCESS !=
		    mds_subtn_res_tbl_query_by_adest(svc_cb->svc_hdl, dest_svc_id, dest_vdest_id, dest)) {
			tx_send_hdl = NULL;
		} else {
			/* Route available, send the data now */
			goto SEND_NOW;
		}
	} else {
		/* Destination svc present on vdest */

		/* Check dest_svc_id, dest_pwe_id, Destination <ADEST, VDEST>,
		   exists in subscription result table */
		dest = 0;	/* CAUTION  is this correct */
		bool flag = 0;
		if (svc_cb->parent_vdest_info->policy == NCS_VDEST_TYPE_MxN)
			flag = 1;
		if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get(svc_cb->svc_hdl, dest_svc_id, dest_vdest_id,
							      &ret_adest, &timer_running, &tx_send_hdl, flag)) {
			/* Destination Route Not Found, still some validations required */
			tx_send_hdl = NULL;
		}
	}

	if (tx_send_hdl == NULL) {
		/* Check in subscriptions whether this exists */
		if (NCSCC_RC_SUCCESS != mds_mcm_process_disc_queue_checks(svc_cb, dest_svc_id, to_dest, req,
									  &ret_adest, &timer_running, &tx_send_hdl)) {
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV:No Route Found from SVC id = %d to SVC id = %d on ADEST <0x%08x, %u>",
			     fr_svc_id, to_svc_id, m_MDS_GET_NODE_ID_FROM_ADEST(to_dest),
			     m_MDS_GET_PROCESS_ID_FROM_ADEST(to_dest));
			if (to_msg.msg_type == MSG_DIRECT_BUFF) {
				return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
			}
			return NCSCC_RC_FAILURE;
		}
	} else if ((tx_send_hdl != NULL) && (timer_running == true)) {
		/* Route Exists active or timer running */
		m_MDS_LOG_INFO("MDS_SND_RCV:Destination is in await active mode so queuing in await active\n");
		status =
		    mcm_process_await_active(svc_cb, tx_send_hdl, to_svc_id, &to_msg, dest_vdest_id, req->i_sendtype,
					     xch_id, pri);
		return status;
	}

	dest = ret_adest;

 SEND_NOW:
	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get_by_adest(svc_cb->svc_hdl, to_svc_id, dest_vdest_id,
							       dest, &role_ret, &subs_result_hdl)) {
		/* Destination Route Not Found */
		subs_result_hdl = NULL;
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Query for Destination failed, This case cannot exist as this has been validated before Src_svc=%d, Dest_scv=%d, vdest=%d, ADEST=%llx",
		     svc_cb->svc_id, to_svc_id, dest_vdest_id, dest);
		return NCSCC_RC_FAILURE;
	}
	mcm_query_for_node_dest_on_archword(dest, &to, subs_result_hdl->rem_svc_arch_word);
#if 1
	/* New code */
	status = mds_mcm_send_msg_enc(to, svc_cb, &to_msg, to_svc_id, dest_vdest_id, req, xch_id, dest, pri);
	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_pvt_normal_snd_process_common\n");
	return status;
#endif

}

/****************************************************************************
 *
 * Function Name: mds_mcm_send_msg_enc
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mds_mcm_send_msg_enc(uint8_t to, MDS_SVC_INFO *svc_cb, SEND_MSG *to_msg,
				  MDS_SVC_ID to_svc_id, MDS_VDEST_ID dest_vdest_id,
				  MDS_SEND_INFO *req, uint32_t xch_id, MDS_DEST dest, MDS_SEND_PRIORITY_TYPE pri)
{
	MDS_SUBSCRIPTION_RESULTS_INFO *subs_result_hdl = NULL;
	V_DEST_RL role = 0;
	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mds_mcm_send_msg_enc\n");

	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get_by_adest(svc_cb->svc_hdl, to_svc_id, dest_vdest_id,
							       dest, &role, &subs_result_hdl)) {
		/* Destination Route Not Found */
		subs_result_hdl = NULL;
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Query for Destination failed, This case cannot exist as this has been validated before Src_svc=%d, Dest_scv=%d, vdest=%d, ADEST=%llx",
		     svc_cb->svc_id, to_svc_id, dest_vdest_id, dest);
		return NCSCC_RC_FAILURE;
	}

	to_msg->rem_svc_sub_part_ver = subs_result_hdl->rem_svc_sub_part_ver;
	to_msg->rem_svc_arch_word = subs_result_hdl->rem_svc_arch_word;

	if (to == DESTINATION_SAME_PROCESS) {
		m_MDS_LOG_INFO("MDS_SND_RCV: Msg Destination is on same process\n");
		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mds_mcm_send_msg_enc\n");
		return mcm_msg_cpy_send(to, svc_cb, to_msg, to_svc_id, dest_vdest_id, req, xch_id, dest, pri);

	} else if ((to == DESTINATION_OFF_NODE) || (to == DESTINATION_ON_NODE)) {
		m_MDS_LOG_INFO("MDS_SND_RCV: Msg Destination is on off node or diff process\n");
		if (to_msg->msg_type == MSG_DIRECT_BUFF) {
			m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mds_mcm_send_msg_enc\n");
			return mcm_msg_direct_send_buff(to, to_msg->data.info, to_svc_id,
							svc_cb, dest, dest_vdest_id, req->i_sendtype, xch_id, pri,
							to_msg->msg_fmt_ver);
		} else if (MSG_NCSCONTEXT == to_msg->msg_type) {
			m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mds_mcm_send_msg_enc\n");
			return mcm_msg_encode_full_or_flat_and_send(to, to_msg, to_svc_id,
								    svc_cb, dest, dest_vdest_id, req->i_sendtype,
								    xch_id, pri);
		}
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mcm_msg_cpy_send
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_msg_cpy_send(uint8_t to, MDS_SVC_INFO *svc_cb, SEND_MSG *to_msg,
			      MDS_SVC_ID to_svc_id, MDS_VDEST_ID dest_vdest_id,
			      MDS_SEND_INFO *i_req, uint32_t xch_id, MDS_DEST dest, MDS_SEND_PRIORITY_TYPE pri)
{
	NCSMDS_CALLBACK_INFO cbinfo;
	NCSCONTEXT cpy = NULL;
	NCSCONTEXT orig_msg = NULL;
	MDTM_SEND_REQ req;
	MDS_SVC_INFO *orig_sender_cb;
	MDS_PWE_HDL orig_sender_pwe_hdl;
	MDS_DATA_RECV recv;
	MDS_MCM_SYNC_SEND_QUEUE *result;
	MDS_BCAST_BUFF_LIST *bcast_ptr = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *lcl_subtn_res = NULL;
	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mcm_msg_cpy_send\n");


	memset(&cbinfo, 0, sizeof(cbinfo));
	memset(&req, 0, sizeof(req));
	memset(&recv, 0, sizeof(recv));

	if (MSG_DIRECT_BUFF == to_msg->msg_type) {
		/* Presently nothing, no call back required so doing nothing for this case */

	} else if (MSG_NCSCONTEXT == to_msg->msg_type) {
		/* Shuroo */
		orig_msg = NULL;
		switch (i_req->i_sendtype) {
		case MDS_SENDTYPE_RSP:
		case MDS_SENDTYPE_SNDRACK:
		case MDS_SENDTYPE_RRSP:
		case MDS_SENDTYPE_REDRACK:
			recv.exchange_id = xch_id;
			recv.snd_type = i_req->i_sendtype;

			orig_sender_pwe_hdl =
			    m_MDS_GET_PWE_HDL_FROM_PWE_ID_AND_VDEST_ID(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl),
								       dest_vdest_id);
			if (NCSCC_RC_SUCCESS !=
			    (mds_svc_tbl_get(orig_sender_pwe_hdl, to_svc_id, ((NCSCONTEXT)&orig_sender_cb)))) {
				m_MDS_LOG_ERR("MDS_SND_RCV: (Local) dest not found SVC=%d, VDEST=%d, PWE=%d\n",
					      to_svc_id, dest_vdest_id, m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl));
				m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_cpy_send\n");
				return NCSCC_RC_FAILURE;
			}
			if (mcm_pvt_get_sync_send_entry(orig_sender_cb, &recv, &result) != NCSCC_RC_SUCCESS) {
				m_MDS_LOG_ERR("MDS_SND_RCV: Sync entry doesnt exist\n");
				m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_cpy_send\n");
				return NCSCC_RC_FAILURE;
			}
			orig_msg = result->orig_msg;
			break;
			/* Khatam */
		default:
			break;
		}

		if (((i_req->i_sendtype == MDS_SENDTYPE_BCAST) || (i_req->i_sendtype == MDS_SENDTYPE_RBCAST)) &&
		    (NCSCC_RC_SUCCESS == (mds_mcm_search_bcast_list(to_msg, BCAST_ENC, to_msg->rem_svc_sub_part_ver,
								    to_msg->rem_svc_arch_word, &bcast_ptr, 1)))) {
			/* Here we are assigning becoz, already we have the enc buffer, no need to call again the enc call back.
			   so just ditto it to the new one */
			req.msg.data.fullenc_uba.start = m_MMGR_DITTO_BUFR(bcast_ptr->bcast_enc);
			req.msg_fmt_ver = bcast_ptr->msg_fmt_ver;
			req.msg_arch_word = bcast_ptr->rem_svc_arch_word;
		} else {
			if ((i_req->i_sendtype == MDS_SENDTYPE_BCAST) || (i_req->i_sendtype == MDS_SENDTYPE_RBCAST)) {
				cbinfo.i_op = MDS_CALLBACK_ENC;
				cbinfo.info.enc.i_msg = to_msg->data.msg;
				cbinfo.info.enc.i_to_svc_id = to_svc_id;
				cbinfo.info.enc.io_uba = NULL;
				cbinfo.info.enc.io_uba = &req.msg.data.fullenc_uba;
				/* svc subpart ver to be filled */
				cbinfo.info.enc.i_rem_svc_pvt_ver = to_msg->rem_svc_sub_part_ver;

				if (ncs_enc_init_space_pp(&req.msg.data.fullenc_uba, NCSUB_MDS_POOL, 0) !=
				    NCSCC_RC_SUCCESS) {
					m_MDS_LOG_ERR("MDS_SND_RCV: encode full init failed svc_id=%d\n",
						      svc_cb->svc_id);
					return NCSCC_RC_FAILURE;
				}

			} else {
				cbinfo.i_op = MDS_CALLBACK_COPY;
				cbinfo.info.cpy.i_msg = to_msg->data.msg;
				cbinfo.info.cpy.i_to_svc_id = to_svc_id;
				cbinfo.info.cpy.o_cpy = orig_msg;
				/* svc subpart ver to be filled */
				cbinfo.info.cpy.i_rem_svc_pvt_ver = to_msg->rem_svc_sub_part_ver;
			}
			cbinfo.i_yr_svc_id = svc_cb->svc_id;
			cbinfo.i_yr_svc_hdl = svc_cb->yr_svc_hdl;

			/* CAUTION what to fill here */

			if (NCSCC_RC_SUCCESS != svc_cb->cback_ptr(&cbinfo)) {
				if ((i_req->i_sendtype == MDS_SENDTYPE_BCAST)
				    || (i_req->i_sendtype == MDS_SENDTYPE_RBCAST)) {
					m_MDS_LOG_ERR
					    ("MDS_C_SNDRCV; ENCode full callback  failed(broadcast) svc_id=%d\n",
					     to_svc_id);
				} else {
					m_MDS_LOG_ERR(" Copy callback failed svc_id=%d\n", to_svc_id);
				}
				return NCSCC_RC_FAILURE;
			}

			if ((i_req->i_sendtype == MDS_SENDTYPE_BCAST) || (i_req->i_sendtype == MDS_SENDTYPE_RBCAST)) {
				/* For the bcast case store the enc buffer */
				if (NCSCC_RC_FAILURE ==
				    mds_mcm_add_bcast_list(to_msg, BCAST_ENC, req.msg.data.fullenc_uba.start,
							   to_msg->rem_svc_sub_part_ver, cbinfo.info.enc.o_msg_fmt_ver,
							   to_msg->rem_svc_arch_word)) {
					m_MDS_LOG_ERR("MDS_C_SNDRCV: Addition to bcast list failed in enc case");
					return NCSCC_RC_FAILURE;
				}
			} else {
				/* This is for normal copy cases */
				cpy = cbinfo.info.cpy.o_cpy;
			}
		}
	}

	/* Now actually send the data */
	req.pri = pri;
	req.to = to;
	req.src_svc_id = svc_cb->svc_id;
	
	/* Get the subtn res tbl send cnt */
	mds_get_subtn_res_tbl_by_adest(svc_cb->svc_hdl, to_svc_id,
			dest_vdest_id, dest, &lcl_subtn_res);
	req.svc_seq_num = lcl_subtn_res->msg_snd_cnt++;
	req.src_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.src_vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.src_adest = m_MDS_GET_ADEST;
	req.xch_id = xch_id;
	req.snd_type = i_req->i_sendtype;

	req.dest_svc_id = to_svc_id;
	req.dest_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.dest_vdest_id = dest_vdest_id;

	if (MSG_DIRECT_BUFF == to_msg->msg_type) {
		req.msg.encoding = MDS_ENC_TYPE_DIRECT_BUFF;
		req.msg.data.buff_info.buff = to_msg->data.info.buff;
		req.msg.data.buff_info.len = to_msg->data.info.len;
		req.msg_fmt_ver = to_msg->msg_fmt_ver;
	} else {
		if ((i_req->i_sendtype == MDS_SENDTYPE_BCAST) || (i_req->i_sendtype == MDS_SENDTYPE_RBCAST)) {
			req.msg.encoding = MDS_ENC_TYPE_FULL;

			if (bcast_ptr == NULL) {
				req.msg_fmt_ver = cbinfo.info.enc.o_msg_fmt_ver;
				req.msg_arch_word = to_msg->rem_svc_arch_word;
				req.src_svc_sub_part_ver = svc_cb->svc_sub_part_ver;
			}
		} else {
			/* NCS CONTEXT Message */
			req.msg.encoding = MDS_ENC_TYPE_CPY;
			req.msg.data.cpy_msg = cpy;

			req.msg_fmt_ver = cbinfo.info.cpy.o_msg_fmt_ver;
			req.src_svc_sub_part_ver = svc_cb->svc_sub_part_ver;
		}
	}
	req.adest = dest;

	m_MDS_LOG_INFO("MDS_SND_RCV: Sending the data to MDTM layer\n");
	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_cpy_send\n");
	return mds_mdtm_send(&req);
}

/****************************************************************************
 *
 * Function Name: mcm_msg_direct_send_buff
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_msg_direct_send_buff(uint8_t to, MDS_DIRECT_BUFF_INFO buff_info,
				      MDS_SVC_ID to_svc_id, MDS_SVC_INFO *svc_cb,
				      MDS_DEST adest, MDS_VDEST_ID dest_vdest_id,
				      uint32_t snd_type, uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri,
				      MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	MDTM_SEND_REQ req;
	MDS_SUBSCRIPTION_RESULTS_INFO *lcl_subtn_res = NULL;

	/* Filling all the parameters */
	memset(&req, 0, sizeof(req));

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mcm_msg_direct_send_buff\n");

	req.msg.data.buff_info.buff = buff_info.buff;
	req.msg.data.buff_info.len = buff_info.len;
	req.msg.encoding = MDS_ENC_TYPE_DIRECT_BUFF;
	req.to = to;

	/* Get the destination sub res table entry and fill the send cnt */
	mds_get_subtn_res_tbl_by_adest(svc_cb->svc_hdl, to_svc_id,
					dest_vdest_id, adest, &lcl_subtn_res);
	req.svc_seq_num = lcl_subtn_res->msg_snd_cnt++;
	req.src_svc_id = svc_cb->svc_id;
	req.src_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.src_vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.src_adest = m_MDS_GET_ADEST;
	req.adest = adest;
	req.xch_id = xch_id;
	req.snd_type = snd_type;

	req.dest_svc_id = to_svc_id;
	req.dest_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.dest_vdest_id = dest_vdest_id;

	req.pri = pri;
	req.msg_fmt_ver = msg_fmt_ver;

	m_MDS_LOG_INFO("MDS_SND_RCV: Sending the data to MDTM layer\n");

	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_direct_send_buff\n");
	return mds_mdtm_send(&req);
}

/****************************************************************************
 *
 * Function Name: mcm_msg_encode_full_or_flat_and_send
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_msg_encode_full_or_flat_and_send(uint8_t to, SEND_MSG *to_msg,
						  MDS_SVC_ID to_svc_id, MDS_SVC_INFO *svc_cb,
						  MDS_DEST adest, MDS_VDEST_ID dest_vdest_id,
						  uint32_t snd_type, uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri)
{
	NCSMDS_CALLBACK_INFO cbinfo;
	uint32_t status;

	MDTM_SEND_REQ msg_send;
	MDS_BCAST_BUFF_LIST *bcast_ptr = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *lcl_subtn_res = NULL;

	memset(&msg_send, 0, sizeof(msg_send));

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mcm_msg_encode_full_or_flat_and_send\n");

	/* The following is for the bcast case, where once enc or enc_flat callback is called, those callbacks
	   shallnot be called again.  */
	if ((snd_type == MDS_SENDTYPE_BCAST) || (snd_type == MDS_SENDTYPE_RBCAST)) {
		if (to == DESTINATION_ON_NODE) {
			if (NCSCC_RC_SUCCESS ==
			    (mds_mcm_search_bcast_list
			     (to_msg, BCAST_ENC_FLAT, to_msg->rem_svc_sub_part_ver, to_msg->rem_svc_arch_word,
			      &bcast_ptr, 1))) {
				msg_send.msg.encoding = MDS_ENC_TYPE_FLAT;
				msg_send.msg.data.fullenc_uba.start = m_MMGR_DITTO_BUFR(bcast_ptr->bcast_enc_flat);
				msg_send.msg_fmt_ver = bcast_ptr->msg_fmt_ver;
				goto BY_PASS;
			}
		} else if (to == DESTINATION_OFF_NODE) {
			if (NCSCC_RC_SUCCESS ==
			    (mds_mcm_search_bcast_list
			     (to_msg, BCAST_ENC, to_msg->rem_svc_sub_part_ver, to_msg->rem_svc_arch_word, &bcast_ptr,
			      1))) {
				msg_send.msg.encoding = MDS_ENC_TYPE_FULL;
				msg_send.msg.data.fullenc_uba.start = m_MMGR_DITTO_BUFR(bcast_ptr->bcast_enc);
				msg_send.msg_fmt_ver = bcast_ptr->msg_fmt_ver;
				msg_send.msg_arch_word = bcast_ptr->rem_svc_arch_word;
				goto BY_PASS;
			}
		}
	}

	if (to == DESTINATION_OFF_NODE) {
		msg_send.msg.encoding = MDS_ENC_TYPE_FULL;
		if (ncs_enc_init_space_pp(&msg_send.msg.data.fullenc_uba, NCSUB_MDS_POOL, 0) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MDS_SND_RCV: encode full init failed svc_id=%d\n", svc_cb->svc_id);
			return NCSCC_RC_FAILURE;
		}
	} else {
		msg_send.msg.encoding = MDS_ENC_TYPE_FLAT;
		if (ncs_enc_init_space_pp(&msg_send.msg.data.flat_uba, NCSUB_MDS_POOL, 0) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MDS_SND_RCV: encode flat init failed svc_id=%d\n", svc_cb->svc_id);
			return NCSCC_RC_FAILURE;
		}
	}
	msg_send.adest = adest;

	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.i_yr_svc_hdl = svc_cb->yr_svc_hdl;
	cbinfo.i_yr_svc_id = svc_cb->svc_id;

	if (to == DESTINATION_ON_NODE) {
		/* Call the user layer callback to full encode msg */
		cbinfo.i_op = MDS_CALLBACK_ENC_FLAT;
		cbinfo.info.enc_flat.i_msg = to_msg->data.msg;
		cbinfo.info.enc_flat.i_to_svc_id = to_svc_id;
		cbinfo.info.enc_flat.io_uba = &msg_send.msg.data.flat_uba;
		/* sub part ver to be filled */
		cbinfo.info.enc_flat.i_rem_svc_pvt_ver = to_msg->rem_svc_sub_part_ver;
	} else if (to == DESTINATION_OFF_NODE) {
		/* Call the user layer callback to flat encode msg */
		cbinfo.i_op = MDS_CALLBACK_ENC;
		cbinfo.info.enc.i_msg = to_msg->data.msg;
		cbinfo.info.enc.i_to_svc_id = to_svc_id;
		cbinfo.info.enc.io_uba = &msg_send.msg.data.fullenc_uba;
		/* sub part ver to be filled */
		cbinfo.info.enc.i_rem_svc_pvt_ver = to_msg->rem_svc_sub_part_ver;
	}

	m_MDS_LOG_DBG("MDS_SND_RCV : calling cb ptr enc or enc flatin mcm_msg_encode_full_or_flat_and_send\n");

	status = svc_cb->cback_ptr(&cbinfo);

	if (status != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Encode callback of  Dest =%d, Adest=%llx, svc-id=%d failed while sending to svc=%d)",
		     dest_vdest_id, adest, svc_cb->svc_id, to_svc_id);
		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_encode_full_or_flat_and_send\n");
		if (msg_send.msg.encoding == MDS_ENC_TYPE_FLAT) {
			m_MMGR_FREE_BUFR_LIST(msg_send.msg.data.flat_uba.start);
		} else if (msg_send.msg.encoding == MDS_ENC_TYPE_FULL) {
			m_MMGR_FREE_BUFR_LIST(msg_send.msg.data.fullenc_uba.start);
		}
		return NCSCC_RC_FAILURE;
	}

	if (to == DESTINATION_OFF_NODE) {
		if (msg_send.msg.data.fullenc_uba.start == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Empty and NULL message from svc-id=%d to svc-id=%d", svc_cb->svc_id,
				      to_svc_id);
			m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_encode_full_or_flat_and_send\n");
			return NCSCC_RC_FAILURE;
		} else if ((snd_type == MDS_SENDTYPE_BCAST) || (snd_type == MDS_SENDTYPE_RBCAST)) {
			if (NCSCC_RC_FAILURE ==
			    mds_mcm_add_bcast_list(to_msg, BCAST_ENC, msg_send.msg.data.fullenc_uba.start,
						   to_msg->rem_svc_sub_part_ver, cbinfo.info.enc.o_msg_fmt_ver,
						   to_msg->rem_svc_arch_word)) {
				m_MDS_LOG_ERR("MDS_C_SNDRCV: Addition to bcast list failed in enc case");
				return NCSCC_RC_FAILURE;
			}
		}
	} else {
		if (msg_send.msg.data.flat_uba.start == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Empty and NULL message from svc-id=%d to svc-id=%d", svc_cb->svc_id,
				      to_svc_id);
			m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_encode_full_or_flat_and_send\n");
			return NCSCC_RC_FAILURE;
		} else if ((snd_type == MDS_SENDTYPE_BCAST) || (snd_type == MDS_SENDTYPE_RBCAST)) {
			if (NCSCC_RC_FAILURE ==
			    mds_mcm_add_bcast_list(to_msg, BCAST_ENC_FLAT, msg_send.msg.data.flat_uba.start,
						   to_msg->rem_svc_sub_part_ver, cbinfo.info.enc_flat.o_msg_fmt_ver,
						   to_msg->rem_svc_arch_word)) {
				m_MDS_LOG_ERR("MDS_C_SNDRCV: Addition to bcast list failed in enc_flat case");
				return NCSCC_RC_FAILURE;
			}
		}
	}

 BY_PASS:
	/* Add the remaining values */
	msg_send.pri = pri;
	msg_send.to = to;

	/* Get the destination sub res table entry and fill the send cnt */
	mds_get_subtn_res_tbl_by_adest(svc_cb->svc_hdl, to_svc_id,
			dest_vdest_id, adest, &lcl_subtn_res);
	msg_send.svc_seq_num = lcl_subtn_res->msg_snd_cnt++;
	msg_send.src_svc_id = svc_cb->svc_id;
	msg_send.src_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	msg_send.src_vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	msg_send.src_adest = m_MDS_GET_ADEST;
	msg_send.adest = adest;
	msg_send.xch_id = xch_id;
	msg_send.snd_type = snd_type;

	msg_send.dest_svc_id = to_svc_id;
	msg_send.dest_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	msg_send.dest_vdest_id = dest_vdest_id;
	msg_send.src_svc_sub_part_ver = svc_cb->svc_sub_part_ver;
	if (msg_send.msg.encoding == MDS_ENC_TYPE_FULL) {
		if (NULL == bcast_ptr) {
			msg_send.msg_fmt_ver = cbinfo.info.enc.o_msg_fmt_ver;	/* archword will be filled in next label */
			msg_send.msg_arch_word = to_msg->rem_svc_arch_word;
		}
	} else {
		if (NULL == bcast_ptr) {
			msg_send.msg_fmt_ver = cbinfo.info.enc_flat.o_msg_fmt_ver;
		}
	}
	m_MDS_LOG_INFO("MDS_SND_RCV: Sending the data to MDTM layer\n");
	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mcm_msg_encode_full_or_flat_and_send\n");
	return mds_mdtm_send(&msg_send);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_raise_selection_obj_for_ack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mds_mcm_raise_selection_obj_for_ack(MDS_SVC_INFO *svc_cb, MDS_DATA_RECV *recv)
{
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue;

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering mds_mcm_raise_selection_obj_for_ack\n");
	if (NCSCC_RC_SUCCESS != mcm_pvt_get_sync_send_entry(svc_cb, recv, &sync_queue)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: No entry in sync send table svc_id=%d, xch_id=%d\n", svc_cb->svc_id,
			      recv->exchange_id);
		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mds_mcm_raise_selection_obj_for_ack\n");
		return NCSCC_RC_FAILURE;
	}
	sync_queue->status = NCSCC_RC_SUCCESS;
	m_MDS_LOG_INFO("MDS_SND_RCV: Entry Found in sync send table svc_id=%d, xch_id=%d ,raising sel object\n",
		       svc_cb->svc_id, recv->exchange_id);
	m_NCS_SEL_OBJ_IND(sync_queue->sel_obj);
	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving mds_mcm_raise_selection_obj_for_ack\n");

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_get_sync_send_entry
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_pvt_get_sync_send_entry(MDS_SVC_INFO *svc_cb, MDS_DATA_RECV *recv,
					 MDS_MCM_SYNC_SEND_QUEUE **sync_queue)
{
	MDS_MCM_SYNC_SEND_QUEUE *queue = NULL;

	queue = svc_cb->sync_send_queue;

	m_MDS_LOG_INFO("MDS_SND_RCV: searching sync entry with xch_id=%d\n", recv->exchange_id);
	while (queue != NULL) {
		if (recv->exchange_id == queue->txn_id) {
			if (queue->msg_snd_type == MDS_SENDTYPE_SNDRSP) {
				if ((recv->snd_type == MDS_SENDTYPE_RSP) || (recv->snd_type == MDS_SENDTYPE_SNDRACK)) {
					*sync_queue = queue;
					m_MDS_LOG_INFO("MDS_SND_RCV: Found sync queue entry for svc_id=%d, xch_id=%d\n",
						       svc_cb->svc_id, recv->exchange_id);
					return NCSCC_RC_SUCCESS;
				}
			} else if ((queue->msg_snd_type == MDS_SENDTYPE_SNDRACK)
				   || (queue->msg_snd_type == MDS_SENDTYPE_REDRACK)) {
				if (recv->snd_type == MDS_SENDTYPE_ACK) {
					if (recv->src_adest == queue->dest_sndrack_adest.adest) {
						*sync_queue = queue;
						m_MDS_LOG_INFO
						    ("MDS_SND_RCV: Found sync queue entry for svc_id=%d, xch_id=%d\n",
						     svc_cb->svc_id, recv->exchange_id);
						return NCSCC_RC_SUCCESS;
					}
				}
			} else if (queue->msg_snd_type == MDS_SENDTYPE_REDRSP) {
				if ((recv->snd_type == MDS_SENDTYPE_RRSP) || (recv->snd_type == MDS_SENDTYPE_REDRACK)) {
					*sync_queue = queue;
					m_MDS_LOG_INFO("MDS_SND_RCV: Found sync queue entry for svc_id=%d, xch_id=%d\n",
						       svc_cb->svc_id, recv->exchange_id);
					return NCSCC_RC_SUCCESS;
				}
			} else if ((queue->msg_snd_type == MDS_SENDTYPE_SNDACK)
				   || (queue->msg_snd_type == MDS_SENDTYPE_REDACK)) {
				if (recv->snd_type == MDS_SENDTYPE_ACK) {
					*sync_queue = queue;
					m_MDS_LOG_INFO("MDS_SND_RCV: Found sync queue entry for svc_id=%d, xch_id=%d\n",
						       svc_cb->svc_id, recv->exchange_id);
					return NCSCC_RC_SUCCESS;
				}
			}
		}
		queue = queue->next_send;
	}
	*sync_queue = NULL;
	m_MDS_LOG_INFO("MDS_SND_RCV:No entry Found for sync queue svc_id=%d, xch_id=%d\n", svc_cb->svc_id,
		       recv->exchange_id);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_disc_queue_checks
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mds_mcm_process_disc_queue_checks(MDS_SVC_INFO *svc_cb, MDS_SVC_ID dest_svc_id,
					       MDS_DEST i_dest, MDS_SEND_INFO *req, MDS_DEST *o_dest,
					       bool *timer_running, MDS_SUBSCRIPTION_RESULTS_INFO **tx_send_hdl)
{
	MDS_VDEST_ID dest_vdest_id;
	MDS_SUBSCRIPTION_INFO *sub_info = NULL;
	V_DEST_QA anchor;
	uint32_t disc_rc;
	MDS_HDL env_hdl;
	bool time_wait = false;

	MDS_SUBSCRIPTION_RESULTS_INFO *t_send_hdl = NULL;	/* Subscription Result */

	m_MDS_LOG_DBG("MDS_SND_RCV :Entering mds_mcm_process_disc_queue_checks\n");

	env_hdl = (MDS_HDL)(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_cb->svc_hdl));

	dest_vdest_id = m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(i_dest);
	if (dest_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
		anchor = i_dest;	/* ADEST specified by "i_dest" to be looked up */
	} else {
		anchor = 0;	/* Active VDEST specified by "dest_vdest_id" to be looked up */
	}

	mds_subtn_tbl_get(svc_cb->svc_hdl, dest_svc_id, &sub_info);

	if (sub_info == NULL) {
		/* No subscription to this */
		/* Make a subscription to this service */
		m_MDS_LOG_INFO("MDS_SND_RCV: No subscription to service=%d, Making subscription\n", dest_svc_id);
		mds_mcm_subtn_add(svc_cb->svc_hdl, dest_svc_id, svc_cb->install_scope,
				  MDS_VIEW_RED /* redundantview */ , MDS_SUBTN_IMPLICIT);
		if (NCSCC_RC_SUCCESS != mds_subtn_tbl_get(svc_cb->svc_hdl, dest_svc_id, &sub_info)) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Subscription made but no pointer got after subscription\n");
			m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
			return NCSCC_RC_FAILURE;
		}
	} else if (sub_info->tmr_flag != true) {
		if ((MDS_SENDTYPE_RSP == req->i_sendtype) || (MDS_SENDTYPE_RRSP == req->i_sendtype)) {
			time_wait = true;
			m_MDS_LOG_INFO("MDS_SND_RCV:Disc queue: Subscr exists no timer running: Waiting for some time\n");
		} else {
			m_MDS_LOG_INFO("MDS_SND_RCV:Subscription exists but no timer running\n");
			m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
			return NCSCC_RC_FAILURE;
		}
	}

	/* Add this message to the DISC Queue, One function call */
	/*
	   Enqueue <sel-obj>, <send-type + destination-data>
	   Wait_for_ind_on_sel_obj_with_remaining_timeout
	 */
	m_MDS_LOG_INFO("MDS_SND_RCV:Blocking send from SVC id = %d to SVC id = %d",
		       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_cb->svc_hdl), dest_svc_id);

	disc_rc = mds_subtn_tbl_add_disc_queue(sub_info, req, dest_vdest_id, 	
							anchor, env_hdl, svc_cb->svc_id, time_wait);
	if (NCSCC_RC_SUCCESS != disc_rc) {
		/* Again we will come here when timeout or result has come */
		/* Check whether the Dest exists */
		/* After Subscription timeout also no route found */
		/* m_MDS_LOG_ERR("MDS_SND_RCV: No Route FOUND from SVC id = %d to SVC id = %d on "); */
		if (NCSCC_RC_REQ_TIMOUT == disc_rc) {
			/* We timed out waiting for a route */
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV:No Route Found from SVC id = %d to SVC id = %d on ADEST <0x%08x, %u>",
			     svc_cb->svc_id, dest_svc_id, m_MDS_GET_NODE_ID_FROM_ADEST(i_dest),
			     m_MDS_GET_PROCESS_ID_FROM_ADEST(i_dest));
		}

		m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
		return NCSCC_RC_FAILURE;
	} else {
		if (dest_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {

			/* Query one type to get the tx_send_hdl(results) */
			if (NCSCC_RC_SUCCESS !=
			    mds_subtn_res_tbl_query_by_adest(svc_cb->svc_hdl, dest_svc_id, dest_vdest_id, anchor)) {
				/* m_MDS_LOG_ERR("MDS_SND_RCV: No Route FOUND from SVC id = %d "); */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV:No Route Found from SVC id = %d to SVC id = %d on ADEST <0x%08x, %u>",
				     svc_cb->svc_id, dest_svc_id, m_MDS_GET_NODE_ID_FROM_ADEST(i_dest),
				     m_MDS_GET_PROCESS_ID_FROM_ADEST(i_dest));
				m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
				return NCSCC_RC_FAILURE;
			} else {
				/* Route available, send the data now */
				*o_dest = i_dest;
				m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
				return NCSCC_RC_SUCCESS;
			}
		} else {
			/* Destination svc is on vdest */
			/* Check dest_svc_id, dest_pwe_id, Destination <ADEST, VDEST>,
			   exists in subscription result table */
			bool flag_t = 0;
			if (svc_cb->parent_vdest_info->policy == NCS_VDEST_TYPE_MxN)
				flag_t = 1;
			if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get(svc_cb->svc_hdl, dest_svc_id, dest_vdest_id,
								      o_dest, timer_running, &t_send_hdl, flag_t)) {
				/* Destination Route still Not Found,  */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV: Destination Route not found even after the DISCOVERY Timer timeout\n");
				m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
				return NCSCC_RC_FAILURE;
			}
			*tx_send_hdl = t_send_hdl;
			m_MDS_LOG_DBG("MDS_SND_RCV :L  mds_mcm_process_disc_queue_checks\n");
			return NCSCC_RC_SUCCESS;
		}
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_subtn_tbl_add_disc_queue
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *                NCSCC_RC_OUT_OF_MEM
 *                NCSCC_RC_REQ_TIMOUT
 *
 * MDS-Lock-state   : ??
 * MDS-FLOW         : ??
 *
 * 
 * QUESTIONS: "sub_info" is a secure pointer? Doesn't it need MDS-lock? 
 *
 ***************************************************************************/
static uint32_t mds_subtn_tbl_add_disc_queue(MDS_SUBSCRIPTION_INFO *sub_info, MDS_SEND_INFO *req,
					  MDS_VDEST_ID dest_vdest_id, MDS_DEST dest, MDS_HDL env_hdl,
					  MDS_SVC_ID fr_svc_id, bool time_wait)
{
	NCS_SEL_OBJ sel_obj;
	MDS_AWAIT_DISC_QUEUE *add_ptr = NULL, *mov_ptr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, status = 0, timeout_val = 0;

	if (true == time_wait) {
		timeout_val = 150; /* This may need a tuning */
	}

	mov_ptr = sub_info->await_disc_queue;
	add_ptr = m_MMGR_ALLOC_DISC_QUEUE;

	if (add_ptr == NULL) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Memory allocation to DISC queue failed\n");
		return NCSCC_RC_OUT_OF_MEM;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	memset(add_ptr, 0, sizeof(MDS_AWAIT_DISC_QUEUE));
	add_ptr->send_type = req->i_sendtype;
	add_ptr->vdest = dest_vdest_id;
	add_ptr->adest = dest;

	status = m_NCS_SEL_OBJ_CREATE(&sel_obj);
	if (status != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_DISC_QUEUE(add_ptr);
		m_MDS_LOG_ERR("MDS_SND_RCV: Selection object creation failed (disc queue)");
		return NCSCC_RC_OUT_OF_MEM;
	}

	add_ptr->sel_obj = sel_obj;

	if (mov_ptr == NULL) {
		add_ptr->next_msg = NULL;
		sub_info->await_disc_queue = add_ptr;
	} else {
		/* adding in the start */
		m_MDS_LOG_INFO("MDS_SND_RCV: Adding in subscription table\n");
		add_ptr->next_msg = mov_ptr;
		sub_info->await_disc_queue = add_ptr;
	}

	/* Now wait till the timeout or an subscription result will come */

	mds_mutex_unlock();

	switch (req->i_sendtype) {
	case MDS_SENDTYPE_SND:
	case MDS_SENDTYPE_RSP:
	case MDS_SENDTYPE_RED:
	case MDS_SENDTYPE_RRSP:
	case MDS_SENDTYPE_BCAST:
	case MDS_SENDTYPE_RBCAST:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, timeout_val)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_SNDRSP:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrsp.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_SNDRACK:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrack.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_REDRSP:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrsp.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_REDRACK:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrack.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_SNDACK:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndack.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	case MDS_SENDTYPE_REDACK:
		{
			m_MDS_LOG_INFO("MDS_SND_RCV: Waiting for timeout\n");
			if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndack.i_time_to_wait)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: timeout or error occured\n");
				rc = NCSCC_RC_REQ_TIMOUT;
			}
		}
		break;

	default:
		m_MDS_LOG_ERR("MDS_SND_RCV: Internal error:File=%s,Line=%d\n", __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
		break;
	}

	mds_mutex_lock();

	if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, req->i_to_svc)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
		return NCSCC_RC_FAILURE;
	}

	/* Free the memory */
#if 1

	/* Success case should also delete the entry */
	mds_subtn_tbl_del_disc_queue(sub_info, add_ptr->sel_obj);
#else
	m_MMGR_FREE_DISC_QUEUE(add_ptr);
#endif

	return rc;
}

/****************************************************************************
 *
 * Function Name: mds_subtn_tbl_del_disc_queue
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mds_subtn_tbl_del_disc_queue(MDS_SUBSCRIPTION_INFO *sub_info, NCS_SEL_OBJ obj)
{
	MDS_AWAIT_DISC_QUEUE *del_ptr = NULL, *q_ptr = NULL;
	q_ptr = sub_info->await_disc_queue;

	if (q_ptr == NULL) {

		return NCSCC_RC_SUCCESS;
	}

	if (q_ptr->next_msg == NULL) {
		if (memcmp(&q_ptr->sel_obj, &obj, sizeof(NCS_SEL_OBJ)) == 0) {
			sub_info->await_disc_queue = NULL;
			m_NCS_SEL_OBJ_DESTROY(q_ptr->sel_obj);
			m_MMGR_FREE_DISC_QUEUE(q_ptr);
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully deleted from discovery queue\n");
			return NCSCC_RC_SUCCESS;
		} else {
			m_MDS_LOG_ERR("MDS_SND_RCV: No Entry in SUBscription queue\n");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (memcmp(&q_ptr->sel_obj, &obj, sizeof(NCS_SEL_OBJ)) == 0) {
			sub_info->await_disc_queue = q_ptr->next_msg;
			m_NCS_SEL_OBJ_DESTROY(q_ptr->sel_obj);
			m_MMGR_FREE_DISC_QUEUE(q_ptr);
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully deleted from discovery queue\n");
			return NCSCC_RC_SUCCESS;
		} else {
			while (q_ptr != NULL) {
				if (memcmp(&q_ptr->sel_obj, &obj, sizeof(NCS_SEL_OBJ)) == 0) {
					if (q_ptr->next_msg == NULL) {
						del_ptr->next_msg = NULL;
					} else {
						del_ptr->next_msg = q_ptr->next_msg;
					}
					m_NCS_SEL_OBJ_DESTROY(q_ptr->sel_obj);
					m_MMGR_FREE_DISC_QUEUE(q_ptr);
					m_MDS_LOG_INFO("MDS_SND_RCV: Successfully deleted from discovery queue\n");
					return NCSCC_RC_SUCCESS;
				}
				del_ptr = q_ptr;
				q_ptr = q_ptr->next_msg;
			}
		}
	}
	m_MDS_LOG_ERR("MDS_SND_RCV: No Entry in SUBscription queue\n");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mcm_query_for_node_dest
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/

static uint32_t mcm_query_for_node_dest(MDS_DEST adest, uint8_t *to)
{
	/* Check whether the destination is offnode or on node or on same process */
	{
		/* Route present to send the message */
		uint32_t dest_node_id = 0, src_node_id = 0;
		uint32_t dest_process_id = 0, src_process_id = 0;

		dest_process_id = m_MDS_GET_PROCESS_ID_FROM_ADEST(adest);
		dest_node_id = m_MDS_GET_NODE_ID_FROM_ADEST(adest);

		src_process_id = m_MDS_GET_PROCESS_ID_FROM_ADEST(m_MDS_GET_ADEST);
		src_node_id = m_MDS_GET_NODE_ID_FROM_ADEST(m_MDS_GET_ADEST);

		if (dest_node_id == src_node_id) {
			if (dest_process_id == src_process_id)
				*to = DESTINATION_SAME_PROCESS;
			else
				*to = DESTINATION_ON_NODE;
		} else if (dest_node_id != src_node_id) {
			*to = DESTINATION_OFF_NODE;
		}
		return NCSCC_RC_SUCCESS;
	}

}

/****************************************************************************
 *
 * Function Name: mcm_process_await_active
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_process_await_active(MDS_SVC_INFO *svc_cb, MDS_SUBSCRIPTION_RESULTS_INFO *send_hdl,
				      MDS_SVC_ID to_svc_id, SEND_MSG *to_msg,
				      MDS_VDEST_ID to_vdest, uint32_t snd_type, uint32_t xch_id, MDS_SEND_PRIORITY_TYPE pri)
{
	/* Await timer running enqueue

	   - if (not direct send)
	   o Do full encode
	   Enqueue <sel-obj>, <ready-to-go-msg> in
	   await-active-table
	   return Sucess; */

	MDTM_SEND_REQ req;

	NCSMDS_CALLBACK_INFO cbinfo;
	MDS_BCAST_BUFF_LIST *bcast_ptr = NULL;

	memset(&req, 0, sizeof(req));

	if (svc_cb->i_fail_no_active_sends) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Returning in noactive state as the option is not to buffer the msgs");
		if (to_msg->msg_type == MSG_DIRECT_BUFF) {
			m_MDS_FREE_DIRECT_BUFF(to_msg->data.info.buff);
		}
		return MDS_RC_MSG_NO_BUFFERING;
	}

	if (to_msg->msg_type == MSG_DIRECT_BUFF) {
		req.msg.data.buff_info.buff = to_msg->data.info.buff;
		req.msg.data.buff_info.len = to_msg->data.info.len;
		req.msg.encoding = MDS_ENC_TYPE_DIRECT_BUFF;
		req.msg_fmt_ver = to_msg->msg_fmt_ver;
	} else if (to_msg->msg_type == MSG_NCSCONTEXT) {

		if (((snd_type == MDS_SENDTYPE_BCAST) || (snd_type == MDS_SENDTYPE_RBCAST)) &&
		    ((NCSCC_RC_SUCCESS == (mds_mcm_search_bcast_list(to_msg, BCAST_ENC, to_msg->rem_svc_sub_part_ver,
								     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED, &bcast_ptr,
								     1))))) {
			if (ncs_enc_init_space_pp(&req.msg.data.fullenc_uba, NCSUB_MDS_POOL, 0) != NCSCC_RC_SUCCESS) {
				m_MDS_LOG_ERR("MDS_SND_RCV: encode full init failed svc_id=%d\n", svc_cb->svc_id);
				return NCSCC_RC_FAILURE;
			}
			req.msg.data.fullenc_uba.start = m_MMGR_DITTO_BUFR(bcast_ptr->bcast_enc);
			req.msg_fmt_ver = bcast_ptr->msg_fmt_ver;
			req.msg_arch_word = MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED;
		} else {
			uint32_t status = 0;
			memset(&cbinfo, 0, sizeof(cbinfo));
			cbinfo.i_yr_svc_hdl = svc_cb->yr_svc_hdl;
			cbinfo.i_yr_svc_id = svc_cb->svc_id;

			cbinfo.i_op = MDS_CALLBACK_ENC;
			cbinfo.info.enc.i_msg = to_msg->data.msg;
			cbinfo.info.enc.i_to_svc_id = to_svc_id;

			cbinfo.info.enc.io_uba = NULL;
			cbinfo.info.enc.io_uba = &req.msg.data.fullenc_uba;
			cbinfo.info.enc.i_rem_svc_pvt_ver =
			    send_hdl->info.active_vdest.active_route_info->last_active_svc_sub_part_ver;

			if (ncs_enc_init_space_pp(&req.msg.data.fullenc_uba, NCSUB_MDS_POOL, 0) != NCSCC_RC_SUCCESS) {
				m_MDS_LOG_ERR("MDS_SND_RCV: Encode init space failed for ub\n");
				return NCSCC_RC_FAILURE;
			}

			status = svc_cb->cback_ptr(&cbinfo);
			if (status != NCSCC_RC_SUCCESS) {
				m_MDS_LOG_ERR("MDS_SND_RCV: CALLBACK ENC failed for svc_id=%d\n", svc_cb->svc_id);
				return NCSCC_RC_FAILURE;
			}

			if (((snd_type == MDS_SENDTYPE_BCAST) || (snd_type == MDS_SENDTYPE_RBCAST))) {
				if (NCSCC_RC_FAILURE ==
				    mds_mcm_add_bcast_list(to_msg, BCAST_ENC, req.msg.data.fullenc_uba.start,
							   to_msg->rem_svc_sub_part_ver, cbinfo.info.enc.o_msg_fmt_ver,
							   MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED)) {
					m_MDS_LOG_ERR("MDS_C_SNDRCV: Addition to bcast list failed in enc case");
					return NCSCC_RC_FAILURE;
				}
			}
			req.msg.encoding = MDS_ENC_TYPE_FULL;
			req.msg_fmt_ver = cbinfo.info.enc.o_msg_fmt_ver;
			req.msg_arch_word = MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED;
		}
	}
	req.adest = 0;		/* Will be filled and send when active comes into picture */
	req.src_svc_id = svc_cb->svc_id;
	req.src_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_cb->svc_hdl);
	req.src_vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_cb->svc_hdl);;
	req.src_adest = m_MDS_GET_ADEST;
	req.snd_type = snd_type;
	req.xch_id = xch_id;
	req.svc_seq_num = ++svc_cb->seq_no; /* Updated again when actual send is attempted */
	req.dest_svc_id = to_svc_id;
	req.dest_pwe_id = req.src_pwe_id;
	req.dest_vdest_id = to_vdest;
	req.pri = pri;

	m_MDS_LOG_INFO("MDS_SND_RCV: Adding in await active tbl\n");

	return mds_await_active_tbl_add(send_hdl, req);

}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_snd
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
    STEP 1: From user-input get Dest-SVC-ID,
                    Destination < ADEST(Anchor Value), VDEST (Standy or Active) >, PWE-ID

    STEP 2: Do a route lookup (Ignore role)

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist)
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                * Subscription timer running (or just started) *
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            Delete the Enqueud Message in SUBCR_Q
            return NO_ROUTE
        }
        else
        {
            * reaching here implies route is available*
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
        return SUCESS;
 ****************************************************************************/

static uint32_t mcm_pvt_red_svc_snd(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				 NCSCONTEXT msg, MDS_DEST to_dest,
				 V_DEST_QA anchor, MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	uint32_t xch_id = 0, status = 0;
	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, anchor);

	if (status == NCSCC_RC_SUCCESS) {
		m_MDS_LOG_INFO("MDS_SND_RCV: RED send Message sent successfully from svc_id= %d, to svc_id=%d\n",
			       fr_svc_id, to_svc_id);
		return NCSCC_RC_SUCCESS;
	} else {
		m_MDS_LOG_ERR("MDS_SND_RCV: RED send Message sent Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		return status;
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_snd_process_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_snd_process_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    SEND_MSG to_msg, MDS_DEST to_dest,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, uint32_t xch_id, V_DEST_QA anchor)
{
	MDS_SVC_ID dest_svc_id, src_svc_id;
	MDS_VDEST_ID dest_vdest_id;
	MDS_DEST dest = 0;
	uint8_t to = 0;		/* Destination on same process, node or off node */
	NCSCONTEXT hdl;

	MDS_PWE_HDL pwe_hdl = (MDS_PWE_HDL)env_hdl;
	MDS_SVC_INFO *svc_cb = NULL;

	uint32_t status = 0;

	MDS_SUBSCRIPTION_RESULTS_INFO *subs_result_hdl = NULL;
	V_DEST_RL role_ret = 0;	/* Not used, only passed to get the subscription result ptr */

	if (to_msg.msg_type == MSG_NCSCONTEXT) {
		if (to_msg.data.msg == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Null message");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (to_msg.data.info.buff == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Null message");
			return NCSCC_RC_FAILURE;
		}
	}

	src_svc_id = fr_svc_id;
	dest_svc_id = to_svc_id;

	/* Get SVC_cb */
	if (NCSCC_RC_SUCCESS != (mds_svc_tbl_get(pwe_hdl, src_svc_id, &hdl))) {
		m_MDS_LOG_ERR("MDS_SND_RCV:SVC not present=%d\n", src_svc_id);
		if (to_msg.msg_type == MSG_DIRECT_BUFF) {
			return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
		}
		return NCSCC_RC_FAILURE;
	}

	svc_cb = (MDS_SVC_INFO *)hdl;

	dest_svc_id = to_svc_id;

	dest_vdest_id = m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest);

	if (dest_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
		dest = to_dest;
	} else
		dest = anchor;
	/* Check dest_svc_id, dest_pwe_id, Destination <ADEST, VDEST>,
	   exists in subscription result table */

	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_query_by_adest(svc_cb->svc_hdl, dest_svc_id, dest_vdest_id, dest)) {
		/* Destination Route Not Found, still some validations required */
		/* Check in subscriptions whether this exists */
		if (NCSCC_RC_SUCCESS !=
		    mds_mcm_process_disc_queue_checks_redundant(svc_cb, dest_svc_id, dest_vdest_id, dest, req)) {
			/* m_MDS_LOG_ERR("MDS_SND_RCV:No Route Found\n"); */
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV:No Route Found from SVC id = %d to SVC id = %d on ADEST <0x%08x, %u>",
			     fr_svc_id, to_svc_id, m_MDS_GET_NODE_ID_FROM_ADEST(to_dest),
			     m_MDS_GET_PROCESS_ID_FROM_ADEST(to_dest));
			if (to_msg.msg_type == MSG_DIRECT_BUFF) {
				return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
			}
			return NCSCC_RC_FAILURE;
		}
	}

	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get_by_adest(svc_cb->svc_hdl, to_svc_id, dest_vdest_id,
							       dest, &role_ret, &subs_result_hdl)) {
		/* Destination Route Not Found */
		subs_result_hdl = NULL;
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Query for Destination failed, This case cannot exist as this has been validated before Src_svc=%d, Dest_scv=%d, vdest=%d, ADEST=%llx",
		     svc_cb->svc_id, to_svc_id, dest_vdest_id, dest);
		return NCSCC_RC_FAILURE;
	}
	mcm_query_for_node_dest_on_archword(dest, &to, subs_result_hdl->rem_svc_arch_word);

	status = mds_mcm_send_msg_enc(to, svc_cb, &to_msg, to_svc_id, dest_vdest_id, req, xch_id, dest, pri);

	return status;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_disc_queue_checks_redundant
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ****************************************************************************/
static uint32_t mds_mcm_process_disc_queue_checks_redundant(MDS_SVC_INFO *svc_cb, MDS_SVC_ID dest_svc_id,
							 MDS_VDEST_ID dest_vdest_id, V_DEST_QA anchor,
							 MDS_SEND_INFO *req)
{

	MDS_SUBSCRIPTION_INFO *sub_info = NULL;
	uint32_t disc_rc;
	bool time_wait = false;

	MDS_HDL env_hdl;

	env_hdl = (MDS_HDL)(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_cb->svc_hdl));
	mds_subtn_tbl_get(svc_cb->svc_hdl, dest_svc_id, &sub_info);

	if (sub_info == NULL) {
		/* No subscription to this */
		/* Make a subscription to this service */
		m_MDS_LOG_INFO("MDS_SND_RCV: No subscription to service=%d, Making subscription\n", dest_svc_id);
		mds_mcm_subtn_add(svc_cb->svc_hdl, dest_svc_id, svc_cb->install_scope,
				  MDS_VIEW_RED /* redundantview */ , MDS_SUBTN_IMPLICIT);

		if (NCSCC_RC_SUCCESS != mds_subtn_tbl_get(svc_cb->svc_hdl, dest_svc_id, &sub_info)) {
			m_MDS_LOG_INFO("MDS_SND_RCV: Subscription made but no pointer available\n");
			return NCSCC_RC_FAILURE;
		}
	} else if (sub_info->tmr_flag != true) {
		if ((MDS_SENDTYPE_RSP == req->i_sendtype) || (MDS_SENDTYPE_RRSP == req->i_sendtype)) {
			time_wait = true;
			m_MDS_LOG_INFO("MDS_SND_RCV:Disc queue red: Subscr exists no timer running: Waiting for some time\n");
		} else {
			m_MDS_LOG_INFO("MDS_SND_RCV: Subscription exists but Timer has expired\n");
			return NCSCC_RC_FAILURE;
		}
	}

	/* Add this message to the DISC Queue, One function call */
	/*
	   Enqueue <sel-obj>, <send-type + destination-data>
	   Wait_for_ind_on_sel_obj_with_remaining_timeout
	 */
	disc_rc = mds_subtn_tbl_add_disc_queue(sub_info, req, dest_vdest_id, 
						anchor, env_hdl, svc_cb->svc_id, time_wait);
	if (NCSCC_RC_SUCCESS != disc_rc) {
		/* Again we will come here when timeout, out-of-mem or result has come */
		/* Check whether the Dest exists */
		/* After Subscription timeout also no route found */
		/* m_MDS_LOG_ERR("MDS_SND_RCV: No Route FOUND \n"); */
		if (NCSCC_RC_REQ_TIMOUT == disc_rc) {
			/* We timed out waiting for a route */
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV:No Route Found from SVC id = %d to SVC id = %d to VDEST id = %d on  ADEST <0x%08x, %u>",
			     svc_cb->svc_id, dest_svc_id, dest_vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(anchor),
			     m_MDS_GET_PROCESS_ID_FROM_ADEST(anchor));
		}
		return NCSCC_RC_FAILURE;
	} else {
		if (NCSCC_RC_SUCCESS != (mds_subtn_res_tbl_query_by_adest(svc_cb->svc_hdl, dest_svc_id,
									  dest_vdest_id, anchor))) {
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Destination Route not found even after the DISCOVERY Timer timeout\n");
			return NCSCC_RC_FAILURE;
		}

		return NCSCC_RC_SUCCESS;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndrsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
    STEP 1: From user-input get Dest-SVC-ID, Destination <ADEST, VDEST>, PWE-ID

    STEP 2: Do a route lookup and match for active role

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist)
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                * Subscription timer running (or just started) *
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            if (no matching entry exists in no-actives table ||
                       Destination Role=standy &&NO_ACTIVE_TMR Expired)
                return route-not-found
            - if (not direct send)
                o Do full encode
            - Enqueue <sel-obj>, <ready-to-go-msg> in
                await-active-table
        }
        else
        {
            * reaching here implies route is available*
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
        * Now wait for response of (put exch-id in DB also)*
        - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
        - Wait_for_ind_on_sel_obj_with_remaining_timeout
        - If (status = no-active-timeout)
            return timeout;
          else if (select-timeout)
            clean-up-await-active-entry and return timeout;
          else
            return SUCCESS (with response data)
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndrsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				       NCSCONTEXT msg, MDS_DEST to_dest,
				       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	MDS_SYNC_TXN_ID xch_id = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	status = mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);

	if (NCSCC_RC_SUCCESS != status) {
		/* delete the created the sync send entry */
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Normal sndrsp mesg SEND Failed from svc_id= %d, to svc_id=%d to_dest=%llx\n",
		     fr_svc_id, to_svc_id, to_dest);
		return status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrsp.i_time_to_wait)) {
			mds_mutex_lock();
			/* This is for response for local dest */
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				req->info.sndrsp.o_rsp = sync_queue->sent_msg;
				req->info.sndrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
				return NCSCC_RC_SUCCESS;
			}

			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on sndrsp message\n");
			m_MDS_ERR_PRINT_ADEST(to_dest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);

			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();

			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_INFO("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
			req->info.sndrsp.o_rsp = sync_queue->sent_msg;
			req->info.sndrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mds_await_active_tbl_del_entry
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mds_await_active_tbl_del_entry(MDS_PWE_HDL env_hdl, MDS_SVC_ID fr_svc_id, uint32_t xch_id,
					    MDS_SVC_ID to_svc_id, MDS_VDEST_ID vdest, MDS_SENDTYPES snd_type)
{

	MDS_SVC_INFO *svc_cb = NULL;
	NCSCONTEXT hdl;
	MDS_DEST dest = 0;
	bool timer_running = 0;
	MDS_SUBSCRIPTION_RESULTS_INFO *result = NULL;
	bool flag_t = 0;

	/* Get SVC_cb */
	if (NCSCC_RC_SUCCESS != (mds_svc_tbl_get(env_hdl, fr_svc_id, &hdl))) {
		m_MDS_LOG_ERR("MDS_SND_RCV: SVC not present=%d\n", fr_svc_id);
		return NCSCC_RC_FAILURE;
	}

	svc_cb = (MDS_SVC_INFO *)hdl;

	if (svc_cb->parent_vdest_info->policy == NCS_VDEST_TYPE_MxN)
		flag_t = 1;

	mds_subtn_res_tbl_get(svc_cb->svc_hdl, to_svc_id, vdest, &dest, &timer_running, &result, flag_t);

	if (timer_running) {
		/* Delete the entry */
		MDS_AWAIT_ACTIVE_QUEUE *mov_ptr = NULL, *bk_ptr = NULL;
		mov_ptr = result->info.active_vdest.active_route_info->await_active_queue;
		if (mov_ptr == NULL) {
			m_MDS_LOG_INFO("MDS_SND_RCV: Await active entry doesnt exists\n");
			return NCSCC_RC_SUCCESS;
		} else {
			while (mov_ptr != NULL) {
				if ((mov_ptr->req.xch_id == xch_id) && (mov_ptr->req.snd_type == snd_type) &&
				    (mov_ptr->req.dest_svc_id == to_svc_id)) {
					if (bk_ptr == NULL) {
						result->info.active_vdest.active_route_info->await_active_queue =
						    mov_ptr->next_msg;
					} else {
						bk_ptr->next_msg = mov_ptr->next_msg;
					}
					m_MDS_LOG_INFO("MDS_SND_RCV: Await active entry successfully deleted\n");
					m_MMGR_FREE_AWAIT_ACTIVE(mov_ptr);
					return NCSCC_RC_SUCCESS;
				}
				bk_ptr = mov_ptr;
				mov_ptr = mov_ptr->next_msg;
			}
		}
	}
	m_MDS_LOG_INFO("MDS_SND_RCV: Await active entry doesnt exists\n");

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_time_wait
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/

static uint32_t mds_mcm_time_wait(NCS_SEL_OBJ sel_obj, uint32_t time_val)
{
	/* Now wait for the response to come */
	uint32_t count = 0;

	if (time_val == 0) {
		count = m_NCS_SEL_OBJ_POLL_SINGLE_OBJ(sel_obj, NULL);
	} else
		count = m_NCS_SEL_OBJ_POLL_SINGLE_OBJ(sel_obj, &time_val);

	if ((count == 0) || (count == -1)) {
		/* Both for Timeout and Error Case */
		m_MDS_LOG_ERR("MDS_SND_RCV: Timeout or Error occured\n");
		return NCSCC_RC_FAILURE;
	} else if (count == 1) {	/* Success case */
		m_NCS_SEL_OBJ_RMV_IND(sel_obj, true, true);
		return NCSCC_RC_SUCCESS;
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_del_sync_send_entry
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_pvt_del_sync_send_entry(MDS_PWE_HDL env_hdl, MDS_SVC_ID fr_svc_id, uint32_t xch_id,
					 MDS_SENDTYPES snd_type, MDS_DEST adest)
{
	NCSCONTEXT hdl;
	MDS_SVC_INFO *svc_cb;
	MDS_MCM_SYNC_SEND_QUEUE *q_ptr, *prev_ptr;

	m_MDS_LOG_INFO("MDS_SND_RCV: Deleting the sync send entry with xch_id=%d\n", xch_id);
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(env_hdl, fr_svc_id, &hdl)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Service Doesnt exists svc_id=%d\n", fr_svc_id);
		return NCSCC_RC_FAILURE;
	}
	svc_cb = (MDS_SVC_INFO *)hdl;

	/* CARE : The for loop below contains "continue" statements */
	for (prev_ptr = NULL, q_ptr = svc_cb->sync_send_queue; q_ptr != NULL; prev_ptr = q_ptr, q_ptr = q_ptr->next_send) {	/* Safe because we quit after deletion */
		if ((q_ptr->txn_id != xch_id) || (q_ptr->msg_snd_type != snd_type))
			continue;

		if ((q_ptr->msg_snd_type == MDS_SENDTYPE_SNDRACK) || (q_ptr->msg_snd_type == MDS_SENDTYPE_REDRACK)) {
			if (q_ptr->dest_sndrack_adest.adest != adest) {
				continue;
			}
		}

      /**** Reaching here implies found a match ****/

		/* Detach by changing parent pointer to point to next */
		if (prev_ptr == NULL) {
			svc_cb->sync_send_queue = q_ptr->next_send;
		} else {
			prev_ptr->next_send = q_ptr->next_send;
		}

		svc_cb->sync_count--;
		m_MDS_LOG_INFO("MDS_SND_RCV: Successfully Deleted the sync send entry with xch_id=%d, fr_svc_id=%d\n",
			       xch_id, fr_svc_id);
		m_NCS_SEL_OBJ_DESTROY(q_ptr->sel_obj);
		m_MMGR_FREE_SYNC_SEND_QUEUE(q_ptr);
		q_ptr = NULL;
		return NCSCC_RC_SUCCESS;
	}
	m_MDS_LOG_ERR("MDS_SND_RCV: No Entry in Sync Send queue xch_id=%d, fr_svc_id=%d\n", xch_id, fr_svc_id);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_create_sync_send_entry
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *                NCSCC_RC_OUT_OF_MEM
 ***************************************************************************/
static uint32_t mcm_pvt_create_sync_send_entry(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_SENDTYPES snd, uint32_t xch_id,
					    MDS_MCM_SYNC_SEND_QUEUE **sync_queue, NCSCONTEXT sent_msg)
{
	uint32_t status = 0;
	NCS_SEL_OBJ sel_obj;
	MDS_SVC_INFO *svc_cb = NULL;
	NCSCONTEXT hdl;
	MDS_MCM_SYNC_SEND_QUEUE *mov_ptr = NULL;

	/* Validate PWE-Handle first:  */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get((MDS_PWE_HDL)env_hdl, fr_svc_id, &hdl)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: SVC doesnt exists\n", fr_svc_id);
		return NCSCC_RC_FAILURE;
	}

	*sync_queue = m_MMGR_ALLOC_SYNC_SEND_QUEUE;

	if (*sync_queue == NULL) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Memory allocation to sync send queue failed\n");
		return NCSCC_RC_FAILURE;
	}
	memset(*sync_queue, 0, sizeof(MDS_MCM_SYNC_SEND_QUEUE));

	m_MDS_LOG_INFO("MDS_SND_RCV: creating sync entry with xch_id=%d\n", xch_id);
	svc_cb = (MDS_SVC_INFO *)hdl;

	if (svc_cb->sync_send_queue == NULL)
		svc_cb->sync_count = 0;

	memset(&sel_obj, 0, sizeof(sel_obj));
	status = m_NCS_SEL_OBJ_CREATE(&sel_obj);
	if (status != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Selection object creation failed (for sync-send entry)");
		m_MMGR_FREE_SYNC_SEND_QUEUE(*sync_queue);
		*sync_queue = NULL;
		return NCSCC_RC_OUT_OF_MEM;
	}
	(*sync_queue)->sel_obj = sel_obj;
	(*sync_queue)->txn_id = xch_id;
	(*sync_queue)->status = MDS_MSG_SENT;
	(*sync_queue)->msg_snd_type = snd;
	(*sync_queue)->orig_msg = sent_msg;

	mov_ptr = svc_cb->sync_send_queue;

	svc_cb->sync_count++;
	(*sync_queue)->next_send = mov_ptr;

	svc_cb->sync_send_queue = (*sync_queue);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndrack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndrack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					NCSCONTEXT msg, MDS_DEST to_dest,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	/*
	   STEP 1: From "resp_async_hdl" get Destination-SVC-ID,
	   ((Destination-VDEST + Anchor)/ADEST)

	   STEP 2: Do a route lookup (Ignore role)

	   STEP 3: route_avail_bool = active-route-available.

	   STEP 4: if (!route_avail_bool)
	   {
	   if ((subscription does not exist) ||
	   (subscription exists but timer is running))
	   {
	   if ((subscription does not exist)
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer
	   }
	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout

	   - route_avail_bool = active-route-available.
	   }
	   }
	   if (!route_avail_bool)
	   {
	   Delete the Enqueud <sel-obj>, <send-type + destination-data> in SYNC_SEND_TBL
	   return NO_ROUTE
	   }
	   else
	   {
	   * reaching here implies route is available*
	   - if (not direct send)
	   Encode/copy/blah
	   - Send using route(buff or USRBUF)
	   - if (direct send)
	   Free buff
	   }
	   * Now wait for ack of (put exch-ack-id in DB also)*
	   - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout
	   - If (status = timeout)
	   return timeout;
	   else if (ACK received)
	   clean-up SYNC_SEND_TBL-entry
	   return SUCESS;
	 */
	uint8_t *data;
	uint32_t xch_id = 0;
	uint64_t msg_dest_adest = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue;
	uint8_t len_sync_ctxt = 0;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	data = req->info.sndrack.i_msg_ctxt.data;
	len_sync_ctxt = req->info.sndrack.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return NCSCC_RC_FAILURE;
	}

	/* Getting the original ones */
	xch_id = ncs_decode_32bit(&data);
	msg_dest_adest = ncs_decode_64bit(&data);

	/* Now create a sync_send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	sync_queue->dest_sndrack_adest.adest = msg_dest_adest;

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	status = mcm_pvt_process_sndrack_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, 0);

	if (NCSCC_RC_SUCCESS != status) {
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, msg_dest_adest);
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal sndrack mesg SEND Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(msg_dest_adest);
		return status;
	} else {
		mds_mutex_unlock();

		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* for local case */
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    msg_dest_adest);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on sndrack message\n");
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(msg_dest_adest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();

			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_process_sndrack_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 ***************************************************************************/
static uint32_t mcm_pvt_process_sndrack_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    SEND_MSG to_msg, MDS_DEST to_dest,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, V_DEST_QA anchor)
{
	uint32_t xch_id = 0, status = 0;
	MDS_DEST dest = 0;
	uint8_t *data = NULL;
	uint8_t to;
	MDS_VDEST_ID dest_vdest_id = 0;

	NCSCONTEXT hdl;

	MDS_SVC_INFO *svc_cb = NULL;

	MDS_SUBSCRIPTION_RESULTS_INFO *subs_result_hdl = NULL;
	V_DEST_RL role_ret = 0;	/* Used only to get the subscription result ptr */

	if (to_msg.msg_type == MSG_NCSCONTEXT) {
		if (to_msg.data.msg == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Null message\n");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (to_msg.data.info.buff == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Null message\n");
			return NCSCC_RC_FAILURE;
		}
	}

	if (req->i_sendtype == MDS_SENDTYPE_SNDRACK)
		data = req->info.sndrack.i_msg_ctxt.data;
	else if (req->i_sendtype == MDS_SENDTYPE_REDRACK)
		data = req->info.redrack.i_msg_ctxt.data;

	/* Getting the original things */
	xch_id = ncs_decode_32bit(&data);
	dest = ncs_decode_64bit(&data);
	dest_vdest_id = m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest);

	/* Get SVC_cb */
	if (NCSCC_RC_SUCCESS != (mds_svc_tbl_get((MDS_PWE_HDL)env_hdl, fr_svc_id, &hdl))) {
		m_MDS_LOG_ERR("MDS_SND_RCV: svc not present=%d\n", fr_svc_id);
		if (to_msg.msg_type == MSG_DIRECT_BUFF) {
			return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
		}
		return NCSCC_RC_FAILURE;
	}

	svc_cb = (MDS_SVC_INFO *)hdl;

	/* Check dest_svc_id, dest_pwe_id, Destination <ADEST, VDEST>,
	   exists in subscription result table */
	if (req->i_sendtype == MDS_SENDTYPE_SNDRACK) {
		if (dest_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
			dest = to_dest;
		} else
			dest = dest;
	} else
		dest = anchor;

	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_query_by_adest(svc_cb->svc_hdl, to_svc_id, dest_vdest_id, dest)) {
		/* Check in subscriptions whether this exists */
		if (NCSCC_RC_SUCCESS !=
		    mds_mcm_process_disc_queue_checks_redundant(svc_cb, to_svc_id, dest_vdest_id, dest, req)) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Nwo_desto Route Found from svc_id=%d, to svc_id= %d",
				      svc_cb->svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			if (to_msg.msg_type == MSG_DIRECT_BUFF) {
				return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
			}
			return NCSCC_RC_FAILURE;
		}
		/* Destination Route Not Found, still some validations required */
	}

	if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_get_by_adest(svc_cb->svc_hdl, to_svc_id, dest_vdest_id,
							       dest, &role_ret, &subs_result_hdl)) {
		/* Destination Route Not Found */
		subs_result_hdl = NULL;
		m_MDS_LOG_ERR
		    ("MDS_SND_RCV: Query for Destination failed, This case cannot exist as this has been validated before Src_svc=%d, Dest_scv=%d, vdest=%d, ADEST=%llx",
		     svc_cb->svc_id, to_svc_id, dest_vdest_id, dest);
		return NCSCC_RC_FAILURE;
	}
	mcm_query_for_node_dest_on_archword(dest, &to, subs_result_hdl->rem_svc_arch_word);

	status = mds_mcm_send_msg_enc(to, svc_cb, &to_msg, to_svc_id, dest_vdest_id, req, xch_id, dest, pri);

	return status;

}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				       NCSCONTEXT msg, MDS_DEST to_dest,
				       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	/*

	   Trivial validations already done?
	   STEP 1: From user-input get Dest-SVC-ID, Destination <ADEST, VDEST>, PWE-ID

	   STEP 2: Do a route lookup and match for active role

	   STEP 3: route_avail_bool = active-route-available.

	   STEP 4: if (!route_avail_bool)
	   {
	   if ((subscription does not exist) ||
	   (subscription exists but timer is running))
	   {
	   if ((subscription does not exist)
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer
	   }
	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout

	   - route_avail_bool = active-route-available.
	   }
	   }
	   if (!route_avail_bool)
	   {
	   if (no matching entry exists in no-actives table ||
	   Destination Role=standy &&NO_ACTIVE_TMR Expired)
	   return route-not-found
	   - if (not direct send)
	   o Do full encode
	   - Enqueue <sel-obj>, <ready-to-go-msg> in
	   await-active-table
	   }
	   else
	   {
	   * reaching here implies route is available*
	   - if (not direct send)
	   Encode/copy/blah
	   - Send using route(buff or USRBUF)
	   - if (direct send)
	   Free buff
	   }
	   * Now wait for ack of (put exch-id in DB also)*
	   - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout
	   - If (status = no-active-timeout)
	   return timeout;
	   else if (select-timeout)
	   clean-up-await-active-entry and return timeout;
	   else
	   return SUCCESS
	 */

	MDS_SYNC_TXN_ID xch_id = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	status = mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);

	if (NCSCC_RC_SUCCESS != status) {
		/* delete the created the sync send entry */
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal sndack message SEND Failed from svc_id= %d, to svc_id=%d\n",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		return status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on sndack message from svc_id=%d, to svc_id=%d",
				      fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_snd_rsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
    STEP 1: From "resp_async_hdl" get Destination-SVC-ID,
        ((Destination-VDEST + Anchor)/ADEST)

    STEP 2: Do a route lookup (Ignore role)

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist)
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                * Subscription timer running (or just started) *
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            Delete the Enqueud Message in SUBCR_Q
            return NO_ROUTE
        }
        else
        {
            * reaching here implies route is available*
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
        return SUCESS;
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_snd_rsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					NCSCONTEXT msg, MDS_DEST to_dest,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	uint32_t xch_id, status = 0;
	MDS_DEST adest = 0;
	uint8_t *data;
	uint8_t len_sync_ctxt = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	data = req->info.rsp.i_msg_ctxt.data;
	len_sync_ctxt = req->info.rsp.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return NCSCC_RC_FAILURE;
	}

	xch_id = ncs_decode_32bit(&data);
	adest = ncs_decode_64bit(&data);

	status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, adest);

	if (status == NCSCC_RC_SUCCESS) {
		m_MDS_LOG_INFO("MDS_SND_RCV: Normal rsp Message sent successfully from svc_id= %d, to svc_id=%d\n",
			       fr_svc_id, to_svc_id);
		return NCSCC_RC_SUCCESS;
	} else {
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal rsp Message sent Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(adest);
		return status;
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndrsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
    STEP 1: From user-input get Dest-SVC-ID,
                    Destination < ADEST(Anchor Value), VDEST (Standy or Active) >, PWE-ID

    STEP 2: Do a route lookup (Ignore role)

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist))
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                * Subscription timer running (or just started) *
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            Delete the Enqueud <sel-obj>, <send-type + destination-data> in SYNC_SEND_TBL
            return NO_ROUTE
        }
        else
        {
            * reaching here implies route is available*
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
       * Now wait for response of (put exch-id in DB also)*
        - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
        - Wait_for_ind_on_sel_obj_with_remaining_timeout
        - if (select-timeout)
            clean-up SYNC_SEND_TBL-entry and return timeout;
          else
            return SUCCESS (with response data)
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndrsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    V_DEST_QA anchor,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	MDS_SYNC_TXN_ID xch_id;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, anchor);

	if (NCSCC_RC_SUCCESS != status) {
		/* delete the created the sync send entry */
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndrsp message SEND Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		return status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrsp.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				req->info.redrsp.o_rsp = sync_queue->sent_msg;
				req->info.redrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on red sndrsp message from svc_id=%d, to svc_id=%d",
				      fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);

			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			req->info.redrsp.o_rsp = sync_queue->sent_msg;
			req->info.redrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndrack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndrack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				     NCSCONTEXT msg, MDS_DEST to_dest,
				     V_DEST_QA anchor,
				     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	/* WRITTEN IN MCM_PVT_NORMAL_SVC_SNDRACK

	   STEP 1: From "resp_async_hdl" get Destination-SVC-ID,
	   ((Destination-VDEST + Anchor)/ADEST),PWE-ID

	   STEP 2: Do a route lookup (Ignore role)

	   STEP 3: route_avail_bool = active-route-available.

	   STEP 4: if (!route_avail_bool)
	   {
	   if ((subscription does not exist) ||
	   (subscription exists but timer is running))
	   {
	   if ((subscription does not exist)
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer
	   }
	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout

	   - route_avail_bool = active-route-available.
	   }
	   }
	   if (!route_avail_bool)
	   {
	   Delete the Enqueud <sel-obj>, <send-type + destination-data> in SYNC_SEND_TBL
	   return NO_ROUTE
	   }
	   else
	   {
	   * reaching here implies route is available*
	   - if (not direct send)
	   Encode/copy/blah
	   - Send using route(buff or USRBUF)
	   - if (direct send)
	   Free buff
	   }
	   * Now wait for ack of (put exch-ack-id in DB also)*
	   - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout
	   - If (status = timeout)
	   return timeout;
	   else if (ACK received)
	   clean-up SYNC_SEND_TBL-entry
	   return SUCESS;
	 */
	uint8_t *data;
	uint32_t xch_id = 0;
	uint64_t msg_dest_adest = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue;
	uint8_t len_sync_ctxt = 0;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	data = req->info.redrack.i_msg_ctxt.data;
	len_sync_ctxt = req->info.redrack.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return NCSCC_RC_FAILURE;
	}

	/* Getting the original ones */
	xch_id = ncs_decode_32bit(&data);
	msg_dest_adest = ncs_decode_64bit(&data);

	/* Now create a sync_send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	sync_queue->dest_sndrack_adest.adest = msg_dest_adest;

	status = mcm_pvt_process_sndrack_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, anchor);

	if (NCSCC_RC_SUCCESS != status) {
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, msg_dest_adest);
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndrack message SEND Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		return status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    msg_dest_adest);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on red sndrack message\n");
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndack(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				    NCSCONTEXT msg, MDS_DEST to_dest,
				    V_DEST_QA anchor,
				    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	/* WRITTEN IN  mcm_pvt_red_svc_sndrsp

	   Trivial validations already done?
	   STEP 1: From user-input get Dest-SVC-ID, Destination <Anchor/ADEST, VDEST>, PWE-ID

	   STEP 2: Do a route lookup (Ignore Role)

	   STEP 3: route_avail_bool = active-route-available.

	   STEP 4: if (!route_avail_bool)
	   {
	   if ((subscription does not exist) ||
	   (subscription exists but timer is running))
	   {
	   if ((subscription does not exist)
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer
	   }
	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout

	   - route_avail_bool = active-route-available.
	   }
	   }
	   if (!route_avail_bool)
	   {
	   Delete the Enqueud <sel-obj>, <send-type + destination-data> in SYNC_SEND_TBL
	   return NO_ROUTE
	   }
	   else
	   {
	   * reaching here implies route is available*
	   - if (not direct send)
	   Encode/copy/blah
	   - Send using route(buff or USRBUF)
	   - if (direct send)
	   Free buff
	   }
	   * Now wait for ack of (put exch-id in DB also)*
	   - Enqueue <sel-obj>, <send-type + dest-data> in sync-send-tbl
	   - Wait_for_ind_on_sel_obj_with_remaining_timeout
	   if (select-timeout)
	   clean-up-await-active-entry and return timeout;
	   else
	   return SUCCESS
	 */

	MDS_SYNC_TXN_ID xch_id;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	NCS_SEL_OBJ sel_obj;

	uint32_t status = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	xch_id = ++mds_mcm_global_exchange_id;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, msg)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send enrty creation failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, anchor);

	if (NCSCC_RC_SUCCESS != status) {
		/* delete the created the sync send entry */
		printf("RED sndack message SEND Failed\n");
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndack message SEND Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		return status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR("MDS_SND_RCV: Timeout occured on red sndack message\n");
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_snd_rsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
    STEP 1: From "resp_async_hdl" get Destination-SVC-ID,
        ((Destination-VDEST + Anchor)/ADEST), PWE-ID

    STEP 2: Do a route lookup (Ignore role)

    STEP 3: route_avail_bool = active-route-available.

    STEP 4: if (!route_avail_bool)
        {
            if ((subscription does not exist) ||
                (subscription exists but timer is running))
            {
                if ((subscription does not exist)
                {
                - Create an implicit subscription
                    (scope = install-scope of sender, view = redundant)
                - Start a subscription timer
                }
                * Subscription timer running (or just started) *
                - Enqueue <sel-obj>, <send-type + destination-data>
                - Wait_for_ind_on_sel_obj_with_remaining_timeout

                - route_avail_bool = active-route-available.
            }
        }
        if (!route_avail_bool)
        {
            Delete the Enqueud Message in SUBCR_Q
            return NO_ROUTE
        }
        else
        {
            * reaching here implies route is available*
            - if (not direct send)
                Encode/copy/blah
            - Send using route(buff or USRBUF)
            - if (direct send)
            Free buff
        }
        return SUCESS;
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_snd_rsp(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				     NCSCONTEXT msg, MDS_DEST to_dest,
				     V_DEST_QA anchor,
				     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req, MDS_SEND_PRIORITY_TYPE pri)
{
	uint32_t xch_id, status = 0;
	MDS_DEST anchor_1 = 0;
	uint8_t *data;
	uint8_t len_sync_ctxt = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.msg_type = MSG_NCSCONTEXT;
	send_msg.data.msg = msg;

	data = req->info.rrsp.i_msg_ctxt.data;
	len_sync_ctxt = req->info.rrsp.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return NCSCC_RC_FAILURE;
	}
	xch_id = ncs_decode_32bit(&data);
	anchor_1 = ncs_decode_64bit(&data);

	status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id,
					   anchor_1);

	if (status == NCSCC_RC_SUCCESS) {
		m_MDS_LOG_INFO("MDS_SND_RCV: RED rsp Message sent successfully from svc_id= %d, to svc_id=%d\n",
			       fr_svc_id, to_svc_id);
		return NCSCC_RC_SUCCESS;
	} else {
		m_MDS_LOG_ERR("MDS_SND_RCV: RED rsp Message sent Failed from svc_id= %d, to svc_id=%d\n", fr_svc_id,
			      to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		return status;
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_bcast
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_bcast(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				      NCSCONTEXT msg,
				      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
				      NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri)
{
	/* Trivial validations already done?
	   STEP 1: From user-input get Dest-SVC-ID,PWE-ID
	   STEP 2: Check whether an entry for <Dest-SVC-ID,PWE-ID>
	   exists in the Global Subscription table
	   if (YES)
	   {
	   if (SUBSCR_TMR is running)
	   {
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_till_timer expires
	   }
	   }
	   else
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer

	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_till_timer expires
	   }

	   - route_avail_bool = active-route-available.

	   if(route_avail_bool=NO_ROUTE)
	   {
	   return NO_ROUTE;
	   }
	   else
	   {
	   for each result entry in the Subscrition result table
	   {
	   if (Destination is ADEST)
	   Send the message Depending on the SCOPE given as Input
	   else if (Destination is VDEST)
	   {
	   if role=ACTIVE
	   Send the message Depending on the
	   SCOPE (PCON,NODE and PWE) given as Input
	   if role=STANDBY
	   {
	   if NO_ACTIVE Timer is running
	   Enqueue <sel-obj>, <ready-to-go-msg> in
	   await-active-table
	   else
	   Drop the message
	   }
	   }
	   }
	   }
	 */
	SEND_MSG to_msg;
	memset(&to_msg, 0, sizeof(to_msg));

	to_msg.msg_type = MSG_NCSCONTEXT;
	to_msg.data.msg = msg;

	mcm_pvt_process_svc_bcast_common(env_hdl, fr_svc_id, to_msg, to_svc_id, req,
					 scope, pri, 0 /* For normal=0, red=1 */ );
	m_MDS_LOG_INFO("MDS_SND_RCV: Normal Bcast Message sent successfully from svc_id= %d, to svc_id=%d\n", fr_svc_id,
		       to_svc_id);
	return NCSCC_RC_SUCCESS;	/* This is why because broadcast whether sends or not should return Success */
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_process_svc_bcast_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_process_svc_bcast_common(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      SEND_MSG to_msg,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					      uint8_t flag /* For normal=0, red=1 */ )
{
	MDS_PWE_HDL pwe_hdl = (MDS_PWE_HDL)env_hdl;
	NCSCONTEXT hdl;
	MDS_SVC_INFO *svc_cb;
	MDS_SUBSCRIPTION_INFO *sub_info = NULL;	/* Subscription info */
	MDS_SUBSCRIPTION_RESULTS_INFO *info_result = NULL;
	uint8_t to;

	if (to_msg.msg_type == MSG_NCSCONTEXT) {
		if (to_msg.data.msg == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: null message\n");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (to_msg.data.info.buff == NULL) {
			m_MDS_LOG_ERR("MDS_SND_RCV: null message\n");
			return NCSCC_RC_FAILURE;
		}
	}

	/* Get SVC_cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(pwe_hdl, fr_svc_id, &hdl)) {
		m_MDS_LOG_ERR("MCM: SVC not present=%d\n", fr_svc_id);
		if (to_msg.msg_type == MSG_DIRECT_BUFF) {
			return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
		}
		return NCSCC_RC_FAILURE;
	}

	svc_cb = (MDS_SVC_INFO *)hdl;

	mds_subtn_tbl_get(svc_cb->svc_hdl, to_svc_id, &sub_info);

	if (scope < NCSMDS_SCOPE_INTRANODE || scope > NCSMDS_SCOPE_NONE) {
		m_MDS_LOG_ERR("MDS_SND_RCV: bcast scope not supported, svc-id=%d, scope=%d", fr_svc_id, scope);
		if (to_msg.msg_type == MSG_DIRECT_BUFF) {
			return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
		}
		return NCSCC_RC_FAILURE;
	}

	if (sub_info == NULL) {
		/* No subscription to this */
		/* Make a subscription to this service */
		m_MDS_LOG_INFO("MDS_SND_RCV: Broadcast :No subscription to service=%d, Making subscription\n",
			       to_svc_id);
		mds_mcm_subtn_add(svc_cb->svc_hdl, to_svc_id, svc_cb->install_scope, MDS_VIEW_RED /* redundantview */ ,
				  MDS_SUBTN_IMPLICIT);

		if (NCSCC_RC_SUCCESS != mds_subtn_tbl_get(svc_cb->svc_hdl, to_svc_id, &sub_info)) {
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Broadcast :Subscription made but no pointer got after subscription\n");
			if (to_msg.msg_type == MSG_DIRECT_BUFF) {
				return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
			}
			return NCSCC_RC_FAILURE;
		} 
	} 

	/* Get each destination and send */
	while (1) {
		if (flag == 0) {
			if (NCSCC_RC_SUCCESS !=
			    mds_subtn_res_tbl_getnext_active(svc_cb->svc_hdl, to_svc_id, &info_result)) {
				break;
			}
		} else if (flag == 1) {
			if (NCSCC_RC_SUCCESS != mds_subtn_res_tbl_getnext_any(svc_cb->svc_hdl, to_svc_id, &info_result)) {
				break;
			}
		}

		if (flag == 0) {
			/* Entry Found send or queue in await active depending on the destination */
			if (info_result->key.adest == 0) {

				/* Await active timer is running so queue */
				if (NCSCC_RC_SUCCESS != mcm_process_await_active(svc_cb, info_result, to_svc_id,
										 &to_msg, info_result->key.vdest_id,
										 req->i_sendtype, 0, pri)) {
					m_MDS_LOG_ERR("MDS_SND_RCV: Queueing in await active table failed\n");
				}
				continue;
			}
		}

		/* THis is used for the scope aspect of the bcast */
		switch (scope) {
		case NCSMDS_SCOPE_INTRANODE:
			if (NCSCC_RC_SUCCESS != mds_mcm_check_intranode(info_result->key.adest)) {
				continue;
			}
			break;
		default:
			/* Nothing to get */
			break;
		}

		mcm_query_for_node_dest_on_archword(info_result->key.adest, &to, info_result->rem_svc_arch_word);

		if (to == DESTINATION_SAME_PROCESS) {
			if (to_msg.msg_type != MSG_NCSCONTEXT) {
				SEND_MSG t_msg;
				memset(&t_msg, 0, sizeof(t_msg));
				t_msg.msg_type = MSG_DIRECT_BUFF;
				t_msg.data.info.buff = mds_alloc_direct_buff(to_msg.data.info.len);
				memcpy(t_msg.data.info.buff, to_msg.data.info.buff, to_msg.data.info.len);
				t_msg.data.info.len = to_msg.data.info.len;
				t_msg.msg_fmt_ver = to_msg.msg_fmt_ver;

				mds_mcm_send_msg_enc(to, svc_cb, &t_msg, to_svc_id, info_result->key.vdest_id,
						     req, 0, info_result->key.adest, pri);
				continue;
			}
		}

		mds_mcm_send_msg_enc(to, svc_cb, &to_msg, to_svc_id, info_result->key.vdest_id,
				     req, 0, info_result->key.adest, pri);
	}			/* While Loop */

#if 1
	if (to_msg.msg_type == MSG_DIRECT_BUFF) {
		mds_free_direct_buff(to_msg.data.info.buff);
	} else {
		mds_mcm_del_all_bcast_list(&to_msg);
	}
#endif

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_check_intranode
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_check_intranode(MDS_DEST adest)
{
	if (MDS_GET_NODE_ID(adest) == MDS_GET_NODE_ID(m_MDS_GET_ADEST))
		return NCSCC_RC_SUCCESS;

	else
		return NCSCC_RC_FAILURE;

}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_bcast
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_bcast(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
				   NCSCONTEXT msg,
				   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
				   NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri)
{
	/* Trivial validations already done?
	   STEP 1: From user-input get Dest-SVC-ID,PWE-ID
	   STEP 2: Check whether an entry for <Dest-SVC-ID,PWE-ID>
	   exists in the Global Subscription table
	   if (YES)
	   {
	   if (SUBSCR_TMR is running)
	   {
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_till_timer expires
	   }
	   }
	   else
	   {
	   - Create an implicit subscription
	   (scope = install-scope of sender, view = redundant)
	   - Start a subscription timer

	   * Subscription timer running (or just started) *
	   - Enqueue <sel-obj>, <send-type + destination-data>
	   - Wait_for_ind_on_sel_obj_till_timer expires
	   }

	   - route_avail_bool = route-available
	   (ADEST or VDEST(Active/Standby), PWE,SVC-ID).

	   if(route_avail_bool=NO_ROUTE)
	   {
	   return NO_ROUTE;
	   }
	   else
	   {
	   for each result entry in the Subscrition result table
	   {
	   Send the message Depending on the SCOPE
	   (PCON,NODE and PWE)given as Input
	   }

	   }
	   return NCSCC_RC_SUCCESS;
	 */
	SEND_MSG to_msg;
	memset(&to_msg, 0, sizeof(to_msg));

	to_msg.msg_type = MSG_NCSCONTEXT;
	to_msg.data.msg = msg;

	mcm_pvt_process_svc_bcast_common(env_hdl, fr_svc_id, to_msg, to_svc_id, req,
					 scope, pri, 1 /* For normal=0, red=1 */ );
	m_MDS_LOG_INFO("MDS_SND_RCV: RED Bcast Message sent successfully from svc_id= %d, to svc_id=%d\n", fr_svc_id,
		       to_svc_id);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_ll_data_rcv
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uint32_t mds_mcm_ll_data_rcv(MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: From the SVC_HDL in the structure recd, get the Pointer to SVCCB.
	   STEP 2: Depending on the SND_TYPE in the message recd call the respective
	   receive function by checking the role of the receiving service
	   as well.
	 */

	/*Check the role of SVC-HDL: Is this required here already validated before giving to upper layer */

	MDS_SVC_INFO *svccb = NULL;
	MDS_PWE_HDL pwe_hdl;
	MDS_SVC_ID svc_id;
	NCSCONTEXT hdl;
	uint32_t status = NCSCC_RC_SUCCESS;

	pwe_hdl = m_MDS_GET_PWE_HDL_FROM_SVC_HDL(recv->dest_svc_hdl);
	svc_id = m_MDS_GET_SVC_ID_FROM_SVC_HDL(recv->dest_svc_hdl);

	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(pwe_hdl, svc_id, &hdl)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Service doesnt exists\n");
		mds_mcm_free_msg_memory(recv->msg);
		return NCSCC_RC_FAILURE;
	}

	svccb = (MDS_SVC_INFO *)(hdl);

	if (NULL == svccb) {
		/* Free the Memory Depending on the data type as Ub or flat data etc */
		mds_mcm_free_msg_memory(recv->msg);
		/* log: MESSAGE Dropped */
		m_MDS_LOG_ERR("MDS_SND_RCV: Message is being dropped as the destination Service doesnt exists\n");
		return NCSCC_RC_FAILURE;
	}

	/* For the message loss indication */
	if ((true == svccb->i_msg_loss_indication) && 
			((recv->snd_type != MDS_SENDTYPE_ACK) || (recv->snd_type != MDS_SENDTYPE_RACK) )) {
		/* Get the subscription table result table function pointer */
		MDS_SUBSCRIPTION_RESULTS_INFO *lcl_subtn_res = NULL;
		if ( NCSCC_RC_SUCCESS == mds_get_subtn_res_tbl_by_adest(recv->dest_svc_hdl, recv->src_svc_id,
					recv->src_vdest, recv->src_adest, &lcl_subtn_res) ) {
			if (recv->src_seq_num != lcl_subtn_res->msg_rcv_cnt) {
				m_MDS_LOG_ERR
					("MDS_SND_RCV: msg loss detected, Src SVC=%d, Src vdest id= %d, Src adest=%llu, local svc id=%d msg num=%d, recvd cnt=%d\n", recv->src_svc_id, recv->src_vdest, recv->src_adest, svccb->svc_id, recv->src_seq_num, lcl_subtn_res->msg_rcv_cnt);
				
				mds_mcm_msg_loss(recv->dest_svc_hdl, recv->src_adest, 
					recv->src_svc_id, recv->src_vdest);
				lcl_subtn_res->msg_rcv_cnt = recv->src_seq_num;
				lcl_subtn_res->msg_rcv_cnt++;
			} else {
				lcl_subtn_res->msg_rcv_cnt++;
			}
		} else {
			m_MDS_LOG_INFO("MDS_SND_RCV: msg loss enabled but no subcription exists\n");
		}
	}

	switch (recv->snd_type) {
	case MDS_SENDTYPE_REDRSP:
		{
			status = mcm_recv_red_sndrsp(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_REDRACK:
		{
			status = mcm_recv_red_sndrack(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_REDACK:
		{
			status = mcm_recv_red_sndack(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_SND:
		{
			if (mds_svc_tbl_get_role(svccb->svc_hdl) != NCSCC_RC_SUCCESS) {
				/* Free the Memory Depending on the data type as Ub or flat data etc */
				mds_mcm_free_msg_memory(recv->msg);
				/* log: MESSAGE Dropped */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV: Message is being dropped as the destination Service is in standby mode=%d\n",
				     svccb->svc_id);
				return NCSCC_RC_FAILURE;
			}
			status = mcm_recv_normal_snd(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_SNDRSP:
		{
			if (mds_svc_tbl_get_role(svccb->svc_hdl) != NCSCC_RC_SUCCESS) {
				/* Free the Memory Depending on the data type as Ub or flat data etc */
				mds_mcm_free_msg_memory(recv->msg);
				/* log: MESSAGE Dropped */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV: Message is being dropped as the destination Service is in standby mode=%d\n",
				     svccb->svc_id);
				return NCSCC_RC_FAILURE;
			}
			status = mcm_recv_normal_sndrsp(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_SNDRACK:
		{
			status = mcm_recv_normal_sndrack(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_SNDACK:
		{
			if (mds_svc_tbl_get_role(svccb->svc_hdl) != NCSCC_RC_SUCCESS) {
				/* Free the Memory Depending on the data type as Ub or flat data etc */
				mds_mcm_free_msg_memory(recv->msg);
				/* log: MESSAGE Dropped */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV: Message is being dropped as the destination Service is in standby mode=%d\n",
				     svccb->svc_id);
				return NCSCC_RC_FAILURE;
			}
			status = mcm_recv_normal_sndack(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_RSP:
		{
			status = mcm_recv_normal_snd_rsp(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_RED:
		{
			status = mcm_recv_red_snd(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_RRSP:
		{
			status = mcm_recv_red_snd_rsp(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_BCAST:
		{
			if (mds_svc_tbl_get_role(svccb->svc_hdl) != NCSCC_RC_SUCCESS) {
				/* Free the Memory Depending on the data type as Ub or flat data etc */
				mds_mcm_free_msg_memory(recv->msg);
				/* log: MESSAGE Dropped */
				m_MDS_LOG_ERR
				    ("MDS_SND_RCV: Message is being dropped as the destination Service is in standby mode=%d\n",
				     svccb->svc_id);
				return NCSCC_RC_FAILURE;
			}
			status = mcm_recv_normal_bcast(svccb, recv);

		}
		break;

	case MDS_SENDTYPE_RBCAST:
		{
			status = mcm_recv_red_bcast(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_ACK:
		{
			status = mcm_recv_normal_ack(svccb, recv);
		}
		break;

	case MDS_SENDTYPE_RACK:
		{
			status = mcm_recv_red_ack(svccb, recv);
		}
		break;

	default:
		{
			/* Free the Memory Depending on the data type as Ub or flat data etc */
			mds_mcm_free_msg_memory(recv->msg);
			/* log: MESSAGE Dropped */
			return NCSCC_RC_FAILURE;
		}
		break;
	}
	return status;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_free_msg_memory
 *
 * Purpose:       To free MDS message in a "receive" flow
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 * NOTES:        This function is only being called in the receive flow
 *               If this is ever violated, then the mem-free actions for
 *               USRBUFs below. See code to free USRBUFs below.
 *                
 *
****************************************************************************/
static uint32_t mds_mcm_free_msg_memory(MDS_ENCODED_MSG msg)
{
	switch (msg.encoding) {
	case MDS_ENC_TYPE_CPY:
		/* Presently doing nothing */
		return NCSCC_RC_SUCCESS;
		break;

	case MDS_ENC_TYPE_FLAT:
		{
			/* See also NOTES above. This function is only expected to be called
			 ** in a receive flow. Hence, uba.ub will be the head-pointer.
			 */
			m_MMGR_FREE_BUFR_LIST(msg.data.flat_uba.ub);
			return NCSCC_RC_SUCCESS;
		}
		break;
	case MDS_ENC_TYPE_FULL:
		{
			/* See also NOTES above. This function is only expected to be called
			 ** in a receive flow. Hence, uba.ub will be the head-pointer.
			 */
			m_MMGR_FREE_BUFR_LIST(msg.data.fullenc_uba.ub);
			return NCSCC_RC_SUCCESS;
		}

	case MDS_ENC_TYPE_DIRECT_BUFF:
		{
			mds_free_direct_buff(msg.data.buff_info.buff);
			return NCSCC_RC_SUCCESS;
		}
		break;
	default:
		return NCSCC_RC_FAILURE;
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_recv_snd_msg_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_process_recv_snd_msg_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	uint32_t rc = 0;

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering  mds_mcm_process_recv_snd_msg_common\n");

	if (true == svccb->q_ownership) {
		if (NCSCC_RC_SUCCESS != mds_mcm_mailbox_post(svccb, recv, recv->pri)) {
			m_MDS_LOG_ERR("Mailbox post failed\n");
			mds_mcm_free_msg_memory(recv->msg);
			return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	} else {
		NCSMDS_CALLBACK_INFO cbinfo;
		NCSCONTEXT msg = NULL;
		memset(&cbinfo, 0, sizeof(cbinfo));
		cbinfo.i_yr_svc_hdl = svccb->yr_svc_hdl;
		cbinfo.i_yr_svc_id = svccb->svc_id;

		if ((recv->msg.encoding == MDS_ENC_TYPE_FLAT) || (recv->msg.encoding == MDS_ENC_TYPE_FULL)) {
			rc = mds_mcm_do_decode_full_or_flat(svccb, &cbinfo, recv, NULL);

			if (rc == NCSCC_RC_SUCCESS) {

				if (MDS_ENC_TYPE_FULL == recv->msg.encoding)
					msg = cbinfo.info.dec.o_msg;
				else
					msg = cbinfo.info.dec_flat.o_msg;
			} else {
				mds_mcm_free_msg_memory(recv->msg);
				return NCSCC_RC_FAILURE;
			}
			cbinfo.i_op = MDS_CALLBACK_RECEIVE;
			cbinfo.info.receive.i_fr_anc = recv->src_adest;

			if (recv->src_vdest == m_VDEST_ID_FOR_ADEST_ENTRY)
				cbinfo.info.receive.i_fr_dest = recv->src_adest;
			else
				cbinfo.info.receive.i_fr_dest = recv->src_vdest;

			cbinfo.info.receive.i_fr_svc_id = recv->src_svc_id;
			cbinfo.info.receive.i_msg = msg;
			if (recv->snd_type == MDS_SENDTYPE_SNDRSP || recv->snd_type == MDS_SENDTYPE_REDRSP) {
				uint8_t *data;
				cbinfo.info.receive.i_rsp_reqd = true;
				cbinfo.info.receive.i_msg_ctxt.length = MDS_SYNC_SND_CTXT_LEN_MAX;
				data = cbinfo.info.receive.i_msg_ctxt.data;
				ncs_encode_32bit(&data, recv->exchange_id);
				ncs_encode_64bit(&data, recv->src_adest);
			} else {
				cbinfo.info.receive.i_rsp_reqd = false;
				cbinfo.info.receive.i_msg_ctxt.length = 0;
			}
			cbinfo.info.receive.i_node_id = MDS_GET_NODE_ID(recv->src_adest);
			cbinfo.info.receive.i_to_dest = m_MDS_GET_ADEST;
			cbinfo.info.receive.i_to_svc_id = svccb->svc_id;
			cbinfo.info.receive.i_priority = recv->pri;
			cbinfo.info.receive.i_msg_fmt_ver = recv->msg_fmt_ver;
		} else if (recv->msg.encoding == MDS_ENC_TYPE_DIRECT_BUFF) {
			cbinfo.i_op = MDS_CALLBACK_DIRECT_RECEIVE;
			cbinfo.info.direct_receive.i_fr_anc = recv->src_adest;

			if (recv->src_vdest == m_VDEST_ID_FOR_ADEST_ENTRY)
				cbinfo.info.direct_receive.i_fr_dest = recv->src_adest;
			else
				cbinfo.info.direct_receive.i_fr_dest = recv->src_vdest;

			cbinfo.info.direct_receive.i_fr_svc_id = recv->src_svc_id;
			cbinfo.info.direct_receive.i_direct_buff = recv->msg.data.buff_info.buff;
			cbinfo.info.direct_receive.i_direct_buff_len = recv->msg.data.buff_info.len;

			if (recv->snd_type == MDS_SENDTYPE_SNDRSP || recv->snd_type == MDS_SENDTYPE_REDRSP) {
				uint8_t *data;
				cbinfo.info.direct_receive.i_rsp_reqd = true;
				cbinfo.info.direct_receive.i_msg_ctxt.length = MDS_SYNC_SND_CTXT_LEN_MAX;
				data = cbinfo.info.direct_receive.i_msg_ctxt.data;
				ncs_encode_32bit(&data, recv->exchange_id);
				ncs_encode_64bit(&data, recv->src_adest);
			} else {
				cbinfo.info.direct_receive.i_rsp_reqd = false;
				cbinfo.info.direct_receive.i_msg_ctxt.length = 0;
			}

			cbinfo.info.direct_receive.i_node_id = MDS_GET_NODE_ID(recv->src_adest);
			cbinfo.info.direct_receive.i_to_dest = m_MDS_GET_ADEST;
			cbinfo.info.direct_receive.i_to_svc_id = svccb->svc_id;
			cbinfo.info.direct_receive.i_priority = recv->pri;
			cbinfo.info.direct_receive.i_msg_fmt_ver = recv->msg_fmt_ver;
		} else if (recv->msg.encoding == MDS_ENC_TYPE_CPY) {
			cbinfo.i_op = MDS_CALLBACK_RECEIVE;

			if (recv->src_vdest == m_VDEST_ID_FOR_ADEST_ENTRY)
				cbinfo.info.receive.i_fr_dest = recv->src_adest;
			else
				cbinfo.info.receive.i_fr_dest = recv->src_vdest;

			cbinfo.info.receive.i_fr_anc = recv->src_adest;
			cbinfo.info.receive.i_fr_svc_id = recv->src_svc_id;
			cbinfo.info.receive.i_msg = recv->msg.data.cpy_msg;
			if (recv->snd_type == MDS_SENDTYPE_SNDRSP || recv->snd_type == MDS_SENDTYPE_REDRSP) {
				uint8_t *data;
				cbinfo.info.receive.i_rsp_reqd = true;
				cbinfo.info.receive.i_msg_ctxt.length = MDS_SYNC_SND_CTXT_LEN_MAX;
				data = cbinfo.info.receive.i_msg_ctxt.data;
				ncs_encode_32bit(&data, recv->exchange_id);
				ncs_encode_64bit(&data, recv->src_adest);
			} else {
				cbinfo.info.receive.i_rsp_reqd = false;
				cbinfo.info.receive.i_msg_ctxt.length = 0;
			}
			cbinfo.info.receive.i_node_id = MDS_GET_NODE_ID(recv->src_adest);
			cbinfo.info.receive.i_to_dest = m_MDS_GET_ADEST;
			cbinfo.info.receive.i_to_svc_id = svccb->svc_id;
			cbinfo.info.receive.i_priority = recv->pri;
			cbinfo.info.receive.i_msg_fmt_ver = recv->msg_fmt_ver;
		} else {
			mds_mcm_free_msg_memory(recv->msg);
			return NCSCC_RC_FAILURE;
		}

		rc = svccb->cback_ptr(&cbinfo);
		if (rc != NCSCC_RC_SUCCESS) {
			mds_mcm_free_msg_memory(recv->msg);
			m_MDS_LOG_ERR("MDS_SND_RCV: Receive callback failed<Svc=%d>", svccb->svc_id);
			return NCSCC_RC_FAILURE;
		}

		return NCSCC_RC_SUCCESS;
	}
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_snd
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_snd(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/* Message arrived means either role is quiesced or active */

	/* decode the UB depending upon the ENCODE Type in Message */
	/* Call the upper layer call back function ptrs with the data
	   or post message depending on the MDS-Q Ownership model */
	return mds_mcm_process_recv_snd_msg_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_snd
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_snd(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   Functionality is same as normal receive

	   STEP 1: decode the UB depending upon the ENCODE Type in Message
	   STEP 2: call the upper layer call back function ptrs with the data

	 */
	return mds_mcm_process_recv_snd_msg_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_bcast
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_bcast(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/* Functionality is same as normal receive

	   STEP 1: Check the role of the destination
	   STEP 2: if Role is standby
	   {
	   Drop the message
	   return
	   }
	   * else role is quiesced or active *

	   - decode the UB depending upon the ENCODE Type in Message
	   - Call the upper layer call back function ptrs with the data
	 */

	return mds_mcm_process_recv_snd_msg_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_bcast
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_bcast(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   Functionality is same as normal receive

	   STEP 1: decode the UB depending upon the ENCODE Type in Message
	   STEP 2: Call the upper layer call back function ptrs with the data
	 */

	return mds_mcm_process_recv_snd_msg_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_sndrsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_sndrsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: Check the role of the destination, (all ready done)
	   STEP 2: if Role is standby
	   {
	   Drop the message
	   return
	   }
	   * else role is quiesced or active *
	   - store the exchange-id, src-mdsdest( ADEST /<VDEST,ANCH> ) PWE_ID
	   in "resp_async_hdl" (MDS_SYNC_SND_CTXT_INFO).
	   - decode the UB depending upon the ENCODE Type in Message (bypass
	   for a copy case)
	   - Call the upper layer call back function ptrs with the data
	 */
	return mds_mcm_process_recv_snd_msg_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_sndrsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_sndrsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: store the exchange-id, src-mdsdest( ADEST /<VDEST,ANCH> ) PWE_ID
	   in "resp_async_hdl"(MDS_SYNC_SND_CTXT_INFO).
	   STEP 2: decode the UB depending upon the ENCODE Type in Message
	   STEP 3: Call the upper layer call back function ptrs with the data
	 */
	return mcm_recv_normal_sndrsp(svccb, recv);

}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_recv_sndrack_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_process_recv_sndrack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	MDS_DATA_RECV rcv1;
	memset(&rcv1, 0, sizeof(rcv1));
	memcpy(&rcv1, recv, sizeof(MDS_DATA_RECV));
	if (NCSCC_RC_SUCCESS != mds_mcm_process_rcv_snd_rsp_common(svccb, recv)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: recv sndrack message failed\n");
		return NCSCC_RC_FAILURE;
	}
	return mds_mcm_send_ack(svccb, &rcv1, SUCCESS, recv->pri);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_sndrack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_sndrack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: if (NO)
	   {
	   Drop the message
	   send an ACK with status = fail
	   return
	   }
	   - Decode the UB
	   - Put the UB into the SYNC_SEND_TBL entry and change status as SUCCESS
	   - raise selection object
	   - send an ACK message to the source of this message
	 */
	return mds_mcm_process_recv_sndrack_common(svccb, recv);

}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_sndrack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_sndrack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: if (NO)
	   {
	   Drop the message
	   send an ACK with status = fail
	   return
	   }
	   - Decode the UB
	   - Put the UB into the SYNC_SEND_TBL entry and change status as SUCCESS
	   - raise selection object
	   - send an ACK message to the source of this message
	 */
	return mds_mcm_process_recv_sndrack_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_recv_sndack_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_process_recv_sndack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	if (mds_mcm_process_recv_snd_msg_common(svccb, recv) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDS_SND_RCV: recv sndack failed\n");
		return NCSCC_RC_FAILURE;
	}
	return mds_mcm_send_ack(svccb, recv, SUCCESS, recv->pri);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_sndack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_sndack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: Check the role of the destination
	   STEP 2: if Role is standby
	   {
	   Drop the message
	   send an ACK with status = fail
	   return
	   }
	   else role is quiesced or active

	   - decode the UB depending upon the ENCODE Type in Message
	   - Call the upper layer call back function ptrs with the data
	   - send an ACK message to the source of this message
	 */
	return mds_mcm_process_recv_sndack_common(svccb, recv);

}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_sndack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_sndack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/* Role is quiesced or active or standby
	   STEP 1: Decode the UB depending upon the ENCODE Type in Message
	   STEP 2: Call the upper layer call back function ptrs with the data
	   STEP 3: send an ACK message to the source of this message
	 */
	return mds_mcm_process_recv_sndack_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_rcv_snd_rsp_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_process_rcv_snd_rsp_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	MDS_MCM_SYNC_SEND_QUEUE *result = NULL;

	if (mcm_pvt_get_sync_send_entry(svccb, recv, &result) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync entry doesnt exits\n");
		mds_mcm_free_msg_memory(recv->msg);
		return NCSCC_RC_FAILURE;
	}

	/* Exchange_id exists, Now decode the message based on the msg type recd and raise selection object */
	uint32_t rc = 0;

	NCSMDS_CALLBACK_INFO cbinfo;
	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.i_yr_svc_hdl = svccb->yr_svc_hdl;
	cbinfo.i_yr_svc_id = svccb->svc_id;

	if ((recv->msg.encoding == MDS_ENC_TYPE_FLAT) || (recv->msg.encoding == MDS_ENC_TYPE_FULL)) {
		rc = mds_mcm_do_decode_full_or_flat(svccb, &cbinfo, recv, result->orig_msg);

		if (rc == NCSCC_RC_SUCCESS) {

			if (MDS_ENC_TYPE_FULL == recv->msg.encoding) {
				result->recvd_msg.encoding = MDS_ENC_TYPE_FULL;
				result->sent_msg = cbinfo.info.dec.o_msg;
			} else {
				result->recvd_msg.encoding = MDS_ENC_TYPE_FLAT;
				result->sent_msg = cbinfo.info.dec_flat.o_msg;
			}
		} else {
			m_MDS_LOG_ERR("MDS_SND_RCV: decode full or flat failed=%d\n", svccb->svc_id);
			mds_mcm_free_msg_memory(recv->msg);
			return NCSCC_RC_FAILURE;
		}
	} else if (recv->msg.encoding == MDS_ENC_TYPE_DIRECT_BUFF) {
		result->recvd_msg.encoding = MDS_ENC_TYPE_DIRECT_BUFF;
		result->recvd_msg.data.buff_info.buff = recv->msg.data.buff_info.buff;
		result->recvd_msg.data.buff_info.len = recv->msg.data.buff_info.len;
	} else if (recv->msg.encoding == MDS_ENC_TYPE_CPY) {
		result->recvd_msg.encoding = MDS_ENC_TYPE_CPY;
		result->sent_msg = recv->msg.data.cpy_msg;
	} else {
		mds_mcm_free_msg_memory(recv->msg);
		return NCSCC_RC_FAILURE;
	}
	result->msg_fmt_ver = recv->msg_fmt_ver;
	/* Raise selection object and return */
	result->status = NCSCC_RC_SUCCESS;
	m_NCS_SEL_OBJ_IND(result->sel_obj);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_snd_rsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_snd_rsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: if (NO)
	   {
	   Drop the message
	   return
	   }
	   - Decode the UB
	   - Put the UB into the SYNC_SEND_TBL entry and change status as SUCCESS
	   - raise selection object (When this selection object is raised that select
	   call will return the status as SUCCESS to the USER with the response data)
	 */
	return mds_mcm_process_rcv_snd_rsp_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_snd_rsp
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_snd_rsp(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: if (NO)
	   {
	   Drop the message
	   return
	   }
	   - Decode the UB
	   - Put the UB into the SYNC_SEND_TBL entry and change status as SUCCESS
	   - raise selection object (When this selection object is raised that select
	   call will return the status as SUCCESS to the USER with the response data)
	 */
	return mds_mcm_process_rcv_snd_rsp_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_process_recv_ack_common
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_process_recv_ack_common(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	return mds_mcm_raise_selection_obj_for_ack(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_normal_ack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_normal_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: Check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: If (no)
	   {
	   Drop message
	   Return
	   }
	   raise selection object (When this selection object is raised that select
	   call will return the status as SUCCESS
	 */
	return mds_mcm_process_recv_ack_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mcm_recv_red_ack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_recv_red_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv)
{
	/*
	   STEP 1: Check whether there is entry in the SYNC_SEND_TBL matching the exchangeid
	   STEP 2: If (no)
	   {
	   Drop message
	   Return
	   }
	   raise selection object (When this selection object is raised that select
	   call will return the status as SUCCESS
	 */
	return mds_mcm_process_recv_ack_common(svccb, recv);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_mailbox_post
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_mailbox_post(MDS_SVC_INFO *dest_svc_cb, MDS_DATA_RECV *recv, MDS_SEND_PRIORITY_TYPE pri)
{
	/* Post a message */
	MDS_MCM_MSG_ELEM *msgelem;
	msgelem = m_MMGR_ALLOC_MSGELEM;
	if (msgelem == NULL) {
		/* Free the UB recd or any another message type, here memory free is removed because at the return of function it is being freed Found */
		m_MDS_LOG_ERR("MDS_SND_RCV: Unable to post the message due to memory failure allocation\n");
		return NCSCC_RC_FAILURE;
	}
	memset(msgelem, 0, sizeof(MDS_MCM_MSG_ELEM));

	msgelem->pri = recv->pri;
	msgelem->type = MDS_DATA_TYPE;
	msgelem->info.data.enc_msg = recv->msg;
	msgelem->info.data.fr_svc_id = recv->src_svc_id;
	msgelem->info.data.fr_pwe_id = recv->src_pwe_id;
	msgelem->info.data.fr_vdest_id = recv->src_vdest;
	msgelem->info.data.adest = recv->src_adest;
	msgelem->info.data.snd_type = recv->snd_type;
	msgelem->info.data.xch_id = recv->exchange_id;
	msgelem->info.data.msg_fmt_ver = recv->msg_fmt_ver;
	msgelem->info.data.src_svc_sub_part_ver = recv->src_svc_sub_part_ver;
	msgelem->info.data.arch_word = recv->msg_arch_word;
	m_MDS_LOG_INFO("MDS_SND_RCV: Posting into mailbox\n");

	return m_NCS_IPC_SEND(&dest_svc_cb->q_mbx, msgelem, pri);
}

/****************************************************************************
 *
 * Function Name: mds_mcm_do_decode_full_or_flat
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_do_decode_full_or_flat(MDS_SVC_INFO *svccb, NCSMDS_CALLBACK_INFO *cbinfo, MDS_DATA_RECV *recv_msg,
					    NCSCONTEXT orig_msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering  mds_mcm_do_decode_full_or_flat\n");
	/* Distinguish between intra-node and inter-node communication */
	if (MDS_ENC_TYPE_FULL == recv_msg->msg.encoding) {
		cbinfo->i_op = MDS_CALLBACK_DEC;
		cbinfo->info.dec.i_fr_svc_id = recv_msg->src_svc_id;
		cbinfo->info.dec.io_uba = &recv_msg->msg.data.fullenc_uba;
		cbinfo->info.dec.i_is_resp = false;
		cbinfo->info.dec.i_node_id = MDS_GET_NODE_ID(recv_msg->src_adest);	/* caution */
		cbinfo->info.dec.o_msg = orig_msg;

		cbinfo->info.dec.i_msg_fmt_ver = recv_msg->msg_fmt_ver;

		m_MDS_LOG_DBG("MDS_SND_RCV : calling callback ptr\n");

		if ((rc = svccb->cback_ptr(cbinfo)) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Decode full callback failed<Svc=%d>", svccb->svc_id);
			rc = NCSCC_RC_FAILURE;
		}

		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving  mds_mcm_do_decode_full_or_flat\n");
		return rc;
	} else if (MDS_ENC_TYPE_FLAT == recv_msg->msg.encoding) {
		cbinfo->i_op = MDS_CALLBACK_DEC_FLAT;
		cbinfo->info.dec_flat.i_fr_svc_id = recv_msg->src_svc_id;
		cbinfo->info.dec_flat.io_uba = &recv_msg->msg.data.flat_uba;
		cbinfo->info.dec_flat.i_is_resp = false;
		cbinfo->info.dec_flat.i_node_id = MDS_GET_NODE_ID(recv_msg->src_adest);
		cbinfo->info.dec_flat.o_msg = orig_msg;

		cbinfo->info.dec_flat.i_msg_fmt_ver = recv_msg->msg_fmt_ver;
		m_MDS_LOG_DBG("MDS_SND_RCV : calling callback ptr\n");

		if ((rc = svccb->cback_ptr(cbinfo)) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MDS_SND_RCV: Decode-flat callback failed<Svc=%d>", svccb->svc_id);
			rc = NCSCC_RC_FAILURE;
		}

		m_MDS_LOG_DBG("MDS_SND_RCV : Leaving  mds_mcm_do_decode_full_or_flat\n");
		return rc;
	}
	return rc;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_send_ack
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

static uint32_t mds_mcm_send_ack(MDS_SVC_INFO *svccb, MDS_DATA_RECV *recv, uint8_t flag, MDS_SEND_PRIORITY_TYPE pri)
{
	MDTM_SEND_REQ req;

	uint8_t to = 0;
	memset(&req, 0, sizeof(req));	/*  Fixed , crash in memory free in mds_dt_tipc.c */
	mcm_query_for_node_dest(recv->src_adest, &to);

	req.pri = pri;
	req.to = to;
	req.src_svc_id = svccb->svc_id;
	req.src_pwe_id = m_MDS_GET_PWE_ID_FROM_SVC_HDL(svccb->svc_hdl);
	req.src_vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svccb->svc_hdl);
	req.src_adest = m_MDS_GET_ADEST;

	req.snd_type = MDS_SENDTYPE_ACK;
	req.xch_id = recv->exchange_id;
	req.svc_seq_num = ++svccb->seq_no;

	req.dest_svc_id = recv->src_svc_id;
	req.dest_pwe_id = recv->src_pwe_id;
	req.dest_vdest_id = recv->src_vdest;
	req.adest = recv->src_adest;
	req.msg_fmt_ver = 1;	/* To be changed in 3.1 */

	if (to == DESTINATION_SAME_PROCESS) {
		if (NCSCC_RC_SUCCESS != mds_mdtm_send(&req)) {
			m_MDS_LOG_ERR("MDS_SND_RCV:  ACK Sent failed\n");
			return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	} else if (to == DESTINATION_ON_NODE || to == DESTINATION_OFF_NODE) {
		req.msg.encoding = MDS_ENC_TYPE_FLAT;
		if (NCSCC_RC_SUCCESS != mds_mdtm_send(&req)) {
			m_MDS_LOG_ERR("MDS_SND_RCV: ACK Sent failed\n");
			return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	}
	return NCSCC_RC_FAILURE;
}

/* Direct send*/
/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_snd_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_snd_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint32_t xch_id = 0;
	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	return mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_snd_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_snd_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					V_DEST_QA anchor,
					MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint32_t xch_id = 0;
	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	return mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id,
					      anchor);
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndrsp_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndrsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	MDS_SYNC_TXN_ID xch_id = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;
	uint32_t ret_status = 0;

	NCS_SEL_OBJ sel_obj;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	ret_status =
	    mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);

	if (NCSCC_RC_SUCCESS != ret_status) {
		/* delete the created the sync send entry */
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal sndrsp direct message SEND Failed from svc_id =%d to svc_id =%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		return ret_status;
	} else {
		mds_mutex_unlock();

		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrsp.i_time_to_wait)) {
			mds_mutex_lock();
			/* This is for response for local dest */
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				req->info.sndrsp.buff = sync_queue->recvd_msg.data.buff_info.buff;
				req->info.sndrsp.len = sync_queue->recvd_msg.data.buff_info.len;
				req->info.sndrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on  Direct sndrsp direct message from svc_id =%d to svc_id =%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response\n");
			req->info.sndrsp.buff = sync_queue->recvd_msg.data.buff_info.buff;
			req->info.sndrsp.len = sync_queue->recvd_msg.data.buff_info.len;
			req->info.sndrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndack_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					      MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					      MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					      MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	MDS_SYNC_TXN_ID xch_id = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	uint32_t ret_status = 0;

	NCS_SEL_OBJ sel_obj;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	ret_status =
	    mcm_pvt_normal_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id);

	if (NCSCC_RC_SUCCESS != ret_status) {
		/* delete the created the sync send entry */
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal sndack direct message SEND Failed from svc_id=%d to svc_id=%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		return ret_status;
	} else {
		mds_mutex_unlock();

		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndack.i_time_to_wait)) {
			/* This is for response for local dest */
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on sndack direct message from svc_id=%d to svc_id=%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_sndrack_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_sndrack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint8_t *data;
	uint32_t xch_id = 0, ret_status = 0;
	uint64_t msg_dest_adest = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue;
	uint8_t len_sync_ctxt = 0;

	NCS_SEL_OBJ sel_obj;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	data = req->info.sndrack.i_msg_ctxt.data;

	len_sync_ctxt = req->info.sndrack.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	/* Getting the original things */
	xch_id = ncs_decode_32bit(&data);
	msg_dest_adest = ncs_decode_64bit(&data);

	/* Now create a sync_send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	sync_queue->dest_sndrack_adest.adest = msg_dest_adest;

	ret_status = mcm_pvt_process_sndrack_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, 0);

	if (NCSCC_RC_SUCCESS != ret_status) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Normal sndrack direct message SEND Failed from svc_id=%d to svc_id=%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, msg_dest_adest);
		return ret_status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.sndrack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* for local case */
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    msg_dest_adest);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on Normal sndrack direct messagefrom svc_id=%d to svc_id=%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_snd_rsp_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_snd_rsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					       MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					       MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					       MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint32_t xch_id = 0;
	MDS_DEST adest = 0;
	uint8_t *data;
	uint8_t len_sync_ctxt = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	data = req->info.rsp.i_msg_ctxt.data;
	len_sync_ctxt = req->info.rsp.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	xch_id = ncs_decode_32bit(&data);
	adest = ncs_decode_64bit(&data);

	return mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id,
					      adest);
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndrsp_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndrsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   V_DEST_QA anchor,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	MDS_SYNC_TXN_ID xch_id;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	uint32_t ret_status = 0;

	NCS_SEL_OBJ sel_obj;

	xch_id = ++mds_mcm_global_exchange_id;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	ret_status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, anchor);

	if (NCSCC_RC_SUCCESS != ret_status) {
		/* delete the created the sync send entry */
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndrsp direct message SEND Failed from svc_id=%d to svc_id=%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		return ret_status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrsp.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				req->info.redrsp.buff = sync_queue->recvd_msg.data.buff_info.buff;
				req->info.redrsp.len = sync_queue->recvd_msg.data.buff_info.len;
				req->info.redrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on RED sndrsp direct message Failed from svc_id=%d to svc_id=%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the response\n");
			req->info.redrsp.buff = sync_queue->recvd_msg.data.buff_info.buff;
			req->info.redrsp.len = sync_queue->recvd_msg.data.buff_info.len;
			req->info.redrsp.o_msg_fmt_ver = sync_queue->msg_fmt_ver;
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndrack_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndrack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					    V_DEST_QA anchor,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint8_t *data;
	uint32_t xch_id = 0, ret_status = 0;
	uint64_t msg_dest_adest = 0;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue;

	NCS_SEL_OBJ sel_obj;

	uint8_t len_sync_ctxt = 0;
	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	data = req->info.redrack.i_msg_ctxt.data;
	len_sync_ctxt = req->info.redrack.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	/* Getting the original things */
	xch_id = ncs_decode_32bit(&data);
	msg_dest_adest = ncs_decode_64bit(&data);

	/* Now create a sync_send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	sync_queue->dest_sndrack_adest.adest = msg_dest_adest;

	ret_status = mcm_pvt_process_sndrack_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, anchor);

	if (NCSCC_RC_SUCCESS != ret_status) {
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndrack direct message SEND Failed Failed from svc_id=%d to svc_id=%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, msg_dest_adest);
		return ret_status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redrack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    msg_dest_adest);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on  RED sndrack direct message Failed from svc_id=%d to svc_id=%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
						    msg_dest_adest);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_sndack_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_sndack_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					   MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					   V_DEST_QA anchor,
					   MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					   MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	MDS_SYNC_TXN_ID xch_id;
	MDS_MCM_SYNC_SEND_QUEUE *sync_queue = NULL;

	uint32_t ret_status = 0;

	NCS_SEL_OBJ sel_obj;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	xch_id = ++mds_mcm_global_exchange_id;

	/* First create a sync send entry */
	if (NCSCC_RC_SUCCESS !=
	    mcm_pvt_create_sync_send_entry(env_hdl, fr_svc_id, req->i_sendtype, xch_id, &sync_queue, NULL)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Sync send entry creation failed\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	memset(&sel_obj, 0, sizeof(sel_obj));
	sel_obj = sync_queue->sel_obj;

	ret_status =
	    mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id, anchor);

	if (NCSCC_RC_SUCCESS != ret_status) {
		/* delete the created the sync send entry */
		m_MDS_LOG_ERR("MDS_SND_RCV: RED sndack direct message SEND Failed from svc_id=%d to svc_id=%d",
			      fr_svc_id, to_svc_id);
		m_MDS_ERR_PRINT_ADEST(to_dest);
		m_MDS_ERR_PRINT_ANCHOR(anchor);
		mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
		return ret_status;
	} else {
		mds_mutex_unlock();
		if (NCSCC_RC_SUCCESS != mds_mcm_time_wait(sel_obj, req->info.redack.i_time_to_wait)) {
			mds_mutex_lock();
			if (sync_queue->status == NCSCC_RC_SUCCESS) {
				/* sucess case */
				mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype,
							    0);
				m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack also\n");
				return NCSCC_RC_SUCCESS;
			}
			m_MDS_LOG_ERR
			    ("MDS_SND_RCV: Timeout occured on RED sndack direct  message from svc_id=%d to svc_id=%d",
			     fr_svc_id, to_svc_id);
			m_MDS_ERR_PRINT_ADEST(to_dest);
			m_MDS_ERR_PRINT_ANCHOR(anchor);
			mds_await_active_tbl_del_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id,
						       to_svc_id, m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(to_dest),
						       req->i_sendtype);
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_REQ_TIMOUT;
		} else {
			mds_mutex_lock();
			if (NCSCC_RC_SUCCESS != mds_check_for_mds_existence(sel_obj, env_hdl, fr_svc_id, to_svc_id)) {
				m_MDS_LOG_ERR("MDS_SND_RCV: MDS entry doesnt exist\n");
				return NCSCC_RC_FAILURE;
			}
			m_MDS_LOG_INFO("MDS_SND_RCV: Successfully recd the ack\n");
			mcm_pvt_del_sync_send_entry((MDS_PWE_HDL)env_hdl, fr_svc_id, xch_id, req->i_sendtype, 0);
			return NCSCC_RC_SUCCESS;
		}
	}
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_snd_rsp_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_snd_rsp_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					    MDS_DIRECT_BUFF buff, uint16_t len, MDS_DEST to_dest,
					    V_DEST_QA anchor,
					    MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					    MDS_SEND_PRIORITY_TYPE pri, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	uint32_t xch_id;
	MDS_DEST anchor_1 = 0;
	uint8_t *data;
	uint8_t len_sync_ctxt = 0;

	SEND_MSG send_msg;
	memset(&send_msg, 0, sizeof(send_msg));

	send_msg.msg_type = MSG_DIRECT_BUFF;
	send_msg.data.info.buff = buff;
	send_msg.data.info.len = len;
	send_msg.msg_fmt_ver = msg_fmt_ver;

	data = req->info.rrsp.i_msg_ctxt.data;
	len_sync_ctxt = req->info.rrsp.i_msg_ctxt.length;

	if ((len_sync_ctxt == 0) || (len_sync_ctxt != MDS_SYNC_SND_CTXT_LEN_MAX)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Invalid Sync CTXT Len\n");
		return MDS_INT_RC_DIRECT_SEND_FAIL;	/* This is as the direct buff is freed at a common location */
	}

	xch_id = ncs_decode_32bit(&data);
	anchor_1 = ncs_decode_64bit(&data);

	return mcm_pvt_red_snd_process_common(env_hdl, fr_svc_id, send_msg, to_dest, to_svc_id, req, pri, xch_id,
					      anchor_1);
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_normal_svc_bcast_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_normal_svc_bcast_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					     MDS_DIRECT_BUFF buff, uint16_t len,
					     MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					     NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					     MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	SEND_MSG to_msg;
	memset(&to_msg, 0, sizeof(to_msg));

	to_msg.msg_type = MSG_DIRECT_BUFF;
	to_msg.data.info.buff = buff;
	to_msg.data.info.len = len;
	to_msg.msg_fmt_ver = msg_fmt_ver;

	return mcm_pvt_process_svc_bcast_common(env_hdl, fr_svc_id, to_msg, to_svc_id, req,
						scope, pri, 0 /* For normal=0, red=1 */ );
}

/****************************************************************************
 *
 * Function Name: mcm_pvt_red_svc_bcast_direct
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_pvt_red_svc_bcast_direct(MDS_HDL env_hdl, MDS_SVC_ID fr_svc_id,
					  MDS_DIRECT_BUFF buff, uint16_t len,
					  MDS_SVC_ID to_svc_id, MDS_SEND_INFO *req,
					  NCSMDS_SCOPE_TYPE scope, MDS_SEND_PRIORITY_TYPE pri,
					  MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver)
{
	SEND_MSG to_msg;
	memset(&to_msg, 0, sizeof(to_msg));

	to_msg.msg_type = MSG_DIRECT_BUFF;
	to_msg.data.info.buff = buff;
	to_msg.data.info.len = len;
	to_msg.msg_fmt_ver = msg_fmt_ver;

	return mcm_pvt_process_svc_bcast_common(env_hdl, fr_svc_id, to_msg, to_svc_id, req,
						scope, pri, 1 /* For normal=0, red=1 */ );
}

/****************************************************************************
 *
 * Function Name: mds_retrieve
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mailbox_proc(MDS_MCM_MSG_ELEM *msgelem, MDS_SVC_INFO *svc_cb);

uint32_t mds_retrieve(NCSMDS_INFO *info)
{
	MDS_SVC_ID svc_id = info->i_svc_id;
	SYSF_MBX local_mbx = 0;
	MDS_MCM_MSG_ELEM *msgelem = NULL;
	NCSCONTEXT hdl;
	MDS_SVC_INFO *svc_cb;

	/* Now get the svccb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get((MDS_PWE_HDL)(info->i_mds_hdl), svc_id, &hdl)) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Service doesnt exists\n");
		return NCSCC_RC_FAILURE;
	}
	svc_cb = (MDS_SVC_INFO *)hdl;
	local_mbx = svc_cb->q_mbx;

	if (svc_cb->q_ownership != true) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Service was not installed with MDS Q-Ownership option\n");
		return NCSCC_RC_FAILURE;
	}

	if (info->info.retrieve_msg.i_dispatchFlags == SA_DISPATCH_BLOCKING) {

		/*  SA_DISPATCH_BLOCKING resulting in to deadlock */
		while (true) {
			/* Unlock the mds_lock first */
			mds_mutex_unlock();

			msgelem = (MDS_MCM_MSG_ELEM *)m_NCS_IPC_RECEIVE(&local_mbx, msgelem);

			/* take mds_lock again */
			mds_mutex_lock();

			if (msgelem == NULL) {
				/* we will reach here only if IPC mail box is released */
				/* just return failure without unlocking mds_lock, it will be done in ncsmds_api() */
				m_MDS_LOG_INFO("MDS_SND_RCV: Service doesnt exists : IPC mailbox is released \n");
				return NCSCC_RC_SUCCESS;
			} else {
				/* Check whether service exists before doing any processing */
				if (NCSCC_RC_SUCCESS != mds_svc_tbl_get((MDS_PWE_HDL)(info->i_mds_hdl), svc_id, &hdl)) {
					/* We should never reach here as Mailbox existance without service existance is invalid state */
					m_MDS_LOG_INFO
					    ("MDS_SND_RCV: Service doesnt exists : But Message received in IPC mailbox\n");
					m_MMGR_FREE_MSGELEM(msgelem);
					return NCSCC_RC_SUCCESS;
				}
				svc_cb = (MDS_SVC_INFO *)hdl;

				/* Validate the service (its mailbox and q_ownership model) */
				if (local_mbx != svc_cb->q_mbx) {
					m_MDS_LOG_INFO
					    ("MDS_SND_RCV: Service doesnt exists : New Mailbox created, means service unistalled and installed again\n");
					m_MMGR_FREE_MSGELEM(msgelem);
					return NCSCC_RC_SUCCESS;
				}
				if (svc_cb->q_ownership != true) {
					m_MDS_LOG_INFO
					    ("MDS_SND_RCV: Service doesnt exists : Service uninstalled and installed again without mds Q-Ownership model\n");
					m_MMGR_FREE_MSGELEM(msgelem);
					return NCSCC_RC_SUCCESS;
				}
				if (mds_mailbox_proc(msgelem, svc_cb) == NCSCC_RC_NO_OBJECT) {
					return NCSCC_RC_FAILURE;
				}
			}
		}
	} else if (info->info.retrieve_msg.i_dispatchFlags == SA_DISPATCH_ONE) {
		uint32_t status = NCSCC_RC_SUCCESS;
		msgelem = (MDS_MCM_MSG_ELEM *)m_NCS_IPC_NON_BLK_RECEIVE(&local_mbx, msgelem);
		if (msgelem == NULL) {
			return NCSCC_RC_FAILURE;
		}
		status = mds_mailbox_proc(msgelem, svc_cb);
		if (status == NCSCC_RC_NO_OBJECT) {
			status = NCSCC_RC_FAILURE;
		}
		return status;
	} else if (info->info.retrieve_msg.i_dispatchFlags == SA_DISPATCH_ALL) {
		while ((msgelem = (MDS_MCM_MSG_ELEM *)m_NCS_IPC_NON_BLK_RECEIVE(&local_mbx, NULL)) != NULL) {
			/* IR Fix 82530 */
			if (mds_mailbox_proc(msgelem, svc_cb) == NCSCC_RC_NO_OBJECT) {
				m_MDS_LOG_INFO
				    ("MDS_SND_RCV: Svc doesnt exists after calling the mailbox_proc, when called(MDS Q-Ownership,DIS-ALL), svc_id=%d",
				     svc_id);
				return NCSCC_RC_FAILURE;
			}
			msgelem = NULL;
		}
		return NCSCC_RC_SUCCESS;
	}
	m_MDS_LOG_ERR("MDS_SND_RCV: Retrieve Dispatch Flag Not Supported=%d", info->info.retrieve_msg.i_dispatchFlags);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_mailbox_proc
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mailbox_proc(MDS_MCM_MSG_ELEM *msgelem, MDS_SVC_INFO *svc_cb)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	NCSMDS_CALLBACK_INFO cbinfo;
	uint32_t svc_id = svc_cb->svc_id;
	MDS_SVC_HDL svc_hdl = svc_cb->svc_hdl;
	NCSMDS_CALLBACK_API local_cb_ptr = NULL;

	if (msgelem->type == MDS_DATA_TYPE) {
		memset(&cbinfo, 0, sizeof(cbinfo));
		cbinfo.i_yr_svc_hdl = svc_cb->yr_svc_hdl;
		cbinfo.i_yr_svc_id = svc_cb->svc_id;

		switch (msgelem->info.data.enc_msg.encoding) {
		case MDS_ENC_TYPE_CPY:
			{
				cbinfo.info.receive.i_msg = msgelem->info.data.enc_msg.data.cpy_msg;
				cbinfo.info.receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;
			}
			break;
		case MDS_ENC_TYPE_FULL:
			{
				cbinfo.i_op = MDS_CALLBACK_DEC;
				cbinfo.info.dec.i_fr_svc_id = msgelem->info.data.fr_svc_id;
				cbinfo.info.dec.io_uba = &msgelem->info.data.enc_msg.data.fullenc_uba;

				cbinfo.info.dec.i_is_resp = false;

				cbinfo.info.dec.i_node_id = MDS_GET_NODE_ID(msgelem->info.data.adest);
				cbinfo.info.dec.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;

				/*  Fixed, mds q ownership callbacks should not take lock */
				local_cb_ptr = svc_cb->cback_ptr;
				mds_mutex_unlock();
				status = local_cb_ptr(&cbinfo);
				mds_mutex_lock();

				if (mds_validate_svc_cb(svc_cb, svc_hdl, svc_id) != NCSCC_RC_SUCCESS) {
					if (msgelem->info.data.enc_msg.data.fullenc_uba.ub != NULL) {
						mds_mcm_free_msg_memory(msgelem->info.data.enc_msg);
					}
					status = NCSCC_RC_NO_OBJECT;
					m_MDS_LOG_INFO
					    ("MDS_SND_RCV: Service doesnt exists : after calling the callback dec_full(MDS Q-Ownership), svc_id=%d",
					     svc_id);
					goto out2;
				}

				if (status != NCSCC_RC_SUCCESS) {
					m_MDS_LOG_ERR("MDS_SND_RCV: decode full failed of SVC=%d\n", svc_id);
					goto out2;
				}

				cbinfo.info.receive.i_msg = cbinfo.info.dec.o_msg;
				cbinfo.info.receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;

			}
			break;
		case MDS_ENC_TYPE_FLAT:
			{
				cbinfo.i_op = MDS_CALLBACK_DEC_FLAT;
				cbinfo.info.dec_flat.i_fr_svc_id = msgelem->info.data.fr_svc_id;
				cbinfo.info.dec_flat.io_uba = &msgelem->info.data.enc_msg.data.flat_uba;

				cbinfo.info.dec_flat.i_is_resp = false;

				cbinfo.info.dec_flat.i_node_id = MDS_GET_NODE_ID(msgelem->info.data.adest);
				cbinfo.info.dec_flat.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;

				/*  Fixed, mds q ownership callbacks should not take lock */

				local_cb_ptr = svc_cb->cback_ptr;
				mds_mutex_unlock();
				status = local_cb_ptr(&cbinfo);
				mds_mutex_lock();

				if (mds_validate_svc_cb(svc_cb, svc_hdl, svc_id) != NCSCC_RC_SUCCESS) {
					if (msgelem->info.data.enc_msg.data.flat_uba.ub != NULL) {
						mds_mcm_free_msg_memory(msgelem->info.data.enc_msg);
					}
					status = NCSCC_RC_NO_OBJECT;
					m_MDS_LOG_INFO
					    ("MDS_SND_RCV: Service doesnt exists : after calling the callback dec_flat(MDS Q-Ownership), svc_id=%d",
					     svc_id);
					goto out2;
				}

				if (status != NCSCC_RC_SUCCESS) {
					m_MDS_LOG_ERR("MDS_SND_RCV: decode flat failed of SVC=%d\n", svc_id);
					goto out2;
				}

				cbinfo.info.receive.i_msg = cbinfo.info.dec_flat.o_msg;
				cbinfo.info.receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;
			}
			break;
		case MDS_ENC_TYPE_DIRECT_BUFF:
			{
				cbinfo.info.direct_receive.i_direct_buff =
				    msgelem->info.data.enc_msg.data.buff_info.buff;
				cbinfo.info.direct_receive.i_direct_buff_len =
				    msgelem->info.data.enc_msg.data.buff_info.len;
				cbinfo.info.direct_receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;
			}
			break;
		default:
			break;
		}

		switch (msgelem->info.data.enc_msg.encoding) {
		case MDS_ENC_TYPE_CPY:
		case MDS_ENC_TYPE_FLAT:
		case MDS_ENC_TYPE_FULL:
			{
				cbinfo.i_op = MDS_CALLBACK_RECEIVE;
				if (msgelem->info.data.snd_type == MDS_SENDTYPE_SNDRSP
				    || msgelem->info.data.snd_type == MDS_SENDTYPE_REDRSP) {
					uint8_t *data;
					cbinfo.info.receive.i_rsp_reqd = true;
					cbinfo.info.receive.i_msg_ctxt.length = MDS_SYNC_SND_CTXT_LEN_MAX;

					data = cbinfo.info.receive.i_msg_ctxt.data;
					ncs_encode_32bit(&data, msgelem->info.data.xch_id);
					ncs_encode_64bit(&data, msgelem->info.data.adest);
				} else
					cbinfo.info.receive.i_rsp_reqd = false;

				if (msgelem->info.data.fr_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY)
					cbinfo.info.receive.i_fr_dest = msgelem->info.data.adest;
				else
					cbinfo.info.receive.i_fr_dest = msgelem->info.data.fr_vdest_id;

				cbinfo.info.receive.i_fr_anc = msgelem->info.data.adest;
				cbinfo.info.receive.i_fr_svc_id = msgelem->info.data.fr_svc_id;

				cbinfo.info.receive.i_to_dest = m_MDS_GET_ADEST;
				cbinfo.info.receive.i_to_svc_id = svc_cb->svc_id;

				cbinfo.info.receive.i_node_id = MDS_GET_NODE_ID(msgelem->info.data.adest);
				cbinfo.info.receive.i_priority = msgelem->pri;
				cbinfo.info.receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;
			}
			break;

		case MDS_ENC_TYPE_DIRECT_BUFF:
			{
				cbinfo.i_op = MDS_CALLBACK_DIRECT_RECEIVE;
				if (msgelem->info.data.snd_type == MDS_SENDTYPE_SNDRSP
				    || msgelem->info.data.snd_type == MDS_SENDTYPE_REDRSP) {
					uint8_t *data;
					cbinfo.info.direct_receive.i_rsp_reqd = true;
					cbinfo.info.direct_receive.i_msg_ctxt.length = MDS_SYNC_SND_CTXT_LEN_MAX;

					data = cbinfo.info.direct_receive.i_msg_ctxt.data;
					ncs_encode_32bit(&data, msgelem->info.data.xch_id);
					ncs_encode_64bit(&data, msgelem->info.data.adest);
				} else
					cbinfo.info.direct_receive.i_rsp_reqd = false;

				if (msgelem->info.data.fr_vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY)
					cbinfo.info.direct_receive.i_fr_dest = msgelem->info.data.adest;
				else
					cbinfo.info.direct_receive.i_fr_dest = msgelem->info.data.fr_vdest_id;

				cbinfo.info.direct_receive.i_fr_anc = msgelem->info.data.adest;
				cbinfo.info.direct_receive.i_fr_svc_id = msgelem->info.data.fr_svc_id;

				cbinfo.info.direct_receive.i_to_dest = m_MDS_GET_ADEST;
				cbinfo.info.direct_receive.i_to_svc_id = svc_cb->svc_id;

				cbinfo.info.direct_receive.i_node_id = MDS_GET_NODE_ID(msgelem->info.data.adest);
				cbinfo.info.direct_receive.i_priority = msgelem->pri;
				cbinfo.info.direct_receive.i_msg_fmt_ver = msgelem->info.data.msg_fmt_ver;
			}
			break;

		default:
			break;
		}
 out2:
		if (status != NCSCC_RC_SUCCESS) {
			if (status == NCSCC_RC_FAILURE) {	/* This check is for validation of svc_cb */
				mds_mcm_free_msg_memory(msgelem->info.data.enc_msg);
			}
			m_MMGR_FREE_MSGELEM(msgelem);
			return status;
		}

		/*  Fixed, mds q ownership callbacks should not take lock */

		local_cb_ptr = svc_cb->cback_ptr;
		mds_mutex_unlock();
		status = local_cb_ptr(&cbinfo);
		mds_mutex_lock();

		if (mds_validate_svc_cb(svc_cb, svc_hdl, svc_id) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_INFO
			    ("MDS_SND_RCV: Service doesnt exists : after calling the callback rec or direct_rec(MDS Q-Ownership), svc_id=%d",
			     svc_id);
			status = NCSCC_RC_NO_OBJECT;
		}

		if (NCSCC_RC_SUCCESS != status) {
			if (status == NCSCC_RC_FAILURE) {
				m_MDS_LOG_ERR("MDS_SND_RCV: receive failed\n");
				mds_mcm_free_msg_memory(msgelem->info.data.enc_msg);
			}
		}
		m_MMGR_FREE_MSGELEM(msgelem);

		return status;
	} else {
		/* Events type */

		/*  Fixed, mds q ownership callbacks should not take lock */

		local_cb_ptr = svc_cb->cback_ptr;
		mds_mutex_unlock();
		status = local_cb_ptr(&msgelem->info.event.cbinfo);
		mds_mutex_lock();

		if (mds_validate_svc_cb(svc_cb, svc_hdl, svc_id) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_INFO
			    ("MDS_SND_RCV: Service doesnt exists : after calling the callback for the Events(MDS Q-Ownership), svc_id=%d",
			     svc_id);
			status = NCSCC_RC_NO_OBJECT;
		}

		m_MMGR_FREE_MSGELEM(msgelem);
		return status;
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_validate_svc_cb
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_validate_svc_cb(MDS_SVC_INFO *svc_cb, MDS_SVC_HDL svc_hdl, MDS_SVC_ID svc_id)
{
	MDS_SVC_INFO *local_svc_cb = NULL;
	NCSCONTEXT hdl;

	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_hdl), svc_id, &hdl)) {
		m_MDS_LOG_INFO
		    ("MDS_SND_RCV: Service Uninstalled while calling the callback's in MDS Q-Ownership model, svc_id=%d",
		     svc_id);
		return NCSCC_RC_FAILURE;
	}

	local_svc_cb = (MDS_SVC_INFO *)(hdl);

	if (svc_cb != local_svc_cb) {
		m_MDS_LOG_INFO("MDS_SND_RCV: Service Uninstalled and installed again, svc_id=%d", svc_id);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 *
 * Function Name: mds_await_active_tbl_send
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uint32_t mds_await_active_tbl_send(MDS_AWAIT_ACTIVE_QUEUE *hdr, 
				MDS_DEST adest, MDS_SVC_HDL svc_hdl)
{
	MDS_AWAIT_ACTIVE_QUEUE *queue, *mov_ptr = NULL;
	MDTM_SEND_REQ req;
	uint8_t to;
	MDS_SUBSCRIPTION_RESULTS_INFO *lcl_subtn_res = NULL;

	memset(&req, 0, sizeof(req));
	mcm_query_for_node_dest(adest, &to);

	queue = hdr;

	if (queue == NULL) {
		return NCSCC_RC_SUCCESS;
	}

	while (queue != NULL) {
		req = queue->req;
		req.to = to;
		mds_get_subtn_res_tbl_by_adest(svc_hdl, req.dest_svc_id,
				req.dest_vdest_id, adest, &lcl_subtn_res);
		req.svc_seq_num = lcl_subtn_res->msg_snd_cnt++;
		req.adest = adest;
		mds_mdtm_send(&req);
		mov_ptr = queue;
		queue = queue->next_msg;
		m_MMGR_FREE_AWAIT_ACTIVE(mov_ptr);
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_await_active_tbl_add
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uint32_t mds_await_active_tbl_add(MDS_SUBSCRIPTION_RESULTS_INFO *info, MDTM_SEND_REQ req)
{
	MDS_AWAIT_ACTIVE_QUEUE *hdr = NULL, *add_ptr;
	hdr = info->info.active_vdest.active_route_info->await_active_queue;

	add_ptr = m_MMGR_ALLOC_AWAIT_ACTIVE;

	if (add_ptr == NULL) {
		m_MDS_LOG_ERR("MDS_SND_RCV: Memory allocation to await active failed\n");
		return NCSCC_RC_FAILURE;
	}

	memset(add_ptr, 0, sizeof(MDS_AWAIT_ACTIVE_QUEUE));

	add_ptr->req = req;

	add_ptr->next_msg = NULL;

	if (hdr == NULL) {
		/* no elements add in start */
		info->info.active_vdest.active_route_info->await_active_queue = add_ptr;
		return NCSCC_RC_SUCCESS;
	} else {
		/* Need to add at the end */
		if (hdr->next_msg == NULL) {
			hdr->next_msg = add_ptr;
			return NCSCC_RC_SUCCESS;
		} else {
			while (hdr != NULL) {
				if (hdr->next_msg == NULL) {
					hdr->next_msg = add_ptr;
					return NCSCC_RC_SUCCESS;
				}
				hdr = hdr->next_msg;
			}
		}
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_await_active_tbl_del
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uint32_t mds_await_active_tbl_del(MDS_AWAIT_ACTIVE_QUEUE *queue)
{
	MDS_AWAIT_ACTIVE_QUEUE *mov_ptr, *del_ptr;
	mov_ptr = queue;

	m_MDS_LOG_DBG("MDS_SND_RCV : Entering : mds_await_active_tbl_del");

	if (mov_ptr == NULL) {
		/* Deleting release_mds_lock() from here as it is resulting in segmantation fault
		   if some other thread is parallely executing Uninstall/Unusbscribe */
		return NCSCC_RC_SUCCESS;
	}

	while (mov_ptr != NULL) {
		del_ptr = mov_ptr;
		mov_ptr = mov_ptr->next_msg;

		mds_mcm_free_msg_uba_start(del_ptr->req.msg);

		m_MMGR_FREE_AWAIT_ACTIVE(del_ptr);
	}
	queue = NULL;

	m_MDS_LOG_DBG("MDS_SND_RCV : Leaving : mds_await_active_tbl_del");

	return NCSCC_RC_SUCCESS;
}

/*********************************************************
  Function NAME: mds_mcm_free_msg_memory
*********************************************************/
uint32_t mds_mcm_free_msg_uba_start(MDS_ENCODED_MSG msg)
{
	switch (msg.encoding) {
	case MDS_ENC_TYPE_CPY:
		/* Presently doing nothing */
		return NCSCC_RC_SUCCESS;
		break;

	case MDS_ENC_TYPE_FLAT:
		{
			m_MMGR_FREE_BUFR_LIST(msg.data.flat_uba.start);
			return NCSCC_RC_SUCCESS;
		}
		break;
	case MDS_ENC_TYPE_FULL:
		{
			m_MMGR_FREE_BUFR_LIST(msg.data.fullenc_uba.start);
			return NCSCC_RC_SUCCESS;
		}

	case MDS_ENC_TYPE_DIRECT_BUFF:
		{
			mds_free_direct_buff(msg.data.buff_info.buff);
			return NCSCC_RC_SUCCESS;
		}
		break;
	default:
		return NCSCC_RC_FAILURE;
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_check_for_mds_existence
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_check_for_mds_existence(NCS_SEL_OBJ sel_obj, MDS_HDL env_hdl,
					 MDS_SVC_ID fr_svc_id, MDS_SVC_ID to_svc_id)
{
	MDS_SVC_INFO *svc_cb = NULL;
	MDS_SUBSCRIPTION_INFO *sub_info = NULL;	/* Subscription info */

	if (gl_mds_mcm_cb != NULL) {
		if (NCSCC_RC_SUCCESS == mds_svc_tbl_get((MDS_PWE_HDL)env_hdl, fr_svc_id, ((NCSCONTEXT)&svc_cb))) {
			mds_subtn_tbl_get(svc_cb->svc_hdl, to_svc_id, &sub_info);
			if (sub_info != NULL) {
				return NCSCC_RC_SUCCESS;
			}
		}
	}
	m_NCS_SEL_OBJ_DESTROY(sel_obj);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_add_bcast_list
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_add_bcast_list(SEND_MSG *msg, MDS_BCAST_ENUM bcast_enum, USRBUF *usr_buf,
				    MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_ver, MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver,
				    MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word)
{
	MDS_BCAST_BUFF_LIST *tmp_bcast_hdr = NULL, *add_ptr = NULL;

	uint32_t status;

	status = mds_mcm_search_bcast_list(msg, bcast_enum, rem_svc_sub_ver, rem_svc_arch_word, &tmp_bcast_hdr, 0);

	if (NCSCC_RC_SUCCESS == status) {
		/* Found in list, just modify */
		tmp_bcast_hdr->bcast_flag |= bcast_enum;
		if (BCAST_ENC_FLAT == bcast_enum) {
			tmp_bcast_hdr->bcast_enc_flat = m_MMGR_DITTO_BUFR(usr_buf);
		} else {
			tmp_bcast_hdr->bcast_enc = m_MMGR_DITTO_BUFR(usr_buf);
		}
		return NCSCC_RC_SUCCESS;

	} else {
		/* Not found, alloc and fill */
		tmp_bcast_hdr = msg->mds_bcast_list_hdr;
		add_ptr = m_MMGR_ALLOC_BCAST_BUFF_LIST;

		if (NULL == add_ptr) {
			m_MDS_LOG_ERR("MDS_C_SNDRCV: Memory allocation to bcast faile");
			return NCSCC_RC_FAILURE;
		}

		memset(add_ptr, 0, sizeof(*add_ptr));
		add_ptr->bcast_flag = 0;
		add_ptr->bcast_flag |= bcast_enum;
		if (BCAST_ENC_FLAT == bcast_enum) {
			add_ptr->bcast_enc_flat = m_MMGR_DITTO_BUFR(usr_buf);
		} else {
			add_ptr->bcast_enc = m_MMGR_DITTO_BUFR(usr_buf);
		}
		add_ptr->rem_svc_sub_part_ver = rem_svc_sub_ver;
		add_ptr->msg_fmt_ver = msg_fmt_ver;
		add_ptr->rem_svc_arch_word = rem_svc_arch_word;

		if (NULL == tmp_bcast_hdr) {
			msg->mds_bcast_list_hdr = add_ptr;
			add_ptr->next = NULL;
		} else {
			add_ptr->next = tmp_bcast_hdr;
			msg->mds_bcast_list_hdr = add_ptr;
		}

		return NCSCC_RC_SUCCESS;
	}

}

/****************************************************************************
 *
 * Function Name: mds_mcm_search_bcast_list
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_search_bcast_list(SEND_MSG *msg, MDS_BCAST_ENUM bcast_enum,
				       MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_ver,
				       MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word, MDS_BCAST_BUFF_LIST **mds_mcm_bcast_ptr,
				       bool flag)
{
	MDS_BCAST_BUFF_LIST *tmp_bcast_ptr = NULL;

	tmp_bcast_ptr = msg->mds_bcast_list_hdr;

	while (tmp_bcast_ptr != NULL) {
		if ((rem_svc_arch_word == tmp_bcast_ptr->rem_svc_arch_word)
		    && (rem_svc_sub_ver == tmp_bcast_ptr->rem_svc_sub_part_ver)) {
			if (0 == flag) {	/* O means only check for sub_part and arch word, 1 means match all the three parameters, bcast enum, sub part and arch word */
				*mds_mcm_bcast_ptr = tmp_bcast_ptr;
				return NCSCC_RC_SUCCESS;
			} else {
				if ((bcast_enum & tmp_bcast_ptr->bcast_flag)) {
					*mds_mcm_bcast_ptr = tmp_bcast_ptr;
					return NCSCC_RC_SUCCESS;
				}
			}
		}
		tmp_bcast_ptr = tmp_bcast_ptr->next;
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 *
 * Function Name: mds_mcm_del_all_bcast_list
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mds_mcm_del_all_bcast_list(SEND_MSG *msg)
{
	MDS_BCAST_BUFF_LIST *tmp_bcast_ptr = NULL, *mov_ptr = NULL;
	mov_ptr = msg->mds_bcast_list_hdr;

	while (mov_ptr != NULL) {
		if (BCAST_ENC_FLAT & mov_ptr->bcast_flag) {
			m_MMGR_FREE_BUFR_LIST(mov_ptr->bcast_enc_flat);
		}

		if (BCAST_ENC & mov_ptr->bcast_flag) {
			m_MMGR_FREE_BUFR_LIST(mov_ptr->bcast_enc);
		}
		tmp_bcast_ptr = mov_ptr;
		mov_ptr = mov_ptr->next;
		m_MMGR_FREE_BCAST_BUFF_LIST(tmp_bcast_ptr);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mcm_query_for_node_dest_on_archword
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
static uint32_t mcm_query_for_node_dest_on_archword(MDS_DEST adest, uint8_t *to, MDS_SVC_ARCHWORD_TYPE arch_word)
{
	/* Route present to send the message */
	if (m_MDS_GET_ADEST == adest) {
		*to = DESTINATION_SAME_PROCESS;
	} else if (MDS_SELF_ARCHWORD == arch_word) {
		if ((0 == (MDS_SELF_ARCHWORD & 0x7) && (0 == (arch_word & 0x7)))) {
			if (m_MDS_GET_NODE_ID_FROM_ADEST(m_MDS_GET_ADEST) == m_MDS_GET_NODE_ID_FROM_ADEST(adest)) {
				*to = DESTINATION_ON_NODE;	/* This hash define may give a wrong impression, but actually it means to do flat_enc */
			} else {
				*to = DESTINATION_OFF_NODE;
			}
		} else {
			*to = DESTINATION_ON_NODE;
		}
	} else {
		*to = DESTINATION_OFF_NODE;
	}

	return NCSCC_RC_SUCCESS;
}
